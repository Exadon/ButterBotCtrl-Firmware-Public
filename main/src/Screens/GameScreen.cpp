#include "GameScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <nvs.h>
#include <Core/Application.h>
#include <Fonts/font.hpp>
#include <LV_Interface/LVGL.h>
#include <Periph/Joystick.h>

#include "HomeScreen.h"
#include "Services/Com.h"
#include "PairingScreen.h"

// --- art (original pixel art) ---------------------------------------------------------------------------------
// '.' transparent, g body, G body shade, w white, k black, y yellow, Y light yellow, o orange,
// p purple, P dark purple, r red, c cyan, s grey

static const char* const BotArt[] = {
		"......yy......",
		"......ss......",
		"..gggggggggg..",
		".gGGGGGGGGGGg.",
		".gGwwkGGwwkGg.",
		".gGwwkGGwwkGg.",
		".gGGGGGGGGGGg.",
		".gGGGrrrrGGGg.",
		"..gggggggggg..",
		".ss.gggggg.ss.",
		".sk.s....s.ks.",
		"..kk......kk..",
};
static constexpr int BotW = 14, BotH = 12;

static const char* const ButterArt[] = {
		"..YYYYYY..",
		".YyyyyyyY.",
		"YyyyyyyyyY",
		"YyyyyyyyyY",
		".oooooooo.",
};
static constexpr int ButterW = 10, ButterH = 5;

static const char* const GoldArt[] = {
		"..wYYYYw..",
		".wyyyyyyY.",
		"YyywyyyyyY",
		"YyyyyyywyY",
		".oooooooo.",
};

static const char* const MonsterArtA[] = {
		"..p.....p..",
		"...ppppp...",
		"..ppppppp..",
		".ppwwpwwpp.",
		".ppwkpwkpp.",
		"ppppppppppp",
		"pPpwpwpwpPp",
		".ppppppppp.",
		".p.p...p.p.",
};
static const char* const MonsterArtB[] = {
		".p.......p.",
		"...ppppp...",
		"..ppppppp..",
		".ppwwpwwpp.",
		".ppkwpkwpp.",
		"ppppppppppp",
		"pPpwpwpwpPp",
		".ppppppppp.",
		"p.p.....p.p",
};
static constexpr int MonsterW = 11, MonsterH = 9;

static constexpr const char* NvsNs = "bbgame";
static constexpr const char* NvsKey = "best";

uint16_t GameScreen::rgb(uint8_t r, uint8_t g, uint8_t b){
	return (uint16_t) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t artColor(char c){
	switch(c){
		case 'g': return (uint16_t) (((0x40 & 0xF8) << 8) | ((0xD0 & 0xFC) << 3) | (0x60 >> 3));
		case 'G': return (uint16_t) (((0x2A & 0xF8) << 8) | ((0x90 & 0xFC) << 3) | (0x40 >> 3));
		case 'w': return 0xFFFF;
		case 'k': return 0x0000;
		case 'y': return (uint16_t) (((0xFF & 0xF8) << 8) | ((0xD8 & 0xFC) << 3) | (0x40 >> 3));
		case 'Y': return (uint16_t) (((0xFF & 0xF8) << 8) | ((0xF0 & 0xFC) << 3) | (0x98 >> 3));
		case 'o': return (uint16_t) (((0xC8 & 0xF8) << 8) | ((0x80 & 0xFC) << 3) | (0x20 >> 3));
		case 'p': return (uint16_t) (((0xB0 & 0xF8) << 8) | ((0x40 & 0xFC) << 3) | (0xE8 >> 3));
		case 'P': return (uint16_t) (((0x60 & 0xF8) << 8) | ((0x18 & 0xFC) << 3) | (0x90 >> 3));
		case 'r': return (uint16_t) (((0xF0 & 0xF8) << 8) | ((0x40 & 0xFC) << 3) | (0x40 >> 3));
		case 'c': return (uint16_t) (((0x40 & 0xF8) << 8) | ((0xF0 & 0xFC) << 3) | (0xF0 >> 3));
		case 's': return (uint16_t) (((0x90 & 0xF8) << 8) | ((0x98 & 0xFC) << 3) | (0xA0 >> 3));
		default: return 0;
	}
}

static float frand(){
	return (float) (esp_random() & 0xFFFF) / 65535.0f;
}

GameScreen::GameScreen(){
	const auto app = Application::getApp();
	buttonInput = app->getService<ButtonInput>();
	LVGL* lvgl = app->getService<LVGL>();

	buttonInput->OnButtonEvent.bind(lvgl, [this](Enum<int> btn, ButtonInput::Action action) {
		handleButtonEvent((Button) (int) btn, action);
	});

	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);

	fb = (uint16_t*) heap_caps_malloc(W * H * 2, MALLOC_CAP_SPIRAM);
	if(fb != nullptr){
		memset(fb, 0, W * H * 2);
	}
	imgDsc.header.magic = LV_IMAGE_HEADER_MAGIC;
	imgDsc.header.cf = LV_COLOR_FORMAT_RGB565;
	imgDsc.header.w = W;
	imgDsc.header.h = H;
	imgDsc.header.stride = W * 2;
	imgDsc.data_size = W * H * 2;
	imgDsc.data = (const uint8_t*) fb;

	img = lv_image_create(*this);
	lv_obj_set_pos(img, 0, 0);
	if(fb != nullptr){
		lv_image_set_src(img, &imgDsc);
	}

	hud = lv_label_create(*this);
	lv_obj_set_style_text_font(hud, &lv_font_butter, 0);
	lv_obj_set_style_text_color(hud, lv_color_hex(0xFFE070), 0);
	lv_obj_set_pos(hud, 2, 1);

	banner = lv_label_create(*this);
	lv_obj_set_style_text_font(banner, &lv_font_butter, 0);
	lv_obj_set_style_text_color(banner, lv_color_hex(0x7DFF9A), 0);
	lv_obj_set_style_text_align(banner, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_line_space(banner, 3, 0);
	lv_obj_set_style_bg_color(banner, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(banner, LV_OPA_70, 0);
	lv_obj_set_style_pad_all(banner, 4, 0);
	lv_obj_set_style_radius(banner, 3, 0);
	lv_obj_set_width(banner, 118);
	lv_label_set_long_mode(banner, LV_LABEL_LONG_WRAP);
	lv_obj_align(banner, LV_ALIGN_CENTER, 0, 6);

	for(auto& s : stars){
		s = { (uint8_t) (esp_random() % W), (uint8_t) (14 + esp_random() % (GroundY - 20)), (uint8_t) (esp_random() & 0xFF) };
	}

	loadBest();
	reset();
	phase = Phase::Title;
	char t[96];
	snprintf(t, sizeof(t), "BUTTER RUN\n\nGrab the butter,\ndodge the monsters!\n\nShut Up: start\nOverride: leave\nBest: %d", best);
	setBanner(t);
	render();
}

GameScreen::~GameScreen(){
	const auto app = Application::getApp();
	buttonInput->OnButtonEvent.unbind(app->getService<LVGL>());
	if(img != nullptr){
		lv_obj_delete(img); // stop LVGL drawing from fb before it goes
		img = nullptr;
	}
	free(fb);
}

void GameScreen::loadBest(){
	nvs_handle_t h;
	if(nvs_open(NvsNs, NVS_READONLY, &h) == ESP_OK){
		int32_t v = 0;
		if(nvs_get_i32(h, NvsKey, &v) == ESP_OK) best = (int) v;
		nvs_close(h);
	}
}

void GameScreen::saveBest(){
	nvs_handle_t h;
	if(nvs_open(NvsNs, NVS_READWRITE, &h) == ESP_OK){
		nvs_set_i32(h, NvsKey, best);
		nvs_commit(h);
		nvs_close(h);
	}
}

void GameScreen::reset(){
	for(auto& t : things) t.alive = false;
	botX = W / 2.0f;
	score = 0;
	lives = Lives;
	playMs = 0;
	nextSpawnMs = 600;
	hurtUntil = 0;
	flashUntil = 0;
	newBest = false;
	updateHud();
}

void GameScreen::setBanner(const char* text){
	if(text == nullptr || text[0] == '\0'){
		lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
		return;
	}
	strncpy(bannerText, text, sizeof(bannerText) - 1);
	lv_label_set_text_static(banner, bannerText);
	lv_obj_remove_flag(banner, LV_OBJ_FLAG_HIDDEN);
	lv_obj_align(banner, LV_ALIGN_CENTER, 0, 6);
}

void GameScreen::updateHud(){
	char hearts[8] = "";
	for(int i = 0; i < Lives; ++i) hearts[i] = i < lives ? '+' : '-';
	hearts[Lives] = '\0';
	snprintf(hudText, sizeof(hudText), "BUTTER %d  LIVES %s", score, hearts);
	lv_label_set_text_static(hud, hudText);
}

void GameScreen::handleButtonEvent(const Button btn, const ButtonInput::Action action){
	if(!buttonGate.accept(btn, action) || action != ButtonInput::Action::Release){
		return;
	}
	if(btn == Button::ManualOverride){
		Com* com = Application::getApp()->getService<Com>();
		if(com != nullptr && com->getStatus() != Com::ConnStatus::Connected){
			transition([]() { return std::make_unique<PairingScreen>(); });
		}else{
			transition([]() { return std::make_unique<HomeScreen>(); });
		}
		return;
	}
	if(btn != Button::ShutUp) return;

	switch(phase){
		case Phase::Title:
		case Phase::Over:
			reset();
			phase = Phase::Play;
			setBanner(nullptr);
			break;
		case Phase::Play:
			phase = Phase::Paused;
			setBanner("PAUSED\n\nShut Up: go on\nOverride: leave");
			break;
		case Phase::Paused:
			phase = Phase::Play;
			setBanner(nullptr);
			break;
	}
	lastFrame = millis();
}

void GameScreen::spawn(){
	Thing* t = nullptr;
	for(auto& c : things){
		if(!c.alive){
			t = &c;
			break;
		}
	}
	if(t == nullptr) return;

	const float level = std::min(1.0f, playMs / 90000.0f); // full speed after 1.5 minutes
	const float monsterChance = 0.30f + 0.25f * level;
	const float r = frand();
	t->kind = r < monsterChance ? Kind::Monster : (r > 0.96f ? Kind::GoldButter : Kind::Butter);
	t->alive = true;
	t->x = portalX + (frand() - 0.5f) * 10.0f;
	t->y = 16;
	t->vy = 26.0f + 50.0f * level + frand() * 14.0f;
	t->vx = t->kind == Kind::Monster ? (frand() - 0.5f) * (10.0f + 25.0f * level) : (frand() - 0.5f) * 8.0f;
	t->frame = 0;
}

void GameScreen::step(float dt){
	const uint32_t ms = (uint32_t) (dt * 1000.0f);
	playMs += ms;

	// stick
	if(Joystick* joy = Application::getApp()->getDevice<Joystick>()){
		const int h = joy->getHorizontal();
		if(h > 12 || h < -12){
			botX += h / 100.0f * 95.0f * dt;
		}
	}
	botX = std::clamp(botX, BotW / 2.0f, W - BotW / 2.0f);

	// the portal wanders
	if(std::fabs(portalX - portalTarget) < 2.0f){
		portalTarget = 14.0f + frand() * (W - 28.0f);
	}
	portalX += (portalTarget > portalX ? 1 : -1) * 22.0f * dt;

	// spawning
	if(playMs >= nextSpawnMs){
		spawn();
		const float level = std::min(1.0f, playMs / 90000.0f);
		nextSpawnMs = playMs + (uint32_t) (900 - 520 * level + frand() * 250);
	}

	const float botL = botX - BotW / 2.0f + 1, botR = botX + BotW / 2.0f - 1;
	const float botT = GroundY - BotH + 2;
	const uint32_t now = millis();

	for(auto& t : things){
		if(!t.alive) continue;
		t.y += t.vy * dt;
		t.x += t.vx * dt;
		if(t.x < 6 || t.x > W - 6) t.vx = -t.vx;
		if((tick & 7) == 0) t.frame ^= 1;

		const int w = t.kind == Kind::Monster ? MonsterW : ButterW;
		const int h = t.kind == Kind::Monster ? MonsterH : ButterH;
		const float l = t.x - w / 2.0f, r = t.x + w / 2.0f, b = t.y + h;
		if(b >= botT && t.y <= GroundY && r >= botL && l <= botR){
			t.alive = false;
			if(t.kind == Kind::Monster){
				if(now >= hurtUntil){
					lives--;
					hurtUntil = now + 1200;
				}
			}else{
				score += t.kind == Kind::GoldButter ? 5 : 1;
				if(t.kind == Kind::GoldButter) flashUntil = now + 150;
			}
			updateHud();
			continue;
		}
		if(t.y > GroundY) t.alive = false;
	}

	if(lives <= 0){
		phase = Phase::Over;
		if(score > best){
			best = score;
			newBest = true;
			saveBest();
		}
		char t[96];
		snprintf(t, sizeof(t), "GAME OVER\n\nButter: %d\n%s%d\n\nShut Up: again\nOverride: leave", score,
		         newBest ? "NEW BEST! " : "Best: ", best);
		setBanner(t);
		updateHud();
	}
}

void GameScreen::loop(){
	const uint32_t now = millis();
	if(now - lastFrame < FrameMs) return;
	const float dt = std::min(0.1f, (now - lastFrame) / 1000.0f);
	lastFrame = now;
	tick++;

	if(phase == Phase::Play){
		step(dt);
	}else{
		// keep the portal alive behind the menus
		portalX += (portalTarget > portalX ? 1 : -1) * 10.0f * dt;
		if(std::fabs(portalX - portalTarget) < 2.0f) portalTarget = 14.0f + frand() * (W - 28.0f);
	}
	render();
}

// --- drawing ---------------------------------------------------------------------------------------------------

void GameScreen::clear(uint16_t c){
	for(int i = 0; i < W * H; ++i) fb[i] = c;
}

void GameScreen::pixel(int x, int y, uint16_t c){
	if(x < 0 || y < 0 || x >= W || y >= H) return;
	fb[y * W + x] = c;
}

void GameScreen::rect(int x, int y, int w, int h, uint16_t c){
	for(int yy = std::max(0, y); yy < std::min(H, y + h); ++yy){
		for(int xx = std::max(0, x); xx < std::min(W, x + w); ++xx){
			fb[yy * W + xx] = c;
		}
	}
}

void GameScreen::sprite(const char* const* rows, int x, int y, bool flip){
	for(int r = 0; r < 16 && rows[r] != nullptr; ++r){
		const int len = (int) strlen(rows[r]);
		for(int c = 0; c < len; ++c){
			const char ch = rows[r][flip ? len - 1 - c : c];
			if(ch == '.') continue;
			pixel(x + c, y + r, artColor(ch));
		}
	}
}

void GameScreen::drawPortal(int cx, int cy){
	// a swirling green ellipse: rings of dots turning at different speeds
	static constexpr int Rx = 13, Ry = 5;
	rect(cx - Rx + 3, cy - Ry + 2, 2 * Rx - 5, 2 * Ry - 3, rgb(10, 60, 30));
	for(int ring = 0; ring < 3; ++ring){
		const float rx = Rx - ring * 3.5f, ry = Ry - ring * 1.3f;
		const int dots = 22 - ring * 6;
		const float spin = (ring & 1 ? -1.0f : 1.0f) * tick * (0.08f + ring * 0.05f);
		for(int i = 0; i < dots; ++i){
			const float a = spin + i * 6.2832f / dots;
			const uint16_t col = (i + ring + tick / 3) % 3 == 0 ? rgb(200, 255, 120) : rgb(60, 220, 90);
			pixel(cx + (int) lroundf(cosf(a) * rx), cy + (int) lroundf(sinf(a) * ry), col);
		}
	}
}

// nullptr-terminated copies so sprite() knows where the art ends
static const char* const* withEnd(const char* const* art, int rows, const char** buf){
	for(int i = 0; i < rows; ++i) buf[i] = art[i];
	buf[rows] = nullptr;
	return buf;
}

void GameScreen::render(){
	if(fb == nullptr) return;
	const uint32_t now = millis();

	clear(now < flashUntil ? rgb(70, 60, 10) : rgb(6, 8, 18));

	// stars
	for(auto& s : stars){
		const uint8_t p = (uint8_t) (s.phase + tick * 3);
		const uint8_t v = p < 128 ? 60 + p : 60 + (255 - p);
		pixel(s.x, s.y, rgb(v, v, v + 20 > 255 ? 255 : v + 20));
	}

	// floor
	rect(0, GroundY, W, H - GroundY, rgb(34, 40, 52));
	for(int x = (int) (tick % 8); x < W; x += 8) rect(x, GroundY, 3, 1, rgb(70, 80, 100));

	drawPortal((int) portalX, 18);

	const char* buf[17];
	for(const auto& t : things){
		if(!t.alive) continue;
		if(t.kind == Kind::Monster){
			sprite(withEnd(t.frame ? MonsterArtB : MonsterArtA, MonsterH, buf), (int) t.x - MonsterW / 2, (int) t.y);
		}else{
			sprite(withEnd(t.kind == Kind::GoldButter ? GoldArt : ButterArt, ButterH, buf), (int) t.x - ButterW / 2,
			       (int) t.y);
		}
	}

	// the robot blinks after a hit
	const bool hurt = now < hurtUntil;
	if(!hurt || (tick / 3) % 2 == 0){
		const int bob = phase == Phase::Play && (tick / 4) % 2 ? 1 : 0;
		sprite(withEnd(BotArt, BotH, buf), (int) botX - BotW / 2, GroundY - BotH + bob);
	}

	lv_obj_invalidate(img);
}
