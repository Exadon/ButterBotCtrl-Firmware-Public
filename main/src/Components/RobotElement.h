#ifndef BUTTERBOTCTRL_FIRMWARE_ROBOTELEMENT_H
#define BUTTERBOTCTRL_FIRMWARE_ROBOTELEMENT_H

#include <cstdint>
#include <LV_Interface/LVObject.h>
#include <Services/ThemeService.h>

/**
 * Fork: the home screen's "ROBOT" view (Settings > HOME), in place of the button hints for people who know them.
 * A little ButterBot drawn from plain shapes in the theme's colours: tracks with three wheels, a thin post, a cube head
 * with one big camera eye, a short chin, two wire loops and a light on top, and two jointed arms hanging from the
 * sides of the head with claw hands. It idles on its own: it bobs, blinks, glances around, rolls a little to and fro
 * with its treads turning, its light blinks, and now and then it waves. Floating like the guide; the owner shows/hides
 * it and calls loop() while shown.
 */
class RobotElement : public LVObject {
public:
	explicit RobotElement(lv_obj_t* parent);

	/** Moves everything for the current time. Cheap: a few position changes per frame. */
	void loop();

	/** Starts from a neutral pose (called when it becomes visible, so it doesn't jump mid-animation). */
	void reset();

private:
	ThemeService* theme;

	lv_obj_t* rig;      // everything that rolls (tracks + robot)
	lv_obj_t* upper;    // everything that bobs (head, arms, post)
	lv_obj_t* pupil;
	lv_obj_t* lid;
	lv_obj_t* light;
	lv_obj_t* armL;     // whole arm, pivoting at the shoulder
	lv_obj_t* armR;
	lv_obj_t* treads[4];

	uint32_t t0 = 0;
	uint32_t nextBlink = 0, blinkUntil = 0;
	uint32_t nextGlance = 0;
	int8_t glanceX = 0, glanceY = 0;
	float pupilX = 0, pupilY = 0;
	uint32_t nextWave = 0, waveStart = 0;
	uint32_t nextFlash = 0, flashUntil = 0;
	int32_t lastRigX = 0;
	float treadPhase = 0;
	int32_t lastWave = 0;

	void buildUI();
	lv_obj_t* buildArm(int32_t x, int32_t y, bool left);
	static uint32_t randomBetween(uint32_t lo, uint32_t hi);
};


#endif //BUTTERBOTCTRL_FIRMWARE_ROBOTELEMENT_H
