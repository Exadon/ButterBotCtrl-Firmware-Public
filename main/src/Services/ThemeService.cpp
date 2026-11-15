#include "ThemeService.h"
#include "Util/ThemeAssets.h"
#include <FileSystem/RawCache.h>
#include <FileSystem/SPIFFS.h>
#include <FileSystem/CompressedFile.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

static constexpr const char* TAG = "ThemeService";

static constexpr Asset CachedAssets[] = {
	Asset::BatteryController,
	Asset::BatteryButter,
	Asset::Battery1,
	Asset::Battery2,
	Asset::Battery3,
	Asset::Battery4,
	Asset::Battery5,
	Asset::Signal1,
	Asset::Signal2,
	Asset::Signal3,
	Asset::Grid,
	Asset::Corner,
	Asset::CornerInverse,
};

/* Style colors */
static const lv_color_t colorBlack = lv_color_black();
static const lv_color_t colorWhite = lv_color_white();

static const lv_color_t colorGreen = lv_color_make(160, 250, 0);
static const lv_color_t colorGreenDark = lv_color_make(0, 180, 0);

static const lv_color_t colorRose = lv_color_make(233, 0, 129);
static const lv_color_t colorRoseDark = lv_color_make(92, 0, 215);

static const lv_color_t colorRed = lv_color_make(225, 56, 0);
static const lv_color_t colorRedBg = lv_color_make(42, 22, 0);
static const lv_color_t colorYellow = lv_color_make(255, 174, 0);

static const lv_color_t colorBlue = lv_color_make(3, 0, 163);

static const ThemeStyle ThemeStyleMain = {
	colorGreen,
	colorGreenDark,
	colorBlack,
};

static const ThemeStyle ThemeStyleRose = {
	colorRose,
	colorRoseDark,
	colorBlack,
};

static const ThemeStyle ThemeStyleRed = {
	colorRed,
	colorYellow,
	colorRedBg,
};

// fork: the recoloured themes (keep in step with tools/theme_tool.py)
static const ThemeStyle ThemeStylePortal = { lv_color_make(0, 230, 190), lv_color_make(120, 255, 60), colorBlack };
static const ThemeStyle ThemeStyleButter = { lv_color_make(255, 214, 64), lv_color_make(214, 140, 0), lv_color_make(30, 18, 0) };
static const ThemeStyle ThemeStyleIce = { lv_color_make(120, 230, 255), lv_color_make(0, 140, 255), lv_color_make(0, 10, 40) };
static const ThemeStyle ThemeStyleMono = { colorWhite, lv_color_make(150, 150, 150), colorBlack };
static const ThemeStyle ThemeStylePurple = { lv_color_make(200, 120, 255), lv_color_make(255, 80, 200), lv_color_make(16, 0, 32) };

static const ThemeStyle ThemeStyleBlue = {
	colorBlue,
	colorYellow,
	colorWhite,
};

/* Theme maps */

static const ThemedPathsMap ThemedPaths =
{
	{ Theme::Main, &ThemeMainPaths },
	{ Theme::Rose, &ThemeRosePaths },
	{ Theme::Red, &ThemeRedPaths },
	{ Theme::Blue, &ThemeBluePaths },
	{ Theme::Portal, &ThemeMainPaths },  // the recoloured themes are copies of Main's archive
	{ Theme::Butter, &ThemeMainPaths },
	{ Theme::Ice, &ThemeMainPaths },
	{ Theme::Mono, &ThemeMainPaths },
	{ Theme::Purple, &ThemeMainPaths },
};

static const ThemedStylesMap ThemedStyles =
{
	{ Theme::Main, &ThemeStyleMain },
	{ Theme::Rose, &ThemeStyleRose },
	{ Theme::Red, &ThemeStyleRed },
	{ Theme::Blue, &ThemeStyleBlue },
	{ Theme::Portal, &ThemeStylePortal },
	{ Theme::Butter, &ThemeStyleButter },
	{ Theme::Ice, &ThemeStyleIce },
	{ Theme::Mono, &ThemeStyleMono },
	{ Theme::Purple, &ThemeStylePurple },
};

