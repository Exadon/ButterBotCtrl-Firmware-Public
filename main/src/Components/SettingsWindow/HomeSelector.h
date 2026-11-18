#ifndef BUTTERBOTCTRL_FIRMWARE_HOMESELECTOR_H
#define BUTTERBOTCTRL_FIRMWARE_HOMESELECTOR_H

#include <LV_Interface/LVObject.h>
#include <LV_Interface/LVStyle.h>
#include <Services/ThemeService.h>
#include <Services/Settings.h>

/** Fork: Settings > HOME - the home screen shows the button hints or the little robot (RobotElement). */
class HomeSelector : public LVObject {
public:
	HomeSelector(lv_obj_t* parent, HomeView initVal, const std::function<void(uint32_t)>& keyCb, const std::function<void(HomeView)>& valCb);
	~HomeSelector() override;

	lv_obj_t* widgetLabel;

private:
	ThemeService* theme;
	LVStyle labelDefaultStyle;

	lv_obj_t* selectorLabel;

	uint8_t currentIndex;
	static constexpr uint8_t OptionsNum = 2;
	static constexpr std::array<HomeView, OptionsNum> OptionValueMap = {
		HomeView::Hints,
		HomeView::Robot
	};
	static constexpr std::array<const char*, OptionsNum> OptionNameMap = {
		"HINTS",
		"ROBOT"
	};

	std::function<void(uint32_t)> keyCb;
	const std::function<void(HomeView)> valCb;

	void buildUI(lv_obj_t* parent);
};


#endif //BUTTERBOTCTRL_FIRMWARE_HOMESELECTOR_H
