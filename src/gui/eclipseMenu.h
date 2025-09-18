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

using namespace gui;

#define GET_SCRIPT_POINTER                                                   \
    ClientScripting *script = m_client->getScript();                         \
    if (!script)                                 \
        return;

#define GET_SCRIPT_POINTER_S32                                               \
    ClientScripting *script = m_client->getScript();                         \
    if (!script)                                 \
        return s32(0);

#define GET_SCRIPT_POINTER_BOOL                                              \
    ClientScripting *script = m_client->getScript();                         \
    if (!script)                                 \
        return true;   

class EclipseMenu: public IGUIElement
{
public:
    EclipseMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent, s32 id, IMenuManager* menumgr, Client *client);

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

    bool m_is_open = false; 
	bool m_is_main_menu = false;

    static float getDeltaTime();

    static std::chrono::high_resolution_clock::time_point lastTime;
};