#include <nvs_flash.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <string>
#include <Util/stdafx.h>
#include <esp_app_desc.h>
#include <cstdlib>
#include <Core/EntryPoint.h>
#include <Devices/Display.h>
#include <Drivers/Input/InputGPIO.h>
#include <Drivers/Output/OutputGPIO.h>
#include <FileSystem/SPIFFS.h>
#include <LV_Interface/FSLVGL.h>
#include <LV_Interface/LVGL.h>
#include <Periphery/WiFi.h>
#include <Services/ButtonInput.h>
#include <Services/LED/LED.h>
#include <Util/EfuseMeta.h>
#include <Util/WiFiAccessPoint.h>
#include <bootloader_random.h>

#include "Periph/Bluetooth.h"
#include "Periph/Joystick.h"
#include "BLE/GAP.h"
#include "BLE/Client.h"
#include "Services/Com.h"

#include "HardwareConfiguration.hpp"
#include "Enums.hpp"
#include "Services/Battery.h"
#include "Services/BatteryIndicator.h"
#include "Services/JoystickInputLVGL.h"
#include "Services/ShutdownService.h"
#include "Services/LEDController.h"
#include "Services/ThemeService.h"
#include "Services/RobotState.h"
#include "Services/Settings.h"
#include "Screens/IntroScreen.h"
#include "JigHWTest/JigHWTest.h"

