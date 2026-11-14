#include "StatusScreen.h"

#include <Core/Application.h>
#include <Fonts/font.hpp>
#include <LV_Interface/LVGL.h>
#include <Periph/Joystick.h>
#include <BBData.h>

#include "HomeScreen.h"
#include "PairingScreen.h"

StatusScreen::StatusScreen(){
	const auto app = Application::getApp();
	theme = app->getService<ThemeService>();
	buttonInput = app->getService<ButtonInput>();
	com = app->getService<Com>();
	LVGL* lvgl = app->getService<LVGL>();

	theme->activateThemeAssets();

	buttonInput->OnButtonEvent.bind(lvgl, [this](Enum<int> btn, ButtonInput::Action action) {
		handleButtonEvent((Button)(int)btn, action);
	});
	com->onData.bind(lvgl, [this](const BB::State state, const BB::Action action, const std::vector<uint8_t>& data) {
		handleDataEvent(state, action, data);
	});

	const lv_color_t colorPrim = theme->getPrimaryColor();
	lv_obj_set_style_bg_color(*this, theme->getTertiaryColor(), 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);

	topBar = new TopBar(*this);
	topBar->setRobotElementsVisible(com->getStatus() == Com::ConnStatus::Connected);

	list = lv_obj_create(*this);
	lv_obj_set_pos(list, 0, 9);
	lv_obj_set_size(list, 128, 119);
	lv_obj_set_style_pad_all(list, 3, 0);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

	lv_style_set_text_font(textStyle, &lv_font_butter);
	lv_style_set_text_color(textStyle, colorPrim);
	lv_style_set_text_line_space(textStyle, 3);

	body = lv_label_create(list);
	lv_obj_add_style(body, textStyle, 0);
	lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(body, 122);

	show("ROBOT STATUS\n\nAsking the robot...");
	openedAt = millis();
	ask();
}

StatusScreen::~StatusScreen(){
	const auto app = Application::getApp();
	LVGL* lvgl = app->getService<LVGL>();
	buttonInput->OnButtonEvent.unbind(lvgl);
	com->onData.unbind(lvgl);
}

void StatusScreen::show(const std::string& t){
	text = t;
	lv_label_set_text_static(body, text.c_str());
}

void StatusScreen::ask(){
	lastAsk = millis();
	if(com->getStatus() == Com::ConnStatus::Connected){
		com->sendCommand(Ctrl::Status);
	}
}

void StatusScreen::loop(){
	topBar->loop();

	const uint32_t now = millis();
	if(now - lastAsk >= RefreshMs){
		ask();
	}
	if(com->getStatus() != Com::ConnStatus::Connected){
		if(text.rfind("ROBOT STATUS\n\nNot connected", 0) != 0){
			show("ROBOT STATUS\n\nNot connected to the robot.\n\nOverride: back");
		}
	}else if(!everReplied && now - openedAt > NoReplyMs){
		if(text.rfind("ROBOT STATUS\n\nNo answer", 0) != 0){
			show("ROBOT STATUS\n\nNo answer - the robot may need the latest fork firmware.\n\nOverride: back");
		}
	}

	// Stick up / down scrolls
	if(Joystick* joy = Application::getApp()->getDevice<Joystick>()){
		const int v = joy->getVertical();
		if(v > 40 || v < -40){
			lv_obj_scroll_by_bounded(list, 0, v > 0 ? 3 : -3, LV_ANIM_OFF);
		}
	}
}

static std::string problemText(const std::string& p){
	if(p.empty() || p == "none") return "OK";
	std::string out = p;
	for(char& c : out) if(c == '_') c = ' ';
	return out;
}

void StatusScreen::handleDataEvent(const BB::State state, const BB::Action action, const std::vector<uint8_t>& data){
	if(state != BB::State::Idle || action.idle != BB::Action::Idle::RobotStatus){
		return;
	}
	const RobotStatusData s = RobotStatusData::deserialize(data.data(), data.size());
	everReplied = true;
	lastReply = millis();

	static constexpr const char* Charge[] = { "", " charging", " full" };
	std::string t = "ROBOT STATUS\n\n";
	t += "Battery: " + std::to_string(s.battery) + "%" + Charge[s.charge < 3 ? s.charge : 0] + "\n";
	t += "Memory: " + std::to_string(s.ramKb) + " KB (low " + std::to_string(s.ramMinKb) + ")\n";
	t += "WiFi: " + (s.wifi ? (s.network.empty() ? std::string("set up") : s.network) : std::string("not set up")) + "\n";
	if(!s.ai){
		t += "AI mode: off\n";
	}else{
		t += std::string("AI mode: on") + (s.brain ? "" : ", no brain yet") + "\n";
		t += "Last try: " + problemText(s.problem) + "\n";
	}
	if(!s.heard.empty()){
		t += "\nYou: " + s.heard + "\n";
	}
	if(!s.said.empty()){
		t += "Bot: " + s.said + "\n";
	}
	const unsigned h = s.upMin / 60, m = s.upMin % 60;
	t += "\nAwake: " + (h ? std::to_string(h) + "h " : std::string()) + std::to_string(m) + "m\n";
	t += "Firmware: " + s.fw + "\n\nOverride: back";
	show(t);
}

void StatusScreen::handleButtonEvent(const Button btn, const ButtonInput::Action action){
	if(!buttonGate.accept(btn, action)){
		return;
	}
	if((btn == Button::ManualOverride || btn == Button::ShutUp) && action == ButtonInput::Action::Release){
		if(com->getStatus() != Com::ConnStatus::Connected){
			transition([]() { return std::make_unique<PairingScreen>(); });
			return;
		}
		transition([]() { return std::make_unique<HomeScreen>(); });
	}
}
