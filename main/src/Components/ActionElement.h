#ifndef BUTTERBOTCTRL_FIRMWARE_ACTIONELEMENT_H
#define BUTTERBOTCTRL_FIRMWARE_ACTIONELEMENT_H

#include <functional>
#include <string>
#include <vector>
#include <BBData.h>
#include <Scenarios.h>
#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>
#include "Util/ScenarioMapping.h"

/** Where the action list is: the folder list (folder = -1) or inside a folder, and which row is focused. */
struct ActionListState {
	int16_t folder = -1;
	uint16_t index = 0;
};

class ActionElement : public LVObject {
public:
	/**
	 * Full-screen pop-up listing the actions, grouped into folders (Util/ScenarioMapping.h), with SETTINGS as the
	 * first row of the folder list. The joystick moves the highlight; Shut Up (the keypad's ENTER) opens a folder,
	 * Settings or runs an action; the home screen calls back() on Manual Override.
	 * @param parent parent LVGL object
	 * @param inputGroup screen input group the list buttons are added to
	 * @param settingsCb invoked when the SETTINGS row is selected
	 * @param scenarioCb invoked when an action is selected
	 * @param initial where to open (restores the last position)
	 */
	ActionElement(lv_obj_t* parent, lv_group_t* inputGroup, std::function<void()> settingsCb,
	              std::function<void(BB::Action::Scenario, ScenarioData)> scenarioCb, ActionListState initial = {});
	~ActionElement() override;

	ActionListState getState() const;

	bool inFolder() const{ return currentFolder >= 0; }

	/** Leave the open folder and return to the folder list, focusing that folder. */
	void back();

private:
	static constexpr int32_t Width = 128;
	static constexpr int32_t Height = 120;
	static constexpr uint32_t ScrollSpeedPxPerSec = 40;

	ThemeService* theme;
	lv_group_t* inputGroup;
	const std::function<void()> settingsCb;
	const std::function<void(BB::Action::Scenario, ScenarioData)> scenarioCb;

	// The folder list shows SETTINGS as row 0, so folder i is row i + FolderRowOffset
	static constexpr int32_t FolderRowOffset = 1;
	static constexpr const char* SettingsTitle = "SETTINGS";

	int16_t currentFolder = -1;
	uint16_t elIndex = 0;

	// Rebuilding the rows from inside a row's own click event is unsafe, so level changes are deferred
	int16_t pendingFolder = -1;
	uint16_t pendingIndex = 0;
	static void asyncShow(void* self);
	void requestShow(int16_t folder, uint16_t focusIndex);

	// Index-aligned with the visible rows
	std::vector<lv_obj_t*> buttons;
	std::vector<Action> rowActions;
	// Second-line texts. reserve()d before any label is created and never mutated afterwards while the rows
	// exist - the labels hold the c_str() pointers (text_static)
	std::vector<std::string> phraseTexts;

	LVStyle buttonStyle;
	LVStyle buttonFocusedStyle;
	LVStyle labelStyle;
	LVStyle subLabelStyle;

	void buildStyles();
	void show(int16_t folder, uint16_t focusIndex);
	lv_obj_t* addButton(const char* title, const char* subtitle, bool settingsIcon = false);
	void moveFocus(int8_t dir);
	void onItemClicked(const lv_obj_t* btn);
	static std::string collectPhrases(BB::Action::Scenario scenario, ScenarioData data);
	static size_t folderSize(const ActionFolder& folder);
};


#endif //BUTTERBOTCTRL_FIRMWARE_ACTIONELEMENT_H
