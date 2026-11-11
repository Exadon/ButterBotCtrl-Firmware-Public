#ifndef BUTTERBOTCTRL_FIRMWARE_BUTTONGATE_H
#define BUTTERBOTCTRL_FIRMWARE_BUTTONGATE_H

#include <cstdint>
#include <Services/ButtonInput.h>
#include "Enums.hpp"

/**
 * Drops button releases whose press this screen never saw.
 *
 * Screens are swapped while buttons are still held (e.g. Override + Shut Up opens Settings mid-hold), and the
 * new screen then gets those releases. Several screens act on release (Settings leaves on Override/Shut Up
 * release, Home mutes on Shut Up release and pokes on Poke release), so a stray release used to fire an action
 * the user never asked for - Settings closed the moment the chord's buttons were let go.
 * Call accept() first in a screen's button handler and ignore the event when it returns false.
 */
class ButtonGate {
public:
	bool accept(Button btn, ButtonInput::Action action){
		const uint32_t bit = 1u << static_cast<uint32_t>(btn);
		if(action == ButtonInput::Action::Press){
			pressed |= bit;
			return true;
		}
		const bool seen = (pressed & bit) != 0;
		pressed &= ~bit;
		return seen;
	}

private:
	uint32_t pressed = 0;
};

#endif //BUTTERBOTCTRL_FIRMWARE_BUTTONGATE_H
