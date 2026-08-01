#pragma once

#include <vector>
#include <string>
#include "irrlichttypes_bloated.h"

class ColorTheme {
public:
	std::string name;

	video::SColor wallpaper;

	video::SColor background_top;
	video::SColor background;
	video::SColor background_bottom;

	video::SColor border;

	video::SColor text;
	video::SColor text_muted;

	video::SColor primary;
	video::SColor primary_muted;

	video::SColor secondary;
	video::SColor secondary_muted;
	
	video::SColor enabled;
	video::SColor disabled;

	video::SColor hud_elem_background;
	video::SColor hud_elem_border;
	video::SColor hud_elem_text;
	video::SColor hud_elem_accent;
	video::SColor hud_elem_tick_major;
	video::SColor hud_elem_tick_minor;

	ColorTheme() = default;
	explicit ColorTheme(const std::string &data);

	ColorTheme withAlpha(float alpha) const;
};

class ThemeManager {
public:
	// Load all .theme files in a folder
	void LoadThemes(const std::string &folderpath);

	// Return available theme names
	std::vector<std::string> GetThemes() const;

	// Get a theme by name (returns a placeholder theme if not found)
	ColorTheme GetThemeByName(const std::string &name) const;

private:
	std::vector<ColorTheme> themes;
};
