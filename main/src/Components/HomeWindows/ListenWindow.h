#ifndef BUTTERBOTCTRL_FIRMWARE_LISTENWINDOW_H
#define BUTTERBOTCTRL_FIRMWARE_LISTENWINDOW_H

#include <LV_Interface/LVGIF.h>
#include "Components/HomeWindow.h"

class ListenWindow : public HomeWindow {
public:
	/**
	 * Window that shows BB Listen action
	 * @param parent parent LVGL object
	 * @param params data for Listen action
	 */
	ListenWindow(lv_obj_t* parent, const ListenData* params);

	void loop() override;
	void onData(const BBData* data) override;
	void onDismiss() override;

private:
	static constexpr const char* TITLE = "Listen";

	LVGIF* gif = nullptr; // the animated "thinking" icon (owned by its LVGL object)

	void buildUI(ListenData data);
};

#endif //BUTTERBOTCTRL_FIRMWARE_LISTENWINDOW_H
