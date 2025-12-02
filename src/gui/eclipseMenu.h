// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#pragma once

#include <irrlicht.h>
#include <iostream>
#include <vector>
#include <codecvt> 
#include <locale> 
#include <IGUIEditBox.h>
#include <IGUIEnvironment.h>
#include <IVideoDriver.h>
#include <IGUIFont.h>
#include <chrono>
#include <cmath>

#include "client/color_theme.h"
#include "script/scripting_client.h"
#include "client/client.h"
#include "log.h"
#include "gui/modalMenu.h"
#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "scripting_mainmenu.h"
#include "client/texturesource.h"
#include "eclipseEditBox.h"

using namespace gui;

#define GET_CATEGORIES_OR_RETURN(categories)                \
    std::vector<ModCategory*> categories;                   \
    if (!m_is_main_menu) {                                  \
        if (!m_client)                                      \
            return;                                         \
        ClientScripting* script = m_client->getScript();    \
        if (!script)                                        \
            return;                                         \
        categories = script->m_categories;                  \
    } else {                                                \
        if (!m_script)                                      \
            return;                                         \
        categories = m_script->m_categories;                \
    }

#define GET_CATEGORIES_OR_RETURN_BOOL(categories)           \
    std::vector<ModCategory*> categories;                   \
    if (!m_is_main_menu) {                                  \
        if (!m_client)                                      \
            return false;                                   \
        ClientScripting* script = m_client->getScript();    \
        if (!script)                                        \
            return false;                                   \
        categories = script->m_categories;                  \
    } else {                                                \
        if (!m_script)                                      \
            return false;                                   \
        categories = m_script->m_categories;                \
    }

struct TextboxData {
    EclipseEditBox* textbox;
    core::rect<s32> clipRect;
    std::string parent_mod_name;
    std::string parent_category_name;
    std::string setting_id;
};

class EclipseMenu: public IGUIElement
{
public:
    EclipseMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id, IMenuManager* menumgr, Client *client, bool is_main_menu, ISimpleTextureSource *texture_src);
    EclipseMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id, IMenuManager* menumgr, MainMenuScripting *script, bool is_main_menu, ISimpleTextureSource *texture_src);

	void updateTheming();
	void updateScaling();

	void create();
	void close();

    virtual bool OnEvent(const SEvent& event) override;
    virtual void draw() override;
    bool isOpen() { return m_is_open; }
    
    ~EclipseMenu();

    bool m_initialized = false;
private:
    ISimpleTextureSource *m_texture_src = nullptr;

    std::string themes_path;
    ThemeManager theme_manager;
    std::string current_theme_name;
    ColorTheme target_theme;
    ColorTheme current_theme;
    ColorTheme old_theme;

    IMenuManager* m_menumgr; 
    Client* m_client;
    gui::IGUIEnvironment* env;
    MainMenuScripting* m_script = nullptr;

    float opening_animation_progress = 0.0;

    void updateAnimationProgress(float dtime);
    void setAnimationTarget(std::string id, double target);
    void setAnimationSpeed(std::string id, double speed);
    void setAnimationInstant(std::string id, double value);
    double getAnimation(std::string id);

    bool m_is_open = false; 
	bool m_is_main_menu = false;
    std::string last_theme_name = "";
    std::string last_scale_factor = "";

    double base_scaling_factor = 100.0;
    double active_scaling_factor = 0.0;
    double last_base_scaling_factor = 100.0;
    double target_base_scaling_factor = 100.0;
    double old_base_scaling_factor = 100.0;

    void calculateActiveScaling(s32 screen_width, s32 screen_height);

    double applyScalingFactorDouble(double value);

    s32 applyScalingFactorS32(s32 value);

    static float getDeltaTime();

    static std::chrono::high_resolution_clock::time_point lastTime;
    
    float m_category_scroll = 0.0f;
    float m_category_scroll_velocity = 0.0f;
    s32 m_last_mouse_x = 0;
    s32 m_last_mouse_y = 0;
    s32 m_drag_threshold = 10;
    bool m_dragging_category = false;
    core::vector2d<s32> m_mouse_down_pos;
    core::rect<s32> m_cat_bar_rect;

    core::vector2d<s32> m_current_mouse_pos;

    std::vector<double> m_animations;
    std::vector<double> m_animation_targets;
    std::vector<double> m_animation_speeds;
    std::vector<std::string> m_animation_ids;

    std::vector<core::rect<s32>> m_category_boxes;
    std::vector<std::string> m_category_names;

    float m_mods_scroll = 0.0f;
    float m_mods_scroll_velocity = 0.0f;
    bool m_dragging_mods = false;
    core::rect<s32> m_mods_list_rect;

    float m_settings_scroll = 0.0f;
    float m_settings_scroll_velocity = 0.0f;
    core::rect<s32> m_module_settings_rect;
    bool m_dragging_settings = false;
    bool m_sliding_slider = false;
    size_t m_sliding_slider_index = 0;
    bool m_selecting_dropdown = false;
    bool m_released_dropdown = false;
    size_t m_selecting_dropdown_index = 0;
    
    std::vector<core::rect<s32>> m_mods_boxes;
    std::vector<core::rect<s32>> m_mods_toggle_boxes;
    std::vector<std::string> m_mods_names;

    std::vector<core::rect<s32>> m_settings_toggle_boxes;
    std::vector<std::string> m_settings_toggle_names;

    std::vector<core::rect<s32>> m_settings_slider_boxes;
    std::vector<core::rect<s32>> m_settings_slider_knob_boxes;
    std::vector<std::string> m_settings_slider_names;

    std::vector<core::rect<s32>> m_settings_dropdown_boxes;
    std::vector<std::string> m_settings_dropdown_names;
    std::vector<core::rect<s32>> m_dropdown_option_boxes;
    std::vector<std::string> m_dropdown_option_names;

    std::unordered_map<std::string, TextboxData> m_settings_textboxes_map;

    core::rect<s32> current_path_rect;

    void draw_categories_bar(video::IVideoDriver* driver, core::rect<s32> clip, gui::IGUIFont* font, ModCategory* current_category, ColorTheme theme, std::vector<ModCategory*> categories, float dtime);
    void draw_mods_list(video::IVideoDriver* driver, core::rect<s32> clip, gui::IGUIFont* font, ModCategory* current_category, ColorTheme theme, float dtime);
    void draw_module_settings(video::IVideoDriver* driver, core::rect<s32> clip, core::rect<s32> topbar_clip, gui::IGUIFont* font, std::vector<ModCategory*> categories, ColorTheme theme, float dtime);
    void draw_dropdown_options(video::IVideoDriver* driver, gui::IGUIFont* font, ColorTheme theme, std::vector<ModCategory *> categories);
};

inline float easeInOutCubic(float t) {
    return (t < 0.5) ? 4 * t * t * t 
                     : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
}