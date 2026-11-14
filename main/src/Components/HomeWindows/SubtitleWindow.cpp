#include "SubtitleWindow.h"

#include "Fonts/font.hpp"

SubtitleWindow::SubtitleWindow(lv_obj_t* parent, const SubtitleData* params) : HomeWindow(parent, TITLE){
	lv_style_set_text_font(heardStyle, &lv_font_butter);
	lv_style_set_text_color(heardStyle, theme->getSecondaryColor());
	lv_style_set_text_line_space(heardStyle, 2);
	lv_style_set_pad_all(heardStyle, 2);
	lv_style_set_bg_opa(heardStyle, LV_OPA_TRANSP);

	heardLabel = lv_label_create(innerContent);
	lv_obj_add_style(heardLabel, heardStyle, 0);
	lv_label_set_long_mode(heardLabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_size(heardLabel, 100, LV_SIZE_CONTENT);

	sayLabel = lv_label_create(innerContent);
	lv_obj_add_style(sayLabel, labelDefaultStyle, 0);
	lv_label_set_long_mode(sayLabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_size(sayLabel, 100, LV_SIZE_CONTENT);

	buildUI(*params);
}

void SubtitleWindow::onData(const BBData* data){
	buildUI(*static_cast<const SubtitleData*>(data));
}

void SubtitleWindow::buildUI(const SubtitleData& data){
	heardText = data.heard.empty() ? std::string() : "You: " + data.heard;
	sayText = data.say;
	lv_label_set_text_static(heardLabel, heardText.c_str());
	lv_label_set_text_static(sayLabel, sayText.c_str());
	if(heardText.empty()){
		lv_obj_add_flag(heardLabel, LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_obj_remove_flag(heardLabel, LV_OBJ_FLAG_HIDDEN);
	}
	updateLayout();
}
