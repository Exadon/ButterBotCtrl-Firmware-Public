#ifndef BUTTERBOTCTRL_FIRMWARE_CHATWINDOW_H
#define BUTTERBOTCTRL_FIRMWARE_CHATWINDOW_H

#include "Components/HomeWindow.h"

class ChatWindow : public HomeWindow {
public:
	/**
	 * Fork: shows the robot's reply to one of the chat voice lines (Phrase::Chat)
	 * @param parent parent LVGL object
	 * @param params which reply
	 */
	ChatWindow(lv_obj_t* parent, const ChatData* params);

	void onData(const BBData* data) override;

private:
	lv_obj_t* sentenceLabel;

	static constexpr const char* TITLE = "Butter Bot";

	void buildUI(ChatData data);
};

#endif //BUTTERBOTCTRL_FIRMWARE_CHATWINDOW_H
