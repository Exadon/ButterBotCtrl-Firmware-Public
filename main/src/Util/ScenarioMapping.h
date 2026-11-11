#ifndef BUTTERBOTCTRL_FIRMWARE_SCENARIOMAPPING_H
#define BUTTERBOTCTRL_FIRMWARE_SCENARIOMAPPING_H

#include <span>
#include <utility>
#include <BBData.h>
#include <Scenarios.h>

struct Action {
	BB::Action::Scenario scenario;
	ScenarioData data;
};

using ActionEntry = std::pair<const char*, Action>;

/**
 * The action list (joystick press on the home screen) is grouped into folders.
 * Folder and entry declaration order is the display order.
 * The Chat folder has no static entries: it lists the Chat activations from Scenarios.h at runtime
 * (title = what you say, second line = the reply), so it always matches the robot's chat lines.
 */
struct ActionFolder {
	const char* name;
	std::span<const ActionEntry> entries;
	bool isChat = false;
};

namespace ActionFolderEntries {

constexpr ActionEntry Fun[] = {
	{ "Fact", { BB::Action::Scenario::Fact, {} } },
	{ "Joke", { BB::Action::Scenario::Joke, {} } },
	{ "Pass The Butter", { BB::Action::Scenario::PassTheButter, {} } },
	{ "You Pass The Butter", { BB::Action::Scenario::YouPassButter, {} } },
	{ "Eight Ball", { BB::Action::Scenario::EightBall, {} } },
	{ "Dance", { BB::Action::Scenario::Dance, {} } },
	{ "Open a Portal", { BB::Action::Scenario::OpenPortal, {} } },
	{ "What is This?", { BB::Action::Scenario::WhatsThis, {} } },
	{ "Current Time", { BB::Action::Scenario::CurrentTime, {} } },
};

constexpr ActionEntry Dice[] = {
	{ "Dice Roll", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::None } } },
	{ "Roll a D4", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D4 } } },
	{ "Roll a D6", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D6 } } },
	{ "Roll a D8", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D8 } } },
	{ "Roll a D10", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D10 } } },
	{ "Roll a D12", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D12 } } },
	{ "Roll a D20", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D20 } } },
	{ "Roll a D100", { BB::Action::Scenario::DiceRoll, DiceScenarioData{ DiceScenarioData::DiceType::D100 } } },
};

constexpr ActionEntry Movement[] = {
	{ "Go Forward", { BB::Action::Scenario::VoiceControl, VoiceScenarioData{ VoiceScenarioData::Direction::Forward } } },
	{ "Go Backward", { BB::Action::Scenario::VoiceControl, VoiceScenarioData{ VoiceScenarioData::Direction::Backward } } },
	{ "Turn Left", { BB::Action::Scenario::VoiceControl, VoiceScenarioData{ VoiceScenarioData::Direction::Left } } },
	{ "Turn Right", { BB::Action::Scenario::VoiceControl, VoiceScenarioData{ VoiceScenarioData::Direction::Right } } },
	{ "Turn Around", { BB::Action::Scenario::VoiceControl, VoiceScenarioData{ VoiceScenarioData::Direction::Rotate } } },
};

constexpr ActionEntry Faces[] = {
	{ "Who is this?", { BB::Action::Scenario::FaceDetect, FaceScenarioData{ FaceScenarioData::Phase::Detect } } },
	{ "Remember this face", { BB::Action::Scenario::FaceDetect, FaceScenarioData{ FaceScenarioData::Phase::Remember } } },
	{ "Forget Owner", { BB::Action::Scenario::FaceDetectForget, FaceScenarioData{ FaceScenarioData::Phase::Forget } } },
};