static constexpr const char* ThemeArchives[] = { "/t_1.sz.hs", "/t_2.sz.hs", "/t_3.sz.hs", "/t_4.sz.hs",
												 "/t_5.sz.hs", "/t_6.sz.hs", "/t_7.sz.hs", "/t_8.sz.hs", "/t_9.sz.hs" };
static_assert(std::size(ThemeArchives) == (size_t) Theme::COUNT, "one archive per theme");

static constexpr const char* IntroArchive = "/intro.sz.hs";
static constexpr const char* PairingArchive = "/pairing.sz.hs";

ThemeService::ThemeService(const Theme theme) : currentTheme(theme){
	// Reserve the shared decompression buffer
	archiveBuffer = (uint8_t*)heap_caps_malloc(ArchiveBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	if(archiveBuffer == nullptr){
		ESP_LOGE(TAG, "Failed to reserve %u B archive buffer in SPIRAM", ArchiveBufferSize);
	}

	animCache = new ArchiveCache();
	archiveCache = new ArchiveCache();
	animCache->setBuffer(archiveBuffer, ArchiveBufferSize);
	archiveCache->setBuffer(archiveBuffer, ArchiveBufferSize);
	rawCache = new RawCache(archiveCache);
	compositeCache = new CompositeCache<NumCaches>({ rawCache, archiveCache, animCache });
}

ThemeService::~ThemeService(){
	free(archiveBuffer);
	archiveBuffer = nullptr;
}

void ThemeService::setTheme(const Theme theme){
	currentTheme = theme;
	loadThemeArchive();
	activeAssets = ActiveAssets::Theme;
}

void ThemeService::activateIntroAssets(){
	loadStartupArchive(IntroArchive, ActiveAssets::Intro);
}

void ThemeService::activatePairingAssets(){
	loadStartupArchive(PairingArchive, ActiveAssets::Pairing);
}

void ThemeService::loadStartupArchive(const char* archivePath, const ActiveAssets target){
	if(activeAssets == target) return;
	archiveCache->unload();
	rawCache->unload();
	animCache->unload();

	animCache->setArchiveFile(CompressedFile::open(SPIFFS::open(archivePath)));
	animCache->load();
	activeAssets = target;
}

void ThemeService::activateThemeAssets(){
	if(activeAssets == ActiveAssets::Theme) return;
	animCache->unload();

	loadThemeArchive();
	activeAssets = ActiveAssets::Theme;
}

void ThemeService::loadThemeArchive(){
	archiveCache->unload();
	rawCache->unload();

	archiveCache->setArchiveFile(CompressedFile::open(SPIFFS::open(ThemeArchives[(size_t)currentTheme])));
	archiveCache->load();

	rawCache->setPaths(buildRawCachePaths());
	rawCache->load();
}

std::vector<std::string> ThemeService::buildRawCachePaths() const{
	std::vector<std::string> paths;
	paths.reserve(std::size(CachedAssets));
	for(const Asset asset : CachedAssets){
		const char* fullPath = getAsset(asset);
		paths.emplace_back(fullPath + 2);
	}
	return paths;
}

FileCache* ThemeService::getCache() const{
	return compositeCache;
}

Theme ThemeService::getTheme() const{
	return currentTheme;
}

const char* ThemeService::getAsset(const Asset asset) const{
	const char* assetPath = ThemePaths.at(asset);

	return assetPath;
}

const ThemeStyle& ThemeService::getThemeStyle() const{
	return getThemeStyle(currentTheme);
}

const ThemeStyle& ThemeService::getThemeStyle(const Theme theme){
	const ThemeStyle& style = *ThemedStyles.at(theme);

	return style;
}

lv_color_t ThemeService::getPrimaryColor() const{
	return getThemeStyle().primaryColor;
}

lv_color_t ThemeService::getSecondaryColor() const{
	return getThemeStyle().secondaryColor;
}

lv_color_t ThemeService::getTertiaryColor() const{
	return getThemeStyle().tertiaryColor;
}
