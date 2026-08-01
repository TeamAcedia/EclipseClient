// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <irrlicht.h>
#include <IGUIFont.h>

#include "client/color_theme.h"

class Client;

struct HudElementRenderContext
{
	video::IVideoDriver *driver = nullptr;
	gui::IGUIFont *font = nullptr;
	v2u32 screensize;
	Client *client = nullptr;
	const ColorTheme *theme = nullptr;
	f32 smoothed_fps = 0.0f;
};

class HudElementBase
{
public:
	HudElementBase(std::string label, std::string enabled_key, std::string setting_prefix,
			f32 default_x, f32 default_y);
	virtual ~HudElementBase() = default;

	void ensureDefaults();
	bool isEnabled() const;
	virtual bool render(const HudElementRenderContext &ctx) = 0;

	const std::string &getLabel() const { return m_label; }
	const std::string &getEnabledKey() const { return m_enabled_key; }
	const std::string &getXKey() const { return m_x_key; }
	const std::string &getYKey() const { return m_y_key; }
	const std::string &getScaleKey() const { return m_scale_key; } // legacy
	const std::string &getScaleXKey() const { return m_scale_x_key; }
	const std::string &getScaleYKey() const { return m_scale_y_key; }
	const core::rect<s32> &getLastRect() const { return m_last_rect; }

protected:
	struct Frame {
		core::rect<s32> rect;
		f32 scale_x;
		f32 scale_y;
		f32 scale_uniform;
	};

	Frame layoutFrame(const v2u32 &screensize, s32 width, s32 height);
	Frame layoutTextFrame(gui::IGUIFont *font, const std::wstring &text, const v2u32 &screensize);
	void drawPanel(video::IVideoDriver *driver, const ColorTheme &theme, const Frame &frame);
	void drawTextPanel(video::IVideoDriver *driver, gui::IGUIFont *font,
			const ColorTheme &theme, const std::wstring &text, const v2u32 &screensize);

	std::string m_label;
	std::string m_enabled_key;
	std::string m_x_key;
	std::string m_y_key;
	std::string m_scale_key;
	std::string m_scale_x_key;
	std::string m_scale_y_key;
	f32 m_default_x;
	f32 m_default_y;
	core::rect<s32> m_last_rect;
};

std::vector<std::unique_ptr<HudElementBase>> create_default_hud_elements();
