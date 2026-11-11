#include "IntroScreen.h"
#include "PairingScreen.h"
#include "Services/ThemeService.h"
#include "Fonts/font.hpp"
#include <esp_app_desc.h>

namespace {
	constexpr uint32_t PortalGreen = 0x39ff6a;
	constexpr uint32_t PortalLime = 0xb6ff3b;
	constexpr uint32_t VersionGreen = 0x7dbf8a;

	// A filled or outlined circle, centered in its parent - the building block of the portal mark
	lv_obj_t* circle(lv_obj_t* parent, int32_t diameter, uint32_t color, int32_t border){
		lv_obj_t* c = lv_obj_create(parent);
		lv_obj_remove_style_all(c);
		lv_obj_set_size(c, diameter, diameter);
		lv_obj_center(c);
		lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
		if(border > 0){
			lv_obj_set_style_border_width(c, border, 0);
			lv_obj_set_style_border_color(c, lv_color_hex(color), 0);
			lv_obj_set_style_border_opa(c, LV_OPA_COVER, 0);
		}else{
			lv_obj_set_style_bg_color(c, lv_color_hex(color), 0);
			lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
		}
		lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
		return c;
	}

	lv_obj_t* label(lv_obj_t* parent, const char* text, uint32_t color){
		lv_obj_t* l = lv_label_create(parent);
		lv_obj_set_style_text_font(l, &lv_font_butter, 0);
		lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
		lv_label_set_text(l, text);
		return l;
	}
}

void IntroScreen::buildForkBadge(){
	lv_obj_t* badge = lv_obj_create(*this);
	lv_obj_remove_style_all(badge);
	lv_obj_set_size(badge, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(badge, 4, 0);
	lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(badge, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_align(badge, LV_ALIGN_BOTTOM_MID, 0, -4);

	// Portal mark: outer ring, inner ring, bright core
	lv_obj_t* portal = lv_obj_create(badge);
	lv_obj_remove_style_all(portal);
	lv_obj_set_size(portal, 13, 13);
	lv_obj_remove_flag(portal, LV_OBJ_FLAG_SCROLLABLE);
	circle(portal, 13, PortalGreen, 2);
	circle(portal, 7, PortalLime, 1);
	circle(portal, 3, 0xeaffef, 0);

	lv_obj_t* text = lv_obj_create(badge);
	lv_obj_remove_style_all(text);
	lv_obj_set_size(text, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(text, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_row(text, 2, 0);
	lv_obj_remove_flag(text, LV_OBJ_FLAG_SCROLLABLE);
	label(text, "EXADON FORK", PortalGreen);
	label(text, esp_app_get_description()->version, VersionGreen);

	// Let the stock animation start on its own, then fade the badge in underneath it
	lv_obj_fade_in(badge, 400, 500);
}

IntroScreen::IntroScreen(){
	const auto app = Application::getApp();

	// Load the intro GIF archive
	app->getService<ThemeService>()->activateIntroAssets();

	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);

	introGif = new LVGIF(*this, "S:/intro");
	introGif->setLoopCallback([this]() {
		// Stop reading GIF frames before fade transition
		lv_obj_add_flag(*introGif, LV_OBJ_FLAG_HIDDEN);
		transition([](){ return std::make_unique<PairingScreen>(); }, LV_SCR_LOAD_ANIM_FADE_IN);
	});
	introGif->setLooping(LVGIF::LoopType::Single);
	introGif->reset();

	buildForkBadge(); // created after the GIF so it draws on top of it
}
