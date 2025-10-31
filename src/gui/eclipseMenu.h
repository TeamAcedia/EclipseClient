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

class EclipseMenu: public IGUIElement
{
public:
    EclipseMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id, IMenuManager* menumgr, Client *client, bool is_main_menu);
    EclipseMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id, IMenuManager* menumgr, MainMenuScripting *script, bool is_main_menu);

    void create();
    void close();

    virtual bool OnEvent(const SEvent& event) override;
    virtual void draw() override;
    bool isOpen() { return m_is_open; }
    
    ~EclipseMenu();

    bool m_initialized = false;
private:
    std::string themes_path;
    ThemeManager theme_manager;
    std::string current_theme_name;
    ColorTheme current_theme;

    IMenuManager* m_menumgr; 
    Client* m_client;
    gui::IGUIEnvironment* env;
    MainMenuScripting* m_script = nullptr;

    float opening_animation_progress = 0.0;

    void updateAnimationProgress(float dtime);
    void setAnimationTarget(std::string id, double target);
    double getAnimation(std::string id);

    bool m_is_open = false; 
	bool m_is_main_menu = false;

    double base_scaling_factor = 100.0;
    double active_scaling_factor = 0.0;

    void calculateActiveScaling(s32 screen_width, s32 screen_height);

    double applyScalingFactorDouble(double value);

    s32 applyScalingFactorS32(s32 value);

    static float getDeltaTime();

    static std::chrono::high_resolution_clock::time_point lastTime;
    
    float m_category_scroll = 0.0f;
    float m_category_scroll_velocity = 0.0f;
    s32 m_last_mouse_x = 0;
    s32 m_drag_threshold = 10;
    bool m_dragging_category = false;
    core::vector2d<s32> m_mouse_down_pos;
    core::rect<s32> m_cat_bar_rect;

    core::vector2d<s32> m_current_mouse_pos;

    std::vector<double> m_animations;
    std::vector<double> m_animation_targets;
    std::vector<std::string> m_animation_ids;

    std::vector<core::rect<s32>> m_category_boxes;
    std::vector<std::string> m_category_names;

    float m_mods_scroll = 0.0f;
    float m_mods_scroll_velocity = 0.0f;
    s32 m_mods_last_mouse_y = 0;
    bool m_dragging_mods = false;
    core::rect<s32> m_mods_list_rect;
    
    std::vector<core::rect<s32>> m_mods_boxes;
    std::vector<std::string> m_mods_names;

    void draw_categories_bar(video::IVideoDriver* driver, core::rect<s32> clip, gui::IGUIFont* font, ModCategory* current_category, ColorTheme theme, std::vector<ModCategory*> categories, float dtime);
    void draw_mods_list(video::IVideoDriver* driver, core::rect<s32> clip, gui::IGUIFont* font, ModCategory* current_category, ColorTheme theme, float dtime);
};

inline float easeInOutCubic(float t) {
    return (t < 0.5) ? 4 * t * t * t 
                     : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
}