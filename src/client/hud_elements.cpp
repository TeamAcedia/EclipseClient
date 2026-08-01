// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#include "hud_elements.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <ICameraSceneNode.h>
#include <ISceneManager.h>
#include <IVideoDriver.h>

#include "client/client.h"
#include "client/fontengine.h"
#include "settings.h"
#include "util/string.h"

namespace {

constexpr f32 HUD_ELEM_MIN_SCALE = 0.5f;
constexpr f32 HUD_ELEM_MAX_SCALE = 3.0f;

f32 compute_uniform_scale(f32 scale_x, f32 scale_y)
{
	return std::sqrt(std::max(0.01f, scale_x * scale_y));
}

gui::IGUIFont *get_scaled_hud_font(gui::IGUIFont *fallback_font, f32 uniform_scale)
{
	if (!fallback_font || !g_fontengine)
		return fallback_font;

	core::dimension2du dim = fallback_font->getDimension(L"Hg");
	s32 base_size = std::max<s32>(8, static_cast<s32>(dim.Height));
	s32 scaled_size = std::max<s32>(8, static_cast<s32>(std::round(base_size * uniform_scale)));
	if (gui::IGUIFont *scaled = g_fontengine->getFont(scaled_size, FM_Standard))
		return scaled;

	return fallback_font;
}

f32 shortest_angle_delta(f32 from_deg, f32 to_deg)
{
	f32 delta = std::fmod(to_deg - from_deg + 540.0f, 360.0f) - 180.0f;
	return delta;
}

class FpsHudElement final : public HudElementBase
{
public:
	FpsHudElement() : HudElementBase("FPS", "eclipse_hud_fps", "eclipse_hud_fps", 0.02f, 0.02f) {}

	bool render(const HudElementRenderContext &ctx) override
	{
		if (!ctx.driver || !ctx.font || !ctx.theme)
			return false;

		std::ostringstream ss;
		ss << "FPS: " << static_cast<int>(std::round(std::max(0.0f, ctx.smoothed_fps)));
		drawTextPanel(ctx.driver, ctx.font, *ctx.theme, utf8_to_wide(ss.str()), ctx.screensize);
		return true;
	}
};

class MemoryHudElement final : public HudElementBase
{
public:
	MemoryHudElement() : HudElementBase("Memory", "eclipse_hud_memory", "eclipse_hud_memory", 0.02f, 0.08f) {}

	bool render(const HudElementRenderContext &ctx) override
	{
		if (!ctx.driver || !ctx.font || !ctx.theme)
			return false;

		std::string text = "Memory: N/A";
#ifdef __linux__
		std::ifstream status_file("/proc/self/status");
		std::string line;
		while (std::getline(status_file, line)) {
			if (line.rfind("VmRSS:", 0) == 0) {
				std::istringstream value_stream(line.substr(6));
				double kb = 0.0;
				value_stream >> kb;
				std::ostringstream memory_stream;
				memory_stream << "Memory: " << std::fixed << std::setprecision(1)
					<< (kb / 1024.0) << " MiB";
				text = memory_stream.str();
				break;
			}
		}
#endif
		drawTextPanel(ctx.driver, ctx.font, *ctx.theme, utf8_to_wide(text), ctx.screensize);
		return true;
	}
};

class PingHudElement final : public HudElementBase
{
public:
	PingHudElement() : HudElementBase("Ping", "eclipse_hud_ping", "eclipse_hud_ping", 0.02f, 0.14f) {}

	bool render(const HudElementRenderContext &ctx) override
	{
		if (!ctx.driver || !ctx.font || !ctx.theme)
			return false;

		float ping_ms = 0.0f;
		if (ctx.client)
			ping_ms = ctx.client->getRTT() * 1000.0f;

		std::ostringstream ss;
		ss << "Ping: " << std::fixed << std::setprecision(0) << ping_ms << "ms";
		drawTextPanel(ctx.driver, ctx.font, *ctx.theme, utf8_to_wide(ss.str()), ctx.screensize);
		return true;
	}
};

class CompassHudElement final : public HudElementBase
{
public:
	CompassHudElement() : HudElementBase("Compass", "eclipse_hud_compass", "eclipse_hud_compass", 0.02f, 0.20f) {}

	bool render(const HudElementRenderContext &ctx) override
	{
		if (!ctx.driver || !ctx.font || !ctx.theme)
			return false;

		f32 scale_x = std::clamp(g_settings->getFloat(m_scale_x_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
		f32 scale_y = std::clamp(g_settings->getFloat(m_scale_y_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
		s32 width = std::max<s32>(200, static_cast<s32>(std::round(320.0f * scale_x)));
		s32 height = std::max<s32>(48, static_cast<s32>(std::round(64.0f * scale_y)));
		Frame frame = layoutFrame(ctx.screensize, width, height);
		drawPanel(ctx.driver, *ctx.theme, frame);
		gui::IGUIFont *draw_font = get_scaled_hud_font(ctx.font, frame.scale_uniform);

		s32 inset = std::max<s32>(6, static_cast<s32>(std::round(8.0f * frame.scale_uniform)));
		core::rect<s32> inner(
			frame.rect.UpperLeftCorner.X + inset,
			frame.rect.UpperLeftCorner.Y + inset,
			frame.rect.LowerRightCorner.X - inset,
			frame.rect.LowerRightCorner.Y - inset
		);

		if (inner.getWidth() <= 0 || inner.getHeight() <= 0)
			return true;

		f32 heading = 0.0f;
		if (ctx.client) {
			if (scene::ISceneManager *smgr = ctx.client->getSceneManager()) {
				if (scene::ICameraSceneNode *cam = smgr->getActiveCamera()) {
					v3f fore = cam->getAbsoluteTransformation().rotateAndScaleVect(v3f(0.f, 0.f, 1.f));
					heading = std::fmod((-fore.getHorizontalAngle().Y + 360.0f), 360.0f);
				}
			}
		}

		s32 center_x = inner.getCenter().X;
		ctx.driver->draw2DRectangle(ctx.theme->hud_elem_accent,
			core::rect<s32>(center_x - 1, inner.UpperLeftCorner.Y, center_x + 1, inner.LowerRightCorner.Y), nullptr);

		const f32 range_deg = 120.0f;
		for (int deg = 0; deg < 360; deg += 15) {
			f32 diff = shortest_angle_delta(heading, static_cast<f32>(deg));
			if (std::abs(diff) > range_deg)
				continue;

			f32 t = diff / range_deg;
			s32 x = center_x + static_cast<s32>(std::round(t * (inner.getWidth() / 2.0f)));
			bool is_major = (deg % 45) == 0;
			s32 tick_h = is_major
				? static_cast<s32>(std::round(inner.getHeight() * 0.55f))
				: static_cast<s32>(std::round(inner.getHeight() * 0.35f));
			video::SColor tick_color = is_major ? ctx.theme->hud_elem_tick_major : ctx.theme->hud_elem_tick_minor;
			ctx.driver->draw2DRectangle(
				tick_color,
				core::rect<s32>(x, inner.LowerRightCorner.Y - tick_h, x + std::max<s32>(1, static_cast<s32>(std::round(frame.scale_uniform))), inner.LowerRightCorner.Y),
				nullptr
			);

			if ((deg % 90) == 0) {
				std::string label;
				switch (deg) {
					case 0: label = "N"; break;
					case 90: label = "E"; break;
					case 180: label = "S"; break;
					case 270: label = "W"; break;
					default: break;
				}
				if (!label.empty()) {
					s32 lw = static_cast<s32>(std::round(20.0f * frame.scale_uniform));
					s32 lh = static_cast<s32>(std::round(16.0f * frame.scale_uniform));
					core::rect<s32> label_rect(x - lw / 2, inner.UpperLeftCorner.Y, x + lw / 2, inner.UpperLeftCorner.Y + lh);
					draw_font->draw(utf8_to_wide(label).c_str(), label_rect, ctx.theme->hud_elem_text, true, true, nullptr);
				}
			}
		}

		return true;
	}
};

} // namespace

HudElementBase::HudElementBase(std::string label, std::string enabled_key, std::string setting_prefix,
		f32 default_x, f32 default_y)
	: m_label(std::move(label)), m_enabled_key(std::move(enabled_key)),
	  m_x_key(std::move(setting_prefix) + ".x"),
	  m_y_key(m_x_key.substr(0, m_x_key.size() - 2) + ".y"),
	  m_scale_key(m_x_key.substr(0, m_x_key.size() - 2) + ".scale"),
	  m_scale_x_key(m_x_key.substr(0, m_x_key.size() - 2) + ".scale_x"),
	  m_scale_y_key(m_x_key.substr(0, m_x_key.size() - 2) + ".scale_y"),
	  m_default_x(default_x), m_default_y(default_y)
{
	m_last_rect = core::rect<s32>(0, 0, 0, 0);
}

void HudElementBase::ensureDefaults()
{
	g_settings->setDefault(m_enabled_key, "false");
	g_settings->setDefault(m_scale_key, "1.0");
	g_settings->setDefault(m_scale_x_key, "1.0");
	g_settings->setDefault(m_scale_y_key, "1.0");
	g_settings->setDefault(m_x_key, std::to_string(m_default_x));
	g_settings->setDefault(m_y_key, std::to_string(m_default_y));

	// Migrate legacy uniform scale to axis scales for existing users.
	if (!g_settings->exists(m_scale_x_key) || !g_settings->exists(m_scale_y_key)) {
		f32 legacy_scale = std::clamp(g_settings->getFloat(m_scale_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
		g_settings->setFloat(m_scale_x_key, legacy_scale);
		g_settings->setFloat(m_scale_y_key, legacy_scale);
	}
}

bool HudElementBase::isEnabled() const
{
	return g_settings->getBool(m_enabled_key);
}

HudElementBase::Frame HudElementBase::layoutFrame(const v2u32 &screensize, s32 width, s32 height)
{
	f32 scale_x = std::clamp(g_settings->getFloat(m_scale_x_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
	f32 scale_y = std::clamp(g_settings->getFloat(m_scale_y_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
	f32 scale_uniform = compute_uniform_scale(scale_x, scale_y);
	f32 nx = std::clamp(g_settings->getFloat(m_x_key), 0.0f, 1.0f);
	f32 ny = std::clamp(g_settings->getFloat(m_y_key), 0.0f, 1.0f);

	s32 x = static_cast<s32>(std::round(nx * static_cast<f32>(screensize.X - 1)));
	s32 y = static_cast<s32>(std::round(ny * static_cast<f32>(screensize.Y - 1)));

	s32 max_x = std::max<s32>(0, static_cast<s32>(screensize.X) - width);
	s32 max_y = std::max<s32>(0, static_cast<s32>(screensize.Y) - height);
	x = std::clamp(x, 0, max_x);
	y = std::clamp(y, 0, max_y);

	f32 clamped_nx = static_cast<f32>(x) / static_cast<f32>(std::max(1, static_cast<s32>(screensize.X) - 1));
	f32 clamped_ny = static_cast<f32>(y) / static_cast<f32>(std::max(1, static_cast<s32>(screensize.Y) - 1));
	g_settings->setFloat(m_x_key, clamped_nx);
	g_settings->setFloat(m_y_key, clamped_ny);

	m_last_rect = core::rect<s32>(x, y, x + width, y + height);
	return {m_last_rect, scale_x, scale_y, scale_uniform};
}

HudElementBase::Frame HudElementBase::layoutTextFrame(gui::IGUIFont *font, const std::wstring &text, const v2u32 &screensize)
{
	f32 scale_x = std::clamp(g_settings->getFloat(m_scale_x_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
	f32 scale_y = std::clamp(g_settings->getFloat(m_scale_y_key), HUD_ELEM_MIN_SCALE, HUD_ELEM_MAX_SCALE);
	f32 scale_uniform = compute_uniform_scale(scale_x, scale_y);
	gui::IGUIFont *draw_font = get_scaled_hud_font(font, scale_uniform);
	core::dimension2du text_size = draw_font->getDimension(text.c_str());
	s32 pad_x = std::max<s32>(4, static_cast<s32>(std::round(8.0f * scale_x)));
	s32 pad_y = std::max<s32>(4, static_cast<s32>(std::round(8.0f * scale_y)));
	s32 width = std::max<s32>(40, text_size.Width + pad_x * 2);
	s32 height = std::max<s32>(20, text_size.Height + pad_y);
	return layoutFrame(screensize, width, height);
}

void HudElementBase::drawPanel(video::IVideoDriver *driver, const ColorTheme &theme, const Frame &frame)
{
	s32 radius = std::max<s32>(4, static_cast<s32>(std::round(6.0f * frame.scale_uniform)));
	s32 border = std::max<s32>(1, static_cast<s32>(std::round(2.0f * frame.scale_uniform)));
	driver->draw2DRoundedRectangle(frame.rect, theme.hud_elem_background, radius, nullptr);
	driver->draw2DRoundedRectangleOutline(frame.rect, theme.hud_elem_border, border,
		radius, radius, radius, radius);
}

void HudElementBase::drawTextPanel(video::IVideoDriver *driver, gui::IGUIFont *font,
		const ColorTheme &theme, const std::wstring &text, const v2u32 &screensize)
{
	Frame frame = layoutTextFrame(font, text, screensize);
	drawPanel(driver, theme, frame);
	gui::IGUIFont *draw_font = get_scaled_hud_font(font, frame.scale_uniform);
	draw_font->draw(text.c_str(), frame.rect, theme.hud_elem_text, true, true, nullptr);
}

std::vector<std::unique_ptr<HudElementBase>> create_default_hud_elements()
{
	std::vector<std::unique_ptr<HudElementBase>> elements;
	elements.emplace_back(std::make_unique<FpsHudElement>());
	elements.emplace_back(std::make_unique<MemoryHudElement>());
	elements.emplace_back(std::make_unique<PingHudElement>());
	elements.emplace_back(std::make_unique<CompassHudElement>());

	for (auto &element : elements)
		element->ensureDefaults();

	return elements;
}
