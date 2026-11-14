#include <nvs_flash.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <string>
#include <Util/stdafx.h>
#include <esp_app_desc.h>
#include <esp_system.h>
#include <esp_gap_ble_api.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <esp_heap_caps.h>
#include <mbedtls/base64.h>
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
		xTaskCreatePinnedToCore([](void* self){ static_cast<ButterBotController*>(self)->diagTask(); }, "bbdiag", 6144,
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
					else if(line == "forget") forgetBonds();
					else if(line == "scr") sendScreen(false);
					else if(line == "scr full") sendScreen(true);
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

	/*
	 * Fork: screen mirror for Dimension Hopper. "scr" prints one line
	 *   [BBSCR] dev=ctrl w=128 h=128 key=0|1 rows=<changed rows> n=<bytes> d=<base64>
	 * d decodes to a 16-byte row mask (bit y%8 of byte y/8 = row y included), then the included rows'
	 * RGB565 pixels (as stored in the frame buffer), run-length coded: a control byte c >= 0x80 means
	 * (c & 0x7f) + 2 copies of the next pixel, c < 0x80 means c + 1 literal pixels follow.
	 * Only rows that changed since the last "scr" are sent; "scr full" (or the first request) sends all.
	 * At 115200 baud a static menu costs a few hundred bytes, a full busy frame up to ~44 KB (~4 s).
	 */
	static constexpr int ScrW = LVGL::Width, ScrH = LVGL::Height, ScrN = ScrW * ScrH;
	uint16_t* scrNow = nullptr;
	uint16_t* scrPrev = nullptr;
	uint8_t* scrPacked = nullptr;
	uint8_t* scrText = nullptr;
	bool scrPrevValid = false;
	static constexpr size_t ScrPackedMax = ScrH / 8 + ScrN * 2 + ScrN / 128 + 16;
	static constexpr size_t ScrTextMax = (ScrPackedMax + 2) / 3 * 4 + 4;

	void sendScreen(bool full){
		LVGL* lvgl = getService<LVGL>();
		if(lvgl == nullptr){
			printf("[BBSCR] dev=ctrl busy=nodisplay\n");
			return;
		}
		auto grab = [](size_t bytes){ return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); };
		if(scrNow == nullptr) scrNow = static_cast<uint16_t*>(grab(ScrN * 2));
		if(scrPrev == nullptr) scrPrev = static_cast<uint16_t*>(grab(ScrN * 2));
		if(scrPacked == nullptr) scrPacked = static_cast<uint8_t*>(grab(ScrPackedMax));
		if(scrText == nullptr) scrText = static_cast<uint8_t*>(grab(ScrTextMax));
		if(scrNow == nullptr || scrPrev == nullptr || scrPacked == nullptr || scrText == nullptr){
			printf("[BBSCR] dev=ctrl busy=nomem\n");
			return;
		}

		lvgl->snapshot(scrNow);
		if(!scrPrevValid) full = true;

		uint8_t* const mask = scrPacked;
		memset(mask, 0, ScrH / 8);
		int rows = 0;
		for(int y = 0; y < ScrH; ++y){
			if(full || memcmp(scrNow + y * ScrW, scrPrev + y * ScrW, ScrW * 2) != 0){
				mask[y / 8] |= 1u << (y % 8);
				rows++;
			}
		}

		// Gather the changed rows' pixels in order and run-length code them.
		uint8_t* o = scrPacked + ScrH / 8;
		const int total = rows * ScrW;
		// Index of each included row, so the k-th sent pixel is found in O(1).
		int rowOf[ScrH];
		for(int y = 0, i = 0; y < ScrH; ++y) if(mask[y / 8] & (1u << (y % 8))) rowOf[i++] = y;
		auto at = [&](int k){ return scrNow[rowOf[k / ScrW] * ScrW + k % ScrW]; };

		int k = 0;
		while(k < total){
			const uint16_t v = at(k);
			int run = 1;
			while(k + run < total && run < 129 && at(k + run) == v) run++;
			if(run >= 2){
				*o++ = 0x80 | (run - 2);
				*o++ = v & 0xff;
				*o++ = v >> 8;
				k += run;
				continue;
			}
			// literal: until the next run of 3+ or 128 pixels
			uint8_t* ctl = o++;
			int lit = 0;
			while(k < total && lit < 128){
				const uint16_t w = at(k);
				if(lit > 0 && k + 2 < total && at(k + 1) == w && at(k + 2) == w) break;
				*o++ = w & 0xff;
				*o++ = w >> 8;
				lit++;
				k++;
			}
			*ctl = lit - 1;
		}
		const size_t packed = o - scrPacked;

		size_t textLen = 0;
		mbedtls_base64_encode(scrText, ScrTextMax, &textLen, scrPacked, packed);

		flockfile(stdout);
		printf("[BBSCR] dev=ctrl w=%d h=%d key=%d rows=%d n=%u d=", ScrW, ScrH, full ? 1 : 0, rows, (unsigned) packed);
		fwrite(scrText, 1, textLen, stdout);
		fputc('\n', stdout);
		fflush(stdout);
		funlockfile(stdout);

		memcpy(scrPrev, scrNow, ScrN * 2);
		scrPrevValid = true;
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
		printf("[BBSTAT] dev=ctrl fw=%s batt=%d link=%s up=%llu reset=%s\n",
		       esp_app_get_description()->version,
		       battery != nullptr ? static_cast<int>(battery->getPerc()) : -1,
		       com == nullptr ? "unknown" : (com->getStatus() == Com::ConnStatus::Connected ? "connected" : "disconnected"),
		       static_cast<unsigned long long>(esp_timer_get_time() / 1000000),
		       resetReasonName());
	}

	// Fork: "forget" drops every Bluetooth bond, so the controller pairs from scratch with the next robot it finds.
	static void forgetBonds(){
		int num = esp_ble_get_bond_device_num();
		int removed = 0;
		if(num > 0){
			std::vector<esp_ble_bond_dev_t> list(static_cast<size_t>(num));
			if(esp_ble_get_bond_device_list(&num, list.data()) == ESP_OK){
				for(int i = 0; i < num; ++i){
					if(esp_ble_remove_bond_device(list[static_cast<size_t>(i)].bd_addr) == ESP_OK) removed++;
				}
			}
		}
		printf("[BBFORGET] dev=ctrl removed=%d\n", removed);
	}

	static const char* resetReasonName(){
		switch(esp_reset_reason()){
			case ESP_RST_POWERON: return "poweron";
			case ESP_RST_SW: return "software";
			case ESP_RST_PANIC: return "crash";
			case ESP_RST_INT_WDT:
			case ESP_RST_TASK_WDT:
			case ESP_RST_WDT: return "watchdog";
			case ESP_RST_BROWNOUT: return "brownout";
			case ESP_RST_DEEPSLEEP: return "sleep";
			case ESP_RST_EXT: return "external";
			case ESP_RST_USB: return "usb";
			default: return "other";
		}
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
