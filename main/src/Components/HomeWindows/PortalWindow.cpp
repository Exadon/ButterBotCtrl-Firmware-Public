#include "PortalWindow.h"
#include <cmath>
#include <Util/stdafx.h>

namespace {
	constexpr uint32_t RingColors[] = { 0x39ff6a, 0xb6ff3b, 0x1fd6b0 };
	constexpr uint32_t DotColor = 0xb6ff3b;
	constexpr float Pi = 3.14159265f;

	lv_obj_t* makeCircle(lv_obj_t* parent){
		lv_obj_t* c = lv_obj_create(parent);
		lv_obj_remove_style_all(c);
		lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
		lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
		return c;
	}

	void placeCentered(lv_obj_t* obj, int32_t cx, int32_t cy, int32_t d){
		lv_obj_set_size(obj, d, d);
		lv_obj_set_pos(obj, cx - d / 2, cy - d / 2);
	}
}

PortalWindow::PortalWindow(lv_obj_t* parent, const OpenPortalData* params) : HomeWindow(parent, TITLE), startMs(millis()){
	stage = lv_obj_create(innerContent);
	lv_obj_remove_style_all(stage);
	lv_obj_set_size(stage, StageSize, StageSize);
	lv_obj_set_style_bg_color(stage, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(stage, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(stage, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_clip_corner(stage, true, 0);
	lv_obj_remove_flag(stage, LV_OBJ_FLAG_SCROLLABLE);

	for(size_t i = 0; i < RingCount; ++i){
		rings[i] = makeCircle(stage);
		lv_obj_set_style_border_width(rings[i], 3 - static_cast<int32_t>(i), 0);
		lv_obj_set_style_border_color(rings[i], lv_color_hex(RingColors[i]), 0);
		lv_obj_set_style_border_opa(rings[i], LV_OPA_COVER, 0);
	}
	for(lv_obj_t*& dot : dots){
		dot = makeCircle(stage);
		lv_obj_set_style_bg_color(dot, lv_color_hex(DotColor), 0);
		lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
	}
	core = makeCircle(stage);
	lv_obj_set_style_bg_color(core, lv_color_hex(0xeaffef), 0);
	lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);
	lv_obj_set_style_shadow_color(core, lv_color_hex(RingColors[0]), 0);
	lv_obj_set_style_shadow_width(core, 10, 0);
	lv_obj_set_style_shadow_opa(core, LV_OPA_COVER, 0);

	loop(); // lay out the first frame before the window becomes visible
	updateLayout();
}

void PortalWindow::loop(){
	const float t = static_cast<float>(millis() - startMs) / 1000.0f;
	const int32_t c = StageSize / 2;

	// Rings breathe out of phase, so the portal looks like it's churning
	for(size_t i = 0; i < RingCount; ++i){
		const float base = 54.0f - 14.0f * static_cast<float>(i);
		const float d = base + 4.0f * std::sin(t * 6.0f + static_cast<float>(i) * 2.1f);
		placeCentered(rings[i], c, c, static_cast<int32_t>(d));
	}

	// Particles orbit and spiral into the core, then respawn at the rim
	for(size_t i = 0; i < DotCount; ++i){
		const float phase = static_cast<float>(i) / static_cast<float>(DotCount);
		const float life = std::fmod(t * 0.9f + phase, 1.0f);            // 0 = rim, 1 = core
		const float radius = 26.0f * (1.0f - life);
		const float angle = t * 5.0f + phase * 2.0f * Pi + life * 4.0f;  // tighter twist near the centre
		const int32_t x = c + static_cast<int32_t>(std::cos(angle) * radius);
		const int32_t y = c + static_cast<int32_t>(std::sin(angle) * radius);
		placeCentered(dots[i], x, y, life > 0.8f ? 2 : 3);
	}

	const int32_t coreD = 10 + static_cast<int32_t>(3.0f * std::sin(t * 10.0f));
	placeCentered(core, c, c, coreD);
}
