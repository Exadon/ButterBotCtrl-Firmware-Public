#ifndef BUTTERBOTCTRL_FIRMWARE_DRIVESPEEDSELECTOR_H
#define BUTTERBOTCTRL_FIRMWARE_DRIVESPEEDSELECTOR_H

#include <array>
#include <functional>
#include <Services/Settings.h>
#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>

class DriveSpeedSelector : public LVObject {
public:
	DriveSpeedSelector(lv_obj_t* parent, DriveSpeed initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(DriveSpeed)>& valCb);
	~DriveSpeedSelector() override;

	void setValue(DriveSpeed value);

	lv_obj_t* widgetLabel;

private:
	ThemeService* theme;
	LVStyle labelDefaultStyle;

	lv_obj_t* selectorLabel;

	uint8_t currentIndex;
	static constexpr uint8_t OptionsNum = 3;
	static constexpr std::array<DriveSpeed, OptionsNum> OptionValueMap = {
		DriveSpeed::Slow,
		DriveSpeed::Normal,
		DriveSpeed::Fast
	};
	static constexpr std::array<const char*, OptionsNum> OptionNameMap = {
		"SLOW",
		"NORMAL",
		"FAST"
	};

	std::function<void(uint32_t)> keyCb;
	const std::function<void(DriveSpeed)> valCb;

	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_DRIVESPEEDSELECTOR_H