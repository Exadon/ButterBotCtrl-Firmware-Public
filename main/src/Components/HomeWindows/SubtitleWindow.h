#ifndef BUTTERBOTCTRL_FIRMWARE_SUBTITLEWINDOW_H
#define BUTTERBOTCTRL_FIRMWARE_SUBTITLEWINDOW_H

#include "Components/HomeWindow.h"

class SubtitleWindow : public HomeWindow {
public:
	/**
	 * Fork: AI-mode subtitles - what the PC heard you say and what ButterBot answers.
	 * @param parent parent LVGL object
	 * @param params the two texts
	 */
	SubtitleWindow(lv_obj_t* parent, const SubtitleData* params);

	void onData(const BBData* data) override;

private:
	lv_obj_t* heardLabel;
	lv_obj_t* sayLabel;
	LVStyle heardStyle;

	// Labels keep pointers to these (text_static)
	std::string heardText;
	std::string sayText;

	static constexpr const char* TITLE = "Butter Bot";

	void buildUI(const SubtitleData& data);
};

#endif //BUTTERBOTCTRL_FIRMWARE_SUBTITLEWINDOW_H
