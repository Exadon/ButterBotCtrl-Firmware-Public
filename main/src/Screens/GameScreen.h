#ifndef BUTTERBOTCTRL_FIRMWARE_GAMESCREEN_H
#define BUTTERBOTCTRL_FIRMWARE_GAMESCREEN_H

#include <array>
#include <cstdint>
#include <LV_Interface/LVScreen.h>
#include <Services/ButtonInput.h>

#include "Enums.hpp"
#include "Util/ButtonGate.h"

/**
 * Fork: "Butter Run" - a portal at the top spits out butter and monsters; steer ButterBot along the bottom with
 * the stick, grab the butter, dodge the monsters. Three lives, it speeds up, best score is kept in NVS.
 * Drawn straight into a 128x128 RGB565 buffer shown as one image (like the RC camera feed).
 * Shut Up: start / pause, Override: leave.
 */
class GameScreen : public LVScreen {
public:
	GameScreen();
	~GameScreen() override;

private:
	static constexpr int W = 128;
	static constexpr int H = 128;
	static constexpr int GroundY = 118;      // top of the floor strip
	static constexpr uint32_t FrameMs = 33;  // ~30 fps
	static constexpr int MaxThings = 12;
	static constexpr int Lives = 3;

	enum class Phase : uint8_t { Title, Play, Paused, Over };
	enum class Kind : uint8_t { Butter, GoldButter, Monster };

	struct Thing {
		bool alive = false;
		Kind kind = Kind::Butter;
		float x = 0, y = 0, vy = 0, vx = 0;
		uint8_t frame = 0;
	};

	struct Star { uint8_t x, y, phase; };

	ButtonInput* buttonInput;
	ButtonGate buttonGate;

	uint16_t* fb = nullptr;
	lv_image_dsc_t imgDsc{};
	lv_obj_t* img = nullptr;
	lv_obj_t* hud = nullptr;     // score / lives text
	lv_obj_t* banner = nullptr;  // title / paused / game over text
	char hudText[32] = "";
	char bannerText[96] = "";

	Phase phase = Phase::Title;
	std::array<Thing, MaxThings> things{};
	std::array<Star, 24> stars{};
	float botX = W / 2.0f;
	float portalX = W / 2.0f;
	float portalTarget = W / 2.0f;
	int score = 0;
	int best = 0;
	int lives = Lives;
	uint32_t playMs = 0;        // time in play, drives the difficulty
	uint32_t nextSpawnMs = 0;
	uint32_t hurtUntil = 0;     // blink after a hit
	uint32_t lastFrame = 0;
	uint32_t tick = 0;
	uint32_t flashUntil = 0;    // screen flash on a gold butter
	bool newBest = false;

	void loop() override;
	void handleButtonEvent(Button btn, ButtonInput::Action action);

	void reset();
	void step(float dt);
	void spawn();
	void render();
	void setBanner(const char* text);
	void updateHud();

	void loadBest();
	void saveBest();

	// drawing
	void clear(uint16_t c);
	void rect(int x, int y, int w, int h, uint16_t c);
	void pixel(int x, int y, uint16_t c);
	void sprite(const char* const* rows, int x, int y, bool flip = false);
	void drawPortal(int cx, int cy);
	static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
};

#endif //BUTTERBOTCTRL_FIRMWARE_GAMESCREEN_H
