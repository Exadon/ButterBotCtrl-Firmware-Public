#ifndef BUTTERBOTCTRL_FIRMWARE_PORTALWINDOW_H
#define BUTTERBOTCTRL_FIRMWARE_PORTALWINDOW_H

#include <array>
#include "Components/HomeWindow.h"

/**
 * Fork: "Open a portal". Plays a swirling portal animation - pulsing rings with particles spiralling
 * into a bright core - drawn from plain LVGL circles (no arc widget in this LVGL build). The robot
 * keeps the window up for OpenPortalData::durationMs and then returns to idle, which closes it.
 */
class PortalWindow : public HomeWindow {
public:
	PortalWindow(lv_obj_t* parent, const OpenPortalData* params);

	void loop() override;

private:
	static constexpr const char* TITLE = "Portal";
	static constexpr int32_t StageSize = 60;
	static constexpr size_t RingCount = 3;
	static constexpr size_t DotCount = 10;

	lv_obj_t* stage;
	std::array<lv_obj_t*, RingCount> rings{};
	std::array<lv_obj_t*, DotCount> dots{};
	lv_obj_t* core;
	uint64_t startMs;
};

#endif //BUTTERBOTCTRL_FIRMWARE_PORTALWINDOW_H
