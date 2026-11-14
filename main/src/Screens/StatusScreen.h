#ifndef BUTTERBOTCTRL_FIRMWARE_STATUSSCREEN_H
#define BUTTERBOTCTRL_FIRMWARE_STATUSSCREEN_H

#include <string>
#include <LV_Interface/LVScreen.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ButtonInput.h>

#include "Enums.hpp"
#include "Util/ButtonGate.h"
#include "Services/Com.h"
#include "Services/ThemeService.h"
#include "Components/TopBar.h"

/**
 * Fork: the robot's health on one page - battery, memory, WiFi and AI mode, the last AI conversation and firmware.
 * Asks the robot (Ctrl::Status) every RefreshMs while open; the joystick scrolls, Override or Shut Up goes back.
 */
class StatusScreen : public LVScreen {
public:
	StatusScreen();
	~StatusScreen() override;

private:
	static constexpr uint32_t RefreshMs = 3000;
	static constexpr uint32_t NoReplyMs = 7000;

	ThemeService* theme;
	ButtonInput* buttonInput;
	Com* com;

	TopBar* topBar = nullptr;
	lv_obj_t* list = nullptr;
	lv_obj_t* body = nullptr;
	LVStyle textStyle;

	std::string text; // body label shows this (text_static)
	uint32_t lastAsk = 0;
	uint32_t lastReply = 0;
	uint32_t openedAt = 0;
	bool everReplied = false;

	ButtonGate buttonGate;

	void loop() override;
	void ask();
	void handleButtonEvent(Button btn, ButtonInput::Action action);
	void handleDataEvent(BB::State state, BB::Action action, const std::vector<uint8_t>& data);
	void show(const std::string& t);
};

#endif //BUTTERBOTCTRL_FIRMWARE_STATUSSCREEN_H
