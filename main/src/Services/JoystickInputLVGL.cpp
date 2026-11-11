#include "JoystickInputLVGL.h"
#include <algorithm>
#include <cstdlib>

JoystickInputLVGL::JoystickInputLVGL(Joystick* joystick, ButtonInput* bi, Enum<int> enterButton) :
		Super(bi, {}, {}), joystick(joystick), bi(bi), enterButton(enterButton){
	lv_indev_set_type(getIndev(), LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_read_cb(getIndev(), [](lv_indev_t* drv, lv_indev_data_t* data){
		static_cast<JoystickInputLVGL*>(InputLVGL::getInstance())->readJoystick(data);
	});
}

void JoystickInputLVGL::postInitProperties() noexcept{
	bi->OnButtonEvent.bind(this, &JoystickInputLVGL::onButtonInput);
}

void JoystickInputLVGL::readJoystick(lv_indev_data_t* data){
	lv_key_t currentKey = static_cast<lv_key_t>(0);
	bool pressed = false;

	if(enterPressed.load()){
		currentKey = LV_KEY_ENTER;
		pressed = true;
	}else{
		currentKey = readDirection();
		pressed = currentKey != static_cast<lv_key_t>(0);
	}

	if(pressed && lastReportedPressed && lastReportedKey != currentKey){
		data->key = lastReportedKey;
		data->state = LV_INDEV_STATE_RELEASED;
		lastReportedPressed = false;
		return;
	}

	if(pressed){
		data->key = currentKey;
		data->state = LV_INDEV_STATE_PRESSED;
		lastReportedKey = currentKey;
		lastReportedPressed = true;
	}else{
		data->key = lastReportedKey;
		data->state = LV_INDEV_STATE_RELEASED;
		lastReportedPressed = false;
	}
}

lv_key_t JoystickInputLVGL::readDirection(){
	const int h = joystick->getHorizontal();
	const int v = joystick->getVertical();

	// Keep the latched direction while its own axis is still pushed the same way past the release threshold
	if(heldDir != static_cast<lv_key_t>(0)){
		const bool stillHeld =
				(heldDir == LV_KEY_RIGHT && h >= ReleaseThreshold) ||
				(heldDir == LV_KEY_LEFT && h <= -ReleaseThreshold) ||
				(heldDir == LV_KEY_UP && v >= ReleaseThreshold) ||
				(heldDir == LV_KEY_DOWN && v <= -ReleaseThreshold);
		if(stillHeld){
			return heldDir;
		}
		heldDir = static_cast<lv_key_t>(0);
	}

	// Nothing latched: take the dominant axis, once it's clearly pushed
	const int ah = std::abs(h);
	const int av = std::abs(v);
	if(std::max(ah, av) < PressThreshold){
		return heldDir;
	}

	if(ah >= av){
		heldDir = h > 0 ? LV_KEY_RIGHT : LV_KEY_LEFT;
	}else{
		heldDir = v > 0 ? LV_KEY_UP : LV_KEY_DOWN;
	}
	return heldDir;
}

void JoystickInputLVGL::onButtonInput(Enum<int> btn, ButtonInput::Action action){
	if(btn != enterButton){
		return;
	}

	enterPressed.store(action == ButtonInput::Action::Press);
}
