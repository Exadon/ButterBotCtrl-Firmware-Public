#ifndef BUTTERBOTCTRL_FIRMWARE_SETTINGS_H
#define BUTTERBOTCTRL_FIRMWARE_SETTINGS_H

#include "Object/Object.h"
#include "Object/Class.h"
#include <nvs.h>
#include <CtrlData.h>

#include "Themes.hpp"

enum class InactivityTimeout : uint8_t {
	Min2 = 2, Min5 = 5, Min10 = 10, Min30 = 30, Off = 0xFF
};

/** Fork: what the home screen shows when nothing else is on it (Settings > HOME). Robot is a marker byte rather than
 *  1: settings saved by older firmware are copied in byte by byte, and this field may land on their padding. */
enum class HomeView : uint8_t {
	Hints = 0, Robot = 0xB0
};

struct SettingsStruct {
	float screenBrightness = 1.0f;
	Theme currentTheme = Theme::Main;
	InactivityTimeout inactivityTimeout = InactivityTimeout::Min10;
	FrontSensorMode frontSensor = FrontSensorMode::On;
	BottomSensorMode bottomSensor = BottomSensorMode::Reduced;
	ActivityMode activityMode = ActivityMode::Active;
	HomeView homeView = HomeView::Hints; // fork: anything but Robot means the button hints
};

class Settings : public Object {
	GENERATED_BODY(Settings, Object, void)
public:
	Settings();
	~Settings() override;

	SettingsStruct get() const;
	void set(const SettingsStruct& settings);
	void store() const;

private:
	SettingsStruct settingsStruct;

	static constexpr const char* BlobName = "Settings";
	static constexpr const char* NVSNamespace = "ButterbotCtrl";

	nvs_handle_t handle{};

	void load();
};


#endif //BUTTERBOTCTRL_FIRMWARE_SETTINGS_H
