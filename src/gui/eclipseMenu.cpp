// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#include "eclipseMenu.h"
#include "client/fontengine.h"

std::chrono::high_resolution_clock::time_point EclipseMenu::lastTime = std::chrono::high_resolution_clock::now();

float EclipseMenu::getDeltaTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    return deltaTime.count();
}

EclipseMenu::EclipseMenu(
    gui::IGUIEnvironment *env, 
    gui::IGUIElement *parent, 
    s32 id, 
    IMenuManager *menumgr, 
    MainMenuScripting *script,
    bool is_main_menu
)
    : IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, core::rect<s32>(0, 0, 0, 0)), 
    m_menumgr(menumgr),
    m_script(script),
    m_is_main_menu(is_main_menu)
{
    infostream << "[EclipseMenu] Successfully created" << std::endl;
    this->env = env;
}

EclipseMenu::EclipseMenu(
	gui::IGUIEnvironment* env, 
    gui::IGUIElement* parent, 
    s32 id, 
	IMenuManager* menumgr, 
    Client* client,
    bool is_main_menu
)
    : IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, core::rect<s32>(0, 0, 0, 0)), 
    m_menumgr(menumgr),
    m_client(client),
    m_is_main_menu(is_main_menu)
{    
    infostream << "[EclipseMenu] Successfully created" << std::endl;
    this->env = env;
}

EclipseMenu::~EclipseMenu()
{
	infostream << "[EclipseMenu] Successfully deleted" << std::endl;
}

void EclipseMenu::create()
{
    GET_CATEGORIES_OR_RETURN(categories);

    if (!m_initialized) {
        themes_path = porting::path_user + DIR_DELIM + "themes";
        theme_manager = ThemeManager();
        theme_manager.LoadThemes(themes_path);
        current_theme_name = g_settings->get("ColorTheme");
        current_theme = theme_manager.GetThemeByName(current_theme_name);
		m_initialized = true;
	}

    core::rect<s32> screenRect(0, 0, 
        Environment->getVideoDriver()->getScreenSize().Width, 
        Environment->getVideoDriver()->getScreenSize().Height);

    setRelativePosition(screenRect);

    IGUIElement::setVisible(true);
    Environment->setFocus(this);
    m_menumgr->createdMenu(this);
    m_is_open = true;
}

void EclipseMenu::close()
{
    Environment->removeFocus(this);
    m_menumgr->deletingMenu(this);
    IGUIElement::setVisible(false);
    m_is_open = false;
}

bool EclipseMenu::OnEvent(const SEvent& event) 
{
    if (!m_is_open) {
        return false;
    }

    GET_CATEGORIES_OR_RETURN_BOOL(categories);

    if (event.EventType == EET_KEY_INPUT_EVENT)
    {
        if (event.KeyInput.Key == KEY_ESCAPE)
        {
            close();
            return true; 
        }
    }
    
    if (event.EventType == EET_MOUSE_INPUT_EVENT) {
        if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
            return false; 
        }
    }
    
    return Parent ? Parent->OnEvent(event) : false; 
}

void EclipseMenu::draw() 
{
    GET_CATEGORIES_OR_RETURN(categories);

	// Initialize some basic variables

    Environment->is_eclipse_menu_open = m_is_open;

    float dtime = getDeltaTime();

    video::IVideoDriver* driver = Environment->getVideoDriver();

    // const core::dimension2du screensize = driver->getScreenSize();

    gui::IGUIFont* font = g_fontengine->getFont(FONT_SIZE_UNSPECIFIED, FM_Standard);

	if (!font)
		return;
    
    if (m_is_open) {
        const s32 pad = 10;

        // Draw placeholder text with dtime
        std::wstring placeholder_text = std::wstring(L"Eclipse Menu [ dtime: ") +
                                        std::to_wstring((int)(dtime * 1000)) + L" ms]";
        core::dimension2du placeholder_size = font->getDimension(placeholder_text.c_str());
        core::rect<s32> placeholder_rect(
            pad, pad,
            pad + (s32)placeholder_size.Width,
            pad + (s32)placeholder_size.Height
        );

        driver->draw2DRectangle(current_theme.primary, placeholder_rect);
        font->draw(placeholder_text.c_str(), placeholder_rect, current_theme.text, true, true, &placeholder_rect);

        // Draw categories, mods, and settings
        s32 y = pad + placeholder_size.Height + pad;

        for (ModCategory* category : categories) {
            // Category info
            std::wstring category_text = std::wstring(L"Category: ") +
                                        core::stringw(category->m_name.c_str()).c_str();
            core::dimension2du cat_size = font->getDimension(category_text.c_str());
            core::rect<s32> cat_rect(pad, y, pad + (s32)cat_size.Width, y + (s32)cat_size.Height);
            font->draw(category_text.c_str(), cat_rect, current_theme.text, true, true);
            y += cat_size.Height + pad;

            for (Mod* mod : category->mods) {
                // Mod info
                std::wstring mod_default_str = mod->m_default ? L"true" : L"false";

                std::wstring mod_text = std::wstring(L"  Mod: ") +
                                        core::stringw(mod->m_name.c_str()).c_str() +
                                        L", ID: " + core::stringw(mod->m_setting_id.c_str()).c_str() +
                                        L", Desc: " + core::stringw(mod->m_description.c_str()).c_str() +
                                        L", Icon: " + core::stringw(mod->m_icon.c_str()).c_str() +
                                        L", Default: " + mod_default_str;
                core::dimension2du mod_size = font->getDimension(mod_text.c_str());
                core::rect<s32> mod_rect(pad, y, pad + (s32)mod_size.Width, y + (s32)mod_size.Height);
                font->draw(mod_text.c_str(), mod_rect, current_theme.text, true, true);
                y += mod_size.Height + pad;

                for (ModSetting* setting : mod->m_mod_settings) {
                    // Setting info (all fields)
                    std::wstring setting_text = std::wstring(L"    Setting: ") +
                                                core::stringw(setting->m_name.c_str()).c_str() +
                                                L", ID: " + core::stringw(setting->m_setting_id.c_str()).c_str() +
                                                L", Type: " + core::stringw(setting->m_type.c_str()).c_str() +
                                                L", Default: " + core::stringw(setting->m_default.c_str()).c_str() +
                                                L", Desc: " + core::stringw(setting->m_description.c_str()).c_str() +
                                                L", Min: " + std::wstring(core::stringw(std::to_string(setting->m_min).c_str()).c_str()) +
                                                L", Max: " + std::wstring(core::stringw(std::to_string(setting->m_max).c_str()).c_str()) +
                                                L", Steps: " + std::wstring(core::stringw(std::to_string(setting->m_steps).c_str()).c_str()) +
                                                L", Size: " + std::wstring(core::stringw(std::to_string(setting->m_size).c_str()).c_str());

                    // Options
                    if (!setting->m_options.empty()) {
                        setting_text += L", Options: ";
                        for (size_t i = 0; i < setting->m_options.size(); ++i) {
                            setting_text += core::stringw(setting->m_options[i]->c_str()).c_str();
                            if (i + 1 < setting->m_options.size()) setting_text += L", ";
                        }
                    }

                    core::dimension2du set_size = font->getDimension(setting_text.c_str());
                    core::rect<s32> set_rect(pad, y, pad + (s32)set_size.Width, y + (s32)set_size.Height);
                    font->draw(setting_text.c_str(), set_rect, current_theme.text, true, true);
                    y += set_size.Height + pad;
                }
            }
        }
    }
} 