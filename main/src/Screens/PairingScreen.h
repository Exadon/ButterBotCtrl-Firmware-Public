#ifndef BUTTERBOTCTRL_FIRMWARE_PAIRINGSCREEN_H
#define BUTTERBOTCTRL_FIRMWARE_PAIRINGSCREEN_H

#include "Util/ButtonGate.h"
#include <LV_Interface/LVScreen.h>
#include <Services/ButtonInput.h>
#include "Enums.hpp"
#include "BLE/GAP.h"
#include "Services/Com.h"
#include "Services/LEDController.h"

class PairingScreen : public LVScreen {
public:
	PairingScreen();
	~PairingScreen() override;

private:
	BLE::GAP* gap;
	Com* com;
	LEDController* ledController;
	ButtonInput* buttonInput;

	void loop() override;

	void buildSettingsHint();

	void handleButtonEvent(Button btn, ButtonInput::Action action);
	ButtonGate buttonGate; // ignores releases of buttons pressed before this screen opened

	// Override + Shut Up held together also opens Settings here (same chord as the home screen)
	static constexpr uint64_t SettingsChordMs = 800;
	bool overrideDown = false;
	bool shutUpDown = false;
	uint64_t chordStartTime = 0;
};


#endif //BUTTERBOTCTRL_FIRMWARE_PAIRINGSCREEN_H
