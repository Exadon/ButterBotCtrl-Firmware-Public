#ifndef BUTTERBOTCTRL_FIRMWARE_INTROSCREEN_H
#define BUTTERBOTCTRL_FIRMWARE_INTROSCREEN_H

#include <LV_Interface/LVScreen.h>
#include <LV_Interface/LVGIF.h>

class IntroScreen : public LVScreen {
public:
	IntroScreen();

private:
	LVGIF* introGif = nullptr;

	/**
	 * Small "EXADON FORK" + version badge in the empty bottom strip of the intro, so it's obvious at
	 * a glance that the controller runs the community fork. The stock intro itself is left as-is.
	 */
	void buildForkBadge();
};

#endif //BUTTERBOTCTRL_FIRMWARE_INTROSCREEN_H