constexpr ActionEntry Phone[] = {
	{ "Phone List Notifications", { BB::Action::Scenario::PhoneListNotifs, {} } },
	{ "Phone What is Playing?", { BB::Action::Scenario::PhoneWhatsPlaying, {} } },
	{ "Phone Next Song", { BB::Action::Scenario::PhoneNextSong, {} } },
	{ "Phone Previous Song", { BB::Action::Scenario::PhonePrevSong, {} } },
	{ "Phone Play Music", { BB::Action::Scenario::PhonePlayMusic, {} } },
	{ "Phone Stop Music", { BB::Action::Scenario::PhoneStopMusic, {} } },
};

constexpr ActionEntry IR[] = {
	{ "IR train", { BB::Action::Scenario::IR_train, {} } },
	{ "IR list", { BB::Action::Scenario::IR_list, {} } },
	{ "IR forget", { BB::Action::Scenario::IR_forget, {} } },
	{ "IR forget all", { BB::Action::Scenario::IR_forgetAll, {} } },
};

constexpr ActionEntry LED[] = {
	{ "LED Turn On", { BB::Action::Scenario::LEDTurnOn, {} } },
	{ "LED Turn Off", { BB::Action::Scenario::LEDTurnOff, {} } },
	{ "LED Strobe", { BB::Action::Scenario::LEDStrobe, {} } },
	{ "LED Breathe", { BB::Action::Scenario::LEDBreathe, {} } },
	{ "LED Faster", { BB::Action::Scenario::LEDFaster, {} } },
	{ "LED Slower", { BB::Action::Scenario::LEDSlower, {} } },
};

constexpr ActionEntry Climate[] = {
	{ "Climate Module", { BB::Action::Scenario::TempHumModule, {} } },
	{ "Climate Scale Celsius", { BB::Action::Scenario::TempHumScaleCelsius, {} } },
	{ "Climate Scale Fahrenheit", { BB::Action::Scenario::TempHumScaleFahrenheit, {} } },
	{ "Climate Scale Kelvin", { BB::Action::Scenario::TempHumScaleKelvin, {} } },
};

constexpr ActionEntry Sensors[] = {
	{ "Gas Module", { BB::Action::Scenario::GasModule, {} } },
	{ "Intruder Detection ON", { BB::Action::Scenario::IntruderDetectionOn, {} } },
	{ "Intruder Detection OFF", { BB::Action::Scenario::IntruderDetectionOff, {} } },
};

constexpr ActionEntry Robot[] = {
	{ "Activity Mode", { BB::Action::Scenario::SettingsActivity, ActivityScenarioData{ ActivityScenarioData::Level::None } } },
	{ "Activity Off", { BB::Action::Scenario::SettingsActivity, ActivityScenarioData{ ActivityScenarioData::Level::Off } } },
	{ "Activity Chill", { BB::Action::Scenario::SettingsActivity, ActivityScenarioData{ ActivityScenarioData::Level::Chill } } },
	{ "Activity Active", { BB::Action::Scenario::SettingsActivity, ActivityScenarioData{ ActivityScenarioData::Level::Active } } },
	{ "Activity Hyper", { BB::Action::Scenario::SettingsActivity, ActivityScenarioData{ ActivityScenarioData::Level::Hyper } } },
	{ "Shutdown", { BB::Action::Scenario::Shutdown, {} } },
};

}

constexpr ActionFolder ActionFolders[] = {
	{ "Fun", ActionFolderEntries::Fun },
	{ "Chat", {}, true },
	{ "Dice", ActionFolderEntries::Dice },
	{ "Movement", ActionFolderEntries::Movement },
	{ "Faces", ActionFolderEntries::Faces },
	{ "Phone", ActionFolderEntries::Phone },
	{ "IR Remote", ActionFolderEntries::IR },
	{ "LED Module", ActionFolderEntries::LED },
	{ "Climate Module", ActionFolderEntries::Climate },
	{ "Sensors", ActionFolderEntries::Sensors },
	{ "Robot", ActionFolderEntries::Robot },
};

#endif //BUTTERBOTCTRL_FIRMWARE_SCENARIOMAPPING_H
