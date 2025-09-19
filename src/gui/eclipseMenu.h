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

    bool m_is_open = false; 
	bool m_is_main_menu = false;

    static float getDeltaTime();

    static std::chrono::high_resolution_clock::time_point lastTime;
};