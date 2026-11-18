#include "RobotElement.h"

#include <cmath>
#include <esp_random.h>
#include <Util/stdafx.h>

namespace {
	// The drawing, in pixels, inside a 100 x 104 rig (the home area is 120 x 112)
	constexpr int32_t RigW = 100, RigH = 104;
	constexpr int32_t CX = RigW / 2;                   // centre line
	constexpr int32_t HeadW = 38, HeadH = 36, HeadY = 12;
	constexpr int32_t HeadX = CX - HeadW / 2;
	constexpr int32_t ChinW = 22, ChinH = 9;
	constexpr int32_t PostW = 6;
	constexpr int32_t TrackW = 56, TrackH = 14, TrackY = 86;
	constexpr int32_t EyeD = 20, PupilD = 8;
	constexpr int32_t ArmW = 16, ArmH = 60;           // one arm's box: shoulder at the top, claw at the bottom

	constexpr uint32_t BobPeriodMs = 1800;   // gentle up and down
	constexpr float BobPx = 2.0f;
	constexpr uint32_t DriftPeriodMs = 11000; // rolls to and fro
	constexpr float DriftPx = 9.0f;
	constexpr uint32_t BlinkMs = 140;
	constexpr uint32_t WaveMs = 1600;
	constexpr uint32_t FlashMs = 350;

	const lv_color_t LightRed = lv_color_hex(0xE24B4A); // the light on its head, in every theme
	const lv_color_t WireOrange = lv_color_hex(0xF0A030);

	lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius){
		lv_obj_t* o = lv_obj_create(parent);
		lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_style_pad_all(o, 0, 0);
		lv_obj_set_style_border_width(o, 0, 0);
		lv_obj_set_style_radius(o, radius, 0);
		lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
		lv_obj_set_size(o, w, h);
		lv_obj_set_pos(o, x, y);
		return o;
	}

	lv_obj_t* group(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h){
		lv_obj_t* o = box(parent, x, y, w, h, 0);
		lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
		return o;
	}

	/** A filled part with an outline: the robot's grey metal in the theme's colours. */
	lv_obj_t* part(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, lv_color_t fill,
	               lv_color_t outline){
		lv_obj_t* o = box(parent, x, y, w, h, radius);
		lv_obj_set_style_bg_color(o, fill, 0);
		lv_obj_set_style_border_width(o, 1, 0);
		lv_obj_set_style_border_color(o, outline, 0);
		return o;
	}
}

RobotElement::RobotElement(lv_obj_t* parent) : LVObject(parent){
	theme = Application::getApp()->getService<ThemeService>();
	buildUI();
	reset();
}

uint32_t RobotElement::randomBetween(uint32_t lo, uint32_t hi){
	return lo + esp_random() % (hi - lo + 1);
}

lv_obj_t* RobotElement::buildArm(int32_t x, int32_t y, bool left){
	const lv_color_t prim = theme->getPrimaryColor();
	const lv_color_t sec = theme->getSecondaryColor();

	// The whole arm is one box pivoting at its shoulder, so a wave swings it all
	lv_obj_t* arm = group(upper, x, y, ArmW, ArmH);
	lv_obj_set_style_transform_pivot_x(arm, ArmW / 2, 0);
	lv_obj_set_style_transform_pivot_y(arm, 4, 0);

	const int32_t mid = ArmW / 2;
	part(arm, mid - 2, 4, 4, 26, 2, sec, prim);                     // upper arm
	part(arm, mid - 2, 33, 4, 20, 2, sec, prim);                    // forearm
	part(arm, mid - 4, 0, 8, 8, LV_RADIUS_CIRCLE, sec, prim);        // shoulder joint
	part(arm, mid - 4, 28, 8, 8, LV_RADIUS_CIRCLE, sec, prim);       // elbow
	// Claw: an L pointing inwards, towards the robot
	part(arm, mid - 2, ArmH - 8, 4, 6, 1, sec, prim);
	part(arm, left ? mid - 2 : mid - 7, ArmH - 4, 9, 3, 1, sec, prim);
	return arm;
}

void RobotElement::buildUI(){
	// Floating, like the guide: out of the window list's layout and scroll
	lv_obj_add_flag(*this, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_add_flag(*this, LV_OBJ_FLAG_FLOATING);
	lv_obj_remove_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(*this, 120, 112);
	lv_obj_set_style_bg_opa(*this, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(*this, 0, 0);
	lv_obj_set_style_pad_all(*this, 0, 0);

	const lv_color_t prim = theme->getPrimaryColor();
	const lv_color_t sec = theme->getSecondaryColor();
	const lv_color_t tert = theme->getTertiaryColor();

	rig = group(*this, 0, 0, RigW, RigH);
	lv_obj_align(rig, LV_ALIGN_CENTER, 0, 0);

	// Shadow, then the tracks: turning treads and three wheels
	lv_obj_t* shadow = box(rig, CX - TrackW / 2 - 3, TrackY + TrackH - 2, TrackW + 6, 4, 2);
	lv_obj_set_style_bg_color(shadow, sec, 0);
	lv_obj_set_style_bg_opa(shadow, LV_OPA_40, 0);
	lv_obj_t* track = part(rig, CX - TrackW / 2, TrackY, TrackW, TrackH, TrackH / 2, tert, prim);
	lv_obj_set_style_border_width(track, 2, 0);
	lv_obj_set_style_clip_corner(track, true, 0);
	for(lv_obj_t*& t : treads){
		t = box(track, 0, 0, 2, TrackH, 0);
		lv_obj_set_style_bg_color(t, prim, 0);
		lv_obj_set_style_bg_opa(t, LV_OPA_40, 0);
	}
	for(int i = 0; i < 3; ++i){
		part(track, 12 + i * 12, 3, 6, 6, LV_RADIUS_CIRCLE, WireOrange, prim);
	}
	part(rig, CX - 10, TrackY - 3, 20, 4, 1, sec, prim); // the plate the post stands on

	// Everything above the tracks bobs together
	upper = group(rig, 0, 0, RigW, TrackY);

	// Two wire loops and a light on top of the head: drawn first, so the head covers their lower half
	for(int i = 0; i < 2; ++i){
		lv_obj_t* wire = box(upper, HeadX + 6 + i * 7, HeadY - 7, 10, 14, LV_RADIUS_CIRCLE);
		lv_obj_set_style_bg_opa(wire, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(wire, 2, 0);
		lv_obj_set_style_border_color(wire, i == 0 ? WireOrange : LightRed, 0);
	}
	light = part(upper, HeadX + HeadW - 9, HeadY - 3, 5, 4, 1, LightRed, prim);

	// The post from the chin down to the tracks
	part(upper, CX - PostW / 2, HeadY + HeadH + ChinH - 2, PostW, TrackY - (HeadY + HeadH + ChinH) + 4, 1, sec, prim);

	// Arms hang from the sides of the head, wide, down to the tracks
	armL = buildArm(HeadX - ArmW + 4, HeadY + 6, true);
	armR = buildArm(HeadX + HeadW - 4, HeadY + 6, false);

	// Head: a cube with one big camera eye, a little off-centre, and a short chin under it
	part(upper, CX - ChinW / 2, HeadY + HeadH - 2, ChinW, ChinH, 2, sec, prim);
	lv_obj_t* head = part(upper, HeadX, HeadY, HeadW, HeadH, 4, sec, prim);
	lv_obj_set_style_border_width(head, 2, 0);
	lv_obj_t* eye = part(head, 5, (HeadH - 4 - EyeD) / 2 - 1, EyeD, EyeD, LV_RADIUS_CIRCLE, tert, prim);
	lv_obj_set_style_border_width(eye, 2, 0);
	lv_obj_set_style_clip_corner(eye, true, 0);
	pupil = box(eye, 0, 0, PupilD, PupilD, LV_RADIUS_CIRCLE);
	lv_obj_set_style_bg_color(pupil, prim, 0);
	lid = box(eye, -2, -2, EyeD, 0, 0);
	lv_obj_set_style_bg_color(lid, sec, 0);
}

void RobotElement::reset(){
	t0 = millis();
	nextBlink = t0 + randomBetween(1500, 4000);
	nextGlance = t0 + randomBetween(1200, 3000);
	nextWave = t0 + randomBetween(6000, 12000);
	nextFlash = t0 + randomBetween(2000, 5000);
	blinkUntil = waveStart = flashUntil = 0;
	glanceX = glanceY = 0;
	pupilX = pupilY = 0;
	lastRigX = 0;
	lastWave = -1;
	loop();
}

void RobotElement::loop(){
	const uint32_t now = millis();
	const float t = (float) (now - t0);

	// Roll to and fro; the treads move with it
	const int32_t rigX = (int32_t) lroundf(sinf(t * 2.0f * (float) M_PI / DriftPeriodMs) * DriftPx);
	lv_obj_align(rig, LV_ALIGN_CENTER, rigX, 0);
	treadPhase += (float) (rigX - lastRigX);
	lastRigX = rigX;
	for(int i = 0; i < 4; ++i){
		float x = fmodf(treadPhase + (float) i * 14.0f, 56.0f);
		if(x < 0) x += 56.0f;
		lv_obj_set_x(treads[i], (int32_t) x - 1);
	}

	// Bob
	lv_obj_set_y(upper, (int32_t) lroundf(sinf(t * 2.0f * (float) M_PI / BobPeriodMs) * BobPx));

	// Glance somewhere now and then; the pupil eases there
	if(now >= nextGlance){
		const bool centre = esp_random() % 3 == 0;
		glanceX = centre ? 0 : (int8_t) ((int32_t) randomBetween(0, 10) - 5);
		glanceY = centre ? 0 : (int8_t) ((int32_t) randomBetween(0, 6) - 3);
		nextGlance = now + randomBetween(1500, 4500);
	}
	pupilX += ((float) glanceX - pupilX) * 0.2f;
	pupilY += ((float) glanceY - pupilY) * 0.2f;
	const int32_t mid = (EyeD - 4 - PupilD) / 2;
	lv_obj_set_pos(pupil, mid + (int32_t) lroundf(pupilX), mid + (int32_t) lroundf(pupilY));

	// Blink (sometimes twice)
	if(now >= nextBlink){
		blinkUntil = now + BlinkMs;
		nextBlink = now + (esp_random() % 5 == 0 ? BlinkMs * 2 : randomBetween(2500, 6000));
	}
	if(now < blinkUntil){
		const float k = 1.0f - fabsf((float) (blinkUntil - now) / BlinkMs * 2.0f - 1.0f); // 0 -> 1 -> 0
		lv_obj_set_height(lid, (int32_t) lroundf(k * EyeD));
	}else{
		lv_obj_set_height(lid, 0);
	}

	// The light on its head flashes now and then
	if(now >= nextFlash){
		flashUntil = now + FlashMs;
		nextFlash = now + randomBetween(2500, 7000);
	}
	lv_obj_set_style_bg_opa(light, now < flashUntil ? LV_OPA_COVER : LV_OPA_30, 0);

	// Now and then the right arm swings out and waves (rotation only while it waves: the rest of the time the arms
	// are drawn plainly, which is cheaper)
	int32_t wave = 0;
	if(now >= nextWave && waveStart == 0) waveStart = now;
	if(waveStart != 0){
		const float k = (float) (now - waveStart) / WaveMs;
		if(k >= 1.0f){
			waveStart = 0;
			nextWave = now + randomBetween(12000, 25000);
		}else{
			// Swing out to ~130 degrees, wag three times, back down (negative = outwards for the right arm)
			const float raise = sinf(k * (float) M_PI);
			const float wag = sinf(k * 6.0f * (float) M_PI) * 180.0f;
			wave = (int32_t) (raise * (-1300.0f + wag));
		}
	}
	if(wave != lastWave){
		lv_obj_set_style_transform_rotation(armR, wave, 0);
		lastWave = wave;
	}
	(void) armL;
}