class ButterBotController : public Application {
	GENERATED_BODY(ButterBotController, Application, void)

protected:
	virtual void begin() noexcept override{
		bootloader_random_enable();
		srand(esp_random());
		bootloader_random_disable();

		esp_log_level_set("*", ESP_LOG_WARN);
		// Pairing is low-volume and hard to debug blind: keep the BLE connection story on the console.
		esp_log_level_set("BLE::GAP", ESP_LOG_INFO);
		esp_log_level_set("BLE::Client", ESP_LOG_INFO);

		// Stack overflow otherwise - increase Threaded/AsyncEntity stack size if logging is needed
		esp_log_level_set("Com", ESP_LOG_NONE);

		auto ret = nvs_flash_init();
		if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		if(JigHWTest::checkJig()){
			ESP_LOGE("checkJig", "Jig\n");

			JigHWTest* test = new JigHWTest();
			test->start();
			vTaskDelete(nullptr);
		} else{
			ESP_LOGE("checkJig", "Hello\n");
		}

		if(!EfuseMeta::check()){
			while(true){
				vTaskDelay(1000);
				EfuseMeta::log();
			}
		}

		Battery* battery = registerService<Battery>();

		// Initial battery level check (for early shutdown)
		if(battery->isShutdown()){
			ESP_LOGE("init_bat_check", "CRITICAL");
			ESP_LOGE("init_bat_check", "SHUTTING DOWN");
			ShutdownService::shutdown(ShutdownReason::Battery);
			return;
		}
		ESP_LOGI("init_bat_check", "OK");

		battery->OnLevelChanged.bind(this, &ButterBotController::onBatteryChange);
		battery->begin();

		const HardwareConfiguration* config = registerSingleton<HardwareConfiguration>();
		GPIOPeriph* gpio = registerPeriphery<GPIOPeriph>();
		InputGPIO* inputGPIO = registerDriver<InputGPIO>(config->getGPIOInputs(), gpio);
		OutputPWM* outputPWM = registerDriver<OutputPWM>(config->getPwmOutputs());

		static const std::vector<std::pair<Enum<int>, InputPin>> buttonInputs = {
			{ Button::Joystick, { inputGPIO, BTN_JOY } },
			{ Button::ManualOverride, { inputGPIO, BTN_MANOVER } },
			{ Button::Poke, { inputGPIO, BTN_POKE } },
			{ Button::ShutUp, { inputGPIO, BTN_SHUTUP } },
			{ Button::Summon, { inputGPIO, BTN_SUMMON } },
		};

		ButtonInput* buttonInput = registerService<ButtonInput>(buttonInputs, false);

		static const std::vector<std::pair<LEDs, OutputPin>> ledPins = {
			{ LEDs::BigGreen, { outputPWM, (int)LEDs::BigGreen } },
			{ LEDs::Wifi, { outputPWM, (int)LEDs::Wifi } },
			{ LEDs::Power, { outputPWM, (int)LEDs::Power } },
			{ LEDs::BatteryLow, { outputPWM, (int)LEDs::BatteryLow } },
			{ LEDs::TftBacklight, { outputPWM, (int)LEDs::TftBacklight } },
		};

		LED<LEDs, RGB_LEDs>* ledService = registerService<LED<LEDs, RGB_LEDs>>(true);
		ledService->reg(ledPins);

		auto* ledController = registerService<LEDController>(ledService);
		ledController->powerLedOn();

		Display* display = registerDevice<Display>(config->getDisplayBusConfig(), config->getDisplayPanelConfig(), [](Sprite& canvas) {
			canvas.setPsram(true);
			canvas.setColorDepth(lgfx::rgb565_2Byte);
			canvas.createSprite(128, 128);
		});
		display->getLGFX().setSwapBytes(true);

		// Clear screen
		display->getCanvas().fillScreen(TFT_BLACK);
		display->commit();

		ledService->on(LEDs::TftBacklight, 1.0f);

		Joystick* joystick = registerDevice<Joystick>((gpio_num_t)JOY_H, (gpio_num_t)JOY_V, false);

		auto bt = new Bluetooth();
		auto gap = registerSingleton<BLE::GAP>();
		auto client = new BLE::Client(gap);

		auto* robotState = registerService<RobotState>();

		// GAP connected means devices are paired/connected, Com event means service is connected
		auto com = registerService<Com>(client, false);

		com->onData.bind(this, [this, robotState](BB::State state, BB::Action action, std::vector<uint8_t> data) {
			// BatteryLevel is a status side-channel, not an action; consume it and leave idle tracking untouched
			if(state == BB::State::Idle && action.idle == BB::Action::Idle::BatteryLevel){
				if(data.size() != sizeof(BatteryLevelData)){
					return;
				}

				auto levelData = (BatteryLevelData*)data.data();
				robotState->setBotBatteryLevel(levelData->level);
				robotState->setChargeState(levelData->charge);
				return;
			}

			if(state == BB::State::Idle && action.idle == BB::Action::Idle::ShutUp){
				if(data.size() != sizeof(ShutUpData)){
					return;
				}

				const auto shutUpData = (ShutUpData*)data.data();
				robotState->setMuted(shutUpData->muted);
				return;
			}

			if(state == BB::State::Idle && action.idle == BB::Action::Idle::SettingsSync){
				if(data.size() != sizeof(SettingsSyncData)){
					return;
				}

				const SettingsSyncData* sync = (const SettingsSyncData*)data.data();
				if(Settings* settings = getService<Settings>()){
					SettingsStruct currentSet = settings->get();
					currentSet.frontSensor = sync->frontSensor;
					currentSet.bottomSensor = sync->bottomSensor;
					currentSet.activityMode = sync->activityMode;
					settings->set(currentSet);
					settings->store();
				}
				return;
			}

			robotState->setIdleNone(state == BB::State::Idle && action.idle == BB::Action::Idle::None);
		});

		Settings* settings = registerService<Settings>();
		if(settings != nullptr){
			ledService->on(LEDs::TftBacklight, settings->get().screenBrightness);
		}

		com->onConnStatus.bind(this, [com, settings](const Com::ConnStatus status) {
			if(status != Com::ConnStatus::Connected || settings == nullptr){
				return;
			}

			const SettingsStruct currentSet = settings->get();
			com->sendSettings({ .frontSensor = currentSet.frontSensor, .bottomSensor = currentSet.bottomSensor, .activityMode = currentSet.activityMode });
		});

		if(!SPIFFS::init()){
			return;
		}

		auto* themeService = registerService<ThemeService>(settings != nullptr ? settings->get().currentTheme : Theme::Main);
		WiFi* wifi = registerPeriphery<WiFi>();
		WiFiAccessPoint* ap = registerPeriphery<WiFiAccessPoint>(wifi);

		registerService<BatteryIndicator>(battery, ledService);

		registerService<ShutdownService>(true);

		LVGL* lvgl = registerService<LVGL>(display);

		FSLVGL* fslvgl = registerService<FSLVGL>('S', themeService->getCache());

		// Menus select with Shut Up (LVGL ENTER), not the joystick press: pressing a stick always tilts it a
		// little, which nudged the selection or changed a value right before the press registered.
		registerService<JoystickInputLVGL>(joystick, buttonInput, Enum<int>(Button::ShutUp));

		lvgl->startScreen([]() {
			return std::make_unique<IntroScreen>();
		});
		lvgl->startThread();

		startStatusReporter();
	}

private:
	/**
	 * Fork: one status line on the USB serial console every StatusIntervalMs, e.g.
	 *   [BBSTAT] dev=ctrl fw=v1.1-exadon.1 batt=81 link=connected up=1234
	 * Dimension Hopper's "Check status" listens for it (without resetting the board).
	 */
	static constexpr uint64_t StatusIntervalMs = 5000;

