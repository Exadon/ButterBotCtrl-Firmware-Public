#include "ActionElement.h"

#include <algorithm>
#include <cstring>
#include <Core/Application.h>
#include <Phrases.h>
#include <Scenarios.h>

#include "Fonts/font.hpp"

ActionElement::ActionElement(lv_obj_t* parent, lv_group_t* inputGroup, std::function<void(BB::Action::Scenario, ScenarioData)> scenarioCb,
                             const ActionListState initial) : LVObject(parent), inputGroup(inputGroup), scenarioCb(std::move(scenarioCb)){
	theme = Application::getApp()->getService<ThemeService>();

	buildStyles();

	const int16_t folder = initial.folder < static_cast<int16_t>(std::size(ActionFolders)) ? initial.folder : -1;
	show(folder, initial.index);
}

ActionElement::~ActionElement(){
	lv_async_call_cancel(&ActionElement::asyncShow, this);
}

ActionListState ActionElement::getState() const{
	return { currentFolder, elIndex };
}

void ActionElement::back(){
	if(currentFolder >= 0){
		requestShow(-1, static_cast<uint16_t>(currentFolder));
	}
}

void ActionElement::requestShow(int16_t folder, uint16_t focusIndex){
	pendingFolder = folder;
	pendingIndex = focusIndex;
	lv_async_call_cancel(&ActionElement::asyncShow, this);
	lv_async_call(&ActionElement::asyncShow, this);
}

void ActionElement::asyncShow(void* self){
	auto* el = static_cast<ActionElement*>(self);
	el->show(el->pendingFolder, el->pendingIndex);
}

