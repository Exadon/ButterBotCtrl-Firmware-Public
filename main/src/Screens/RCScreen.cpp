#include "RCScreen.h"
#include <Periph/Joystick.h>
#include "Util/WiFiAccessPoint.h"
#include <Periphery/WiFi.h>
#include <Services/Com.h>
#include "Util/Feed.h"
#include <LV_Interface/LVGL.h>
#include "HomeScreen.h"
#include "PairingScreen.h"
#include "Fonts/font.hpp"

RCScreen::RCScreen(){
	const Application* app = Application::getApp();
	accessPoint = app->getPeriphery<WiFiAccessPoint>();
	feed = newObject<Feed>();

	LVGL* lvgl = app->getService<LVGL>();

	accessPoint->OnConnectionEvent.bind(lvgl, [this](WiFiAccessPoint::EventType type){ onAPConnectionStatus(type); });

	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);

	feedBuf = (uint8_t*) heap_caps_malloc(128*128*2, MALLOC_CAP_SPIRAM);
	memset(feedBuf, 0, 128*128*2);
	imgDsc.data = feedBuf;

	feedImg = lv_img_create(*this);
	lv_obj_set_pos(feedImg, 0, 0);
	lv_img_set_src(feedImg, &imgDsc);

	topBar = new TopBar(*this);

	exitHint = lv_label_create(*this);
	lv_label_set_text_static(exitHint, "Override: exit RC");
	lv_obj_set_style_text_font(exitHint, &lv_font_butter, 0);
	lv_obj_set_style_text_color(exitHint, lv_color_hex(0x39ff6a), 0);
	lv_obj_set_style_bg_color(exitHint, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(exitHint, LV_OPA_70, 0);
	lv_obj_set_style_pad_hor(exitHint, 4, 0);
	lv_obj_set_style_pad_ver(exitHint, 3, 0);
	lv_obj_set_style_radius(exitHint, 3, 0);
	lv_obj_align(exitHint, LV_ALIGN_BOTTOM_MID, 0, -4);
	showExitHint(ExitHintStartMs);

	Com* com = app->getService<Com>();

	if(com == nullptr){
		return;
	}

	com->sendDrive(lastData);

	com->onConnStatus.bind(lvgl, [this](Com::ConnStatus status){ onConnectionStatus(status); });

	if(ButtonInput* buttonInput = app->getService<ButtonInput>()){
		buttonInput->OnButtonEvent.bind(lvgl, [this](Enum<int> btn, ButtonInput::Action action){ handleButtonEvent(static_cast<Button>(static_cast<int>(btn)), action); });
	}
}

RCScreen::~RCScreen(){
	const Application* app = Application::getApp();
	LVGL* lvgl = app->getService<LVGL>();

	if(Com* com = app->getService<Com>()){
		com->onConnStatus.unbind(lvgl);
	}

	if(ButtonInput* buttonInput = app->getService<ButtonInput>()){
		buttonInput->OnButtonEvent.unbind(lvgl);
	}

	accessPoint->OnConnectionEvent.unbind(lvgl);

	free(feedBuf);

	delete *feed;
}

void RCScreen::loop(){
	if(topBar != nullptr){
		topBar->loop();
	}

	if(exitHint != nullptr){
		lv_obj_update_flag(exitHint, LV_OBJ_FLAG_HIDDEN, millis() >= exitHintUntil);
	}

	feed->nextFrame([this](const uint16_t* buf){
		memcpy(feedBuf, buf, 128*128*2);
		lv_obj_invalidate(feedImg);
	});

	const Application* app = Object::getApp();
	Com* com = app->getService<Com>();
	Joystick* joystick = app->getDevice<Joystick>();

	if(com == nullptr || joystick == nullptr){
		return;
	}

	DriveData data{ .joystickX = joystick->getHorizontal(), .joystickY = joystick->getVertical() };
	if(data.joystickX == lastData.joystickX && data.joystickY == lastData.joystickY){
		return;
	}

	// Rate-limit movement updates, but never delay a stop: letting go of the stick halts the robot at once
	const bool isStop = data.joystickX == 0 && data.joystickY == 0;
	if(!isStop && millis() - lastDataSend < 100){
		return;
	}

	lastDataSend = millis();
	lastData = data;
	com->sendDrive(data);
}

void RCScreen::showExitHint(uint64_t durationMs){
	exitHintUntil = millis() + durationMs;
	if(exitHint != nullptr){
		lv_obj_remove_flag(exitHint, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(exitHint);
	}
}

void RCScreen::handleButtonEvent(Button btn, ButtonInput::Action action){
	// Only presses do anything here. Acting on press (not release) also means the release of the Override
	// hold that started RC - pressed on the home screen - can't exit RC straight away.
	if(action != ButtonInput::Action::Press || btn == Button::Joystick){
		return; // the joystick button has no function
	}

	if(btn != Button::ManualOverride){
		if(Com* com = Object::getApp()->getService<Com>()){
			com->sendCommand(Ctrl::Command::RCSound);
		}
		showExitHint(ExitHintRepeatMs);
		return;
	}

	// Manual Override: leave RC right away

	if(Com* com = Object::getApp()->getService<Com>()){
		com->sendCommand(Ctrl::Command::ExitRC);
	}

	transition([]() {
		return std::make_unique<HomeScreen>();
	});
}

void RCScreen::onConnectionStatus(Com::ConnStatus status){
	if(status != Com::ConnStatus::Disconnected){
		return;
	}

	transition([]() {
		return std::make_unique<PairingScreen>();
	});
}

void RCScreen::onAPConnectionStatus(WiFiAccessPoint::EventType type){
	if(type != WiFiAccessPoint::EventType::Disconnect){
		return;
	}

	transition([]() {
		return std::make_unique<HomeScreen>();
	});
}