	/*
	 * Diagnostics stream for Dimension Hopper's Diagnostics tab. Off by default; "diag on" over the serial
	 * port enables it for DiagHoldMs (the app re-sends it while connected), "diag off" stops it.
	 * While on, a [BBDIAG] line is printed every DiagIntervalMs: joystick (raw + processed + calibrated
	 * centre), the five buttons, battery and robot link. Runs in its own low-priority task.
	 */
	static constexpr uint64_t DiagIntervalMs = 100;
	static constexpr uint64_t DiagHoldMs = 15000;
	uint64_t diagUntil = 0;
	std::atomic<uint32_t> buttonsDown{ 0 }; // bit per Button enum value

	void startStatusReporter(){
		if(ButtonInput* bi = getService<ButtonInput>()){
			bi->OnButtonEvent.bind(this, [this](Enum<int> btn, ButtonInput::Action action){
				const uint32_t bit = 1u << static_cast<int>(btn);
				if(action == ButtonInput::Action::Press) buttonsDown |= bit;
				else buttonsDown &= ~bit;
			});
		}
		xTaskCreatePinnedToCore([](void* self){ static_cast<ButterBotController*>(self)->diagTask(); }, "bbdiag", 4096,
		                        this, 1, nullptr, tskNO_AFFINITY);
	}

	void diagTask(){
		const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

		std::string line;
		uint64_t nextStatus = millis() + StatusIntervalMs;
		uint64_t nextDiag = 0;
		for(;;){
			char buf[32];
			const int n = read(STDIN_FILENO, buf, sizeof(buf));
			for(int i = 0; i < n; ++i){
				if(buf[i] == '\n' || buf[i] == '\r'){
					if(line == "diag on") diagUntil = millis() + DiagHoldMs;
					else if(line == "diag off") diagUntil = 0;
					line.clear();
				}else if(line.size() < 32){
					line += buf[i];
				}
			}

			const uint64_t now = millis();
			if(now >= nextStatus){
				nextStatus = now + StatusIntervalMs;
				printStatus();
			}
			if(now < diagUntil && now >= nextDiag){
				nextDiag = now + DiagIntervalMs;
				printDiag();
			}
			vTaskDelay(pdMS_TO_TICKS(20));
		}
	}

	void printDiag() const{
		const Joystick* joy = getDevice<Joystick>();
		const Battery* battery = getService<Battery>();
		const Com* com = getService<Com>();
		const uint32_t b = buttonsDown.load();
		auto down = [b](Button btn){ return static_cast<int>((b >> static_cast<int>(btn)) & 1u); };
		Joystick::Diag j{};
		if(joy != nullptr){
			j = joy->getDiag();
		}
		printf("[BBDIAG] dev=ctrl joy_x=%d joy_y=%d raw_x=%d raw_y=%d center_x=%d center_y=%d "
		       "override=%d poke=%d shutup=%d summon=%d joybtn=%d batt=%d link=%d\n",
		       j.x, j.y, j.rawX, j.rawY, j.centerX, j.centerY,
		       down(Button::ManualOverride), down(Button::Poke), down(Button::ShutUp), down(Button::Summon),
		       down(Button::Joystick),
		       battery != nullptr ? static_cast<int>(battery->getPerc()) : -1,
		       com != nullptr && com->getStatus() == Com::ConnStatus::Connected ? 1 : 0);
	}

	void printStatus() const{
		const Battery* battery = getService<Battery>();
		const Com* com = getService<Com>();
		printf("[BBSTAT] dev=ctrl fw=%s batt=%d link=%s up=%llu\n",
		       esp_app_get_description()->version,
		       battery != nullptr ? static_cast<int>(battery->getPerc()) : -1,
		       com == nullptr ? "unknown" : (com->getStatus() == Com::ConnStatus::Connected ? "connected" : "disconnected"),
		       static_cast<unsigned long long>(esp_timer_get_time() / 1000000));
	}

	void onBatteryChange(const Battery::Level level){
		if(level != Battery::Level::Critical){
			return;
		}

		ShutdownService::shutdown(ShutdownReason::Battery);
		vTaskDelay(portMAX_DELAY);
	}
};

CMF_MAIN(ButterBotController)