void ActionElement::buildStyles(){
	const lv_color_t colorPrim = theme->getPrimaryColor();
	const lv_color_t colorSecond = theme->getSecondaryColor();
	const lv_color_t colorTert = theme->getTertiaryColor();

	// LIST CONTAINER
	lv_obj_set_size(*this, Width, Height);
	lv_obj_set_layout(*this, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(*this, 2, 0);
	lv_obj_set_style_pad_gap(*this, 2, 0);
	lv_obj_set_style_bg_color(*this, colorTert, 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
	lv_obj_add_flag(*this, LV_OBJ_FLAG_SCROLLABLE); // LVObject ctor clears it
	lv_obj_set_scroll_dir(*this, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(*this, LV_SCROLLBAR_MODE_OFF);

	// BUTTON STYLES
	lv_style_set_pad_ver(buttonStyle, 2);
	lv_style_set_pad_row(buttonStyle, 1);
	lv_style_set_radius(buttonStyle, 0);
	lv_style_set_border_width(buttonStyle, 0);
	lv_style_set_bg_opa(buttonStyle, LV_OPA_TRANSP);

	lv_style_set_border_width(buttonFocusedStyle, 1);
	lv_style_set_border_color(buttonFocusedStyle, colorPrim);

	// LABEL STYLE
	lv_style_set_text_font(labelStyle, &lv_font_butter);
	lv_style_set_text_color(labelStyle, colorPrim);
	lv_style_set_width(labelStyle, lv_pct(100));
	lv_style_set_bg_opa(labelStyle, LV_OPA_TRANSP);

	// SUB-LABEL STYLE
	lv_style_set_text_font(subLabelStyle, &lv_font_butter);
	lv_style_set_text_color(subLabelStyle, colorSecond);
	lv_style_set_width(subLabelStyle, lv_pct(100));
	lv_style_set_bg_opa(subLabelStyle, LV_OPA_TRANSP);
}

size_t ActionElement::folderSize(const ActionFolder& folder){
	if(!folder.isChat){
		return folder.entries.size();
	}
	return static_cast<size_t>(std::count_if(std::begin(activations), std::end(activations),
	                                         [](const ScenarioActivation& a){ return a.scenario == BB::Action::Scenario::Chat; }));
}

void ActionElement::show(int16_t folder, uint16_t focusIndex){
	// Deleting the rows removes them from inputGroup too; texts go after their labels are gone
	lv_obj_clean(*this);
	buttons.clear();
	rowActions.clear();
	phraseTexts.clear();
	currentFolder = folder;

	if(folder < 0){
		// Folder list
		phraseTexts.reserve(std::size(ActionFolders));
		buttons.reserve(std::size(ActionFolders));
		for(const ActionFolder& f : ActionFolders){
			phraseTexts.push_back(std::to_string(folderSize(f)) + (f.isChat ? " chat lines" : " actions"));
			addButton(f.name, phraseTexts.back().c_str());
		}
	}else if(ActionFolders[folder].isChat){
		// Chat lines straight from the activations: say this -> the robot replies that
		const size_t n = folderSize(ActionFolders[folder]);
		phraseTexts.reserve(n);
		buttons.reserve(n);
		rowActions.reserve(n);
		for(const ScenarioActivation& act : activations){
			if(act.scenario != BB::Action::Scenario::Chat) continue;
			phraseTexts.push_back(Phrases::mapShown(Phrase::Chat, act.scenarioData.raw));
			rowActions.push_back({ act.scenario, act.scenarioData });
			addButton(act.string, phraseTexts.back().c_str());
		}
	}else{
		const std::span<const ActionEntry> entries = ActionFolders[folder].entries;
		phraseTexts.reserve(entries.size());
		buttons.reserve(entries.size());
		rowActions.reserve(entries.size());
		for(const auto& [name, action] : entries){
			phraseTexts.push_back(collectPhrases(action.scenario, action.data));
			rowActions.push_back(action);
			addButton(name, phraseTexts.back().c_str());
		}
	}

	if(buttons.empty()){
		return;
	}

	elIndex = std::min<uint16_t>(focusIndex, buttons.size() - 1);
	lv_obj_update_layout(*this);
	lv_group_focus_obj(buttons[elIndex]);
	// SCROLL_ON_FOCUS would animate the jump to the restored entry; snap instead
	lv_obj_scroll_to_view(buttons[elIndex], LV_ANIM_OFF);
}

lv_obj_t* ActionElement::addButton(const char* title, const char* subtitle){
	lv_obj_t* btn = lv_obj_create(*this);
	lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
	lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_add_style(btn, buttonStyle, 0);
	lv_obj_add_style(btn, buttonFocusedStyle, LV_STATE_FOCUSED);

	lv_obj_t* titleLabel = lv_label_create(btn);
	lv_obj_add_style(titleLabel, labelStyle, 0);
	lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_CLIP);
	lv_label_set_text_static(titleLabel, title);
	lv_obj_set_style_pad_hor(titleLabel, 2, 0);

	if(subtitle != nullptr && subtitle[0] != '\0'){
		lv_obj_t* phraseLabel = lv_label_create(btn);
		lv_obj_add_style(phraseLabel, subLabelStyle, 0);
		lv_label_set_long_mode(phraseLabel, LV_LABEL_LONG_CLIP);
		lv_label_set_text_static(phraseLabel, subtitle);
		lv_obj_set_style_pad_hor(phraseLabel, 2, 0);

		// Normalize text scroll speed and bypass LVGL 10s scroll anim cap
		const int32_t scrollDist = lv_obj_get_self_width(phraseLabel) + lv_font_get_glyph_width(&lv_font_butter, ' ', ' ') * LV_LABEL_WAIT_CHAR_COUNT;
		lv_obj_set_style_anim_duration(phraseLabel, scrollDist * 1000 / ScrollSpeedPxPerSec, 0);

		// The second line is clipped while idle and marquee-scrolls only while focused
		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			const auto label = (lv_obj_t*)lv_event_get_user_data(e);
			lv_obj_set_style_pad_hor(label, 0, 0);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
		}, LV_EVENT_FOCUSED, phraseLabel);

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			const auto label = (lv_obj_t*)lv_event_get_user_data(e);
			lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
			lv_obj_set_style_pad_hor(label, 2, 0);
		}, LV_EVENT_DEFOCUSED, phraseLabel);
	}

	lv_obj_add_event_cb(btn, [](lv_event_t* e) {
		const auto el = (ActionElement*)lv_event_get_user_data(e);
		const uint32_t key = lv_event_get_key(e);
		if(key == LV_KEY_DOWN){
			el->moveFocus(1);
		} else if(key == LV_KEY_UP){
			el->moveFocus(-1);
		}
	}, LV_EVENT_KEY, this);

	lv_obj_add_event_cb(btn, [](lv_event_t* e) {
		const auto el = (ActionElement*)lv_event_get_user_data(e);
		el->onItemClicked(lv_event_get_target_obj(e));
	}, LV_EVENT_CLICKED, this);

	// Keep elIndex in sync with the actually focused button for moveFocus()
	lv_obj_add_event_cb(btn, [](lv_event_t* e) {
		const auto el = (ActionElement*)lv_event_get_user_data(e);
		el->elIndex = lv_obj_get_index(lv_event_get_target_obj(e));
	}, LV_EVENT_FOCUSED, this);

	lv_group_add_obj(inputGroup, btn);
	buttons.push_back(btn);
	return btn;
}

void ActionElement::moveFocus(const int8_t dir){
	const auto n = (uint16_t)buttons.size();
	if(n == 0) return;
	elIndex = (elIndex + n + dir) % n;
	lv_group_focus_obj(buttons[elIndex]);
}

void ActionElement::onItemClicked(const lv_obj_t* btn){
	const int32_t index = lv_obj_get_index(btn);
	if(index < 0) return;

	if(currentFolder < 0){
		// Open the folder (deferred: we're inside this row's own event)
		if(index < static_cast<int32_t>(std::size(ActionFolders))){
			requestShow(static_cast<int16_t>(index), 0);
		}
		return;
	}

	if(scenarioCb && index < static_cast<int32_t>(rowActions.size())){
		const Action& action = rowActions[index];
		scenarioCb(action.scenario, action.data);
	}
}

std::string ActionElement::collectPhrases(const BB::Action::Scenario scenario, const ScenarioData data){
	std::string out;
	const char* prev = nullptr;
	for(const ScenarioActivation& act : activations){
		if(act.scenario != scenario || act.scenarioData != data) continue;
		// Some phrases repeat back-to-back with different phonemes ("roll a d twenty")
		if(prev != nullptr && strcmp(prev, act.string) == 0) continue;
		if(!out.empty()) out += " \\ ";
		out += act.string;
		prev = act.string;
	}
	return out;
}
