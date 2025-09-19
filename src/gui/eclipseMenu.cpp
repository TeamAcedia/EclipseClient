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
        const s32 pad = 25;

        // --- First, measure everything to get total bounds ---
        s32 total_height = pad; 
        s32 max_width = 0;

        struct LayoutItem {
            s32 y_top;
            s32 height;
            ModCategory* category = nullptr;
            Mod* mod = nullptr;
            ModSetting* setting = nullptr;
        };
        core::array<LayoutItem> layout_items;

        // Placeholder text
        std::wstring placeholder_text = std::wstring(L"Eclipse Menu [ dtime: ") +
                                        std::to_wstring((int)(dtime * 1000)) + L" ms]";
        core::dimension2du placeholder_size = font->getDimension(placeholder_text.c_str());
        total_height += placeholder_size.Height + pad;
        max_width = core::max_(max_width, (s32)placeholder_size.Width);

        // Categories, mods, settings
        for (ModCategory* category : categories) {
            core::dimension2du cat_size = font->getDimension(
                (std::wstring(L"Category: ") + core::stringw(category->m_name.c_str()).c_str()).c_str());

            LayoutItem cat_item;
            cat_item.y_top = total_height;
            cat_item.height = cat_size.Height;
            cat_item.category = category;
            cat_item.mod = nullptr;
            cat_item.setting = nullptr;
            layout_items.push_back(cat_item);

            total_height += cat_size.Height + pad;
            max_width = core::max_(max_width, (s32)cat_size.Width);

            for (Mod* mod : category->mods) {
                core::dimension2du mod_size = font->getDimension(
                    (std::wstring(L"  Mod: ") + core::stringw(mod->m_name.c_str()).c_str()).c_str());

                LayoutItem mod_item;
                mod_item.y_top = total_height;
                mod_item.height = mod_size.Height;
                mod_item.category = nullptr;
                mod_item.mod = mod;
                mod_item.setting = nullptr;
                layout_items.push_back(mod_item);

                total_height += mod_size.Height + pad;
                max_width = core::max_(max_width, (s32)mod_size.Width);

                for (ModSetting* setting : mod->m_mod_settings) {
                    core::dimension2du set_size = font->getDimension(
                        (std::wstring(L"    Setting: ") + core::stringw(setting->m_name.c_str()).c_str()).c_str());

                    LayoutItem set_item;
                    set_item.y_top = total_height;
                    set_item.height = set_size.Height;
                    set_item.category = nullptr;
                    set_item.mod = nullptr;
                    set_item.setting = setting;
                    layout_items.push_back(set_item);

                    total_height += set_size.Height + pad;
                    max_width = core::max_(max_width, (s32)set_size.Width);
                }
            }
        }

        // --- Centering ---
        s32 screen_width = driver->getScreenSize().Width;
        s32 screen_height = driver->getScreenSize().Height;
        s32 offset_x = (screen_width - (max_width + pad*2)) / 2;
        s32 offset_y = (screen_height - total_height) / 2;

        // --- Draw overall background ---
        core::rect<s32> bg_rect(offset_x, offset_y, offset_x + pad + max_width + pad, offset_y + total_height);
        driver->draw2DRoundedRectangle(bg_rect, current_theme.background_bottom, 20);
        driver->draw2DRoundedRectangleOutline(bg_rect, current_theme.border, 2, 20);

        // --- Draw placeholder text ---
        core::rect<s32> placeholder_rect(offset_x + pad, offset_y + pad,
                                        offset_x + pad + (s32)placeholder_size.Width,
                                        offset_y + pad + (s32)placeholder_size.Height);
        font->draw(placeholder_text.c_str(), placeholder_rect, current_theme.text, true, true, &placeholder_rect);

        // --- Draw layout items with category/mod boxes ---
        s32 y = offset_y + pad + placeholder_size.Height + pad;
        for (u32 idx = 0; idx < layout_items.size(); ++idx) {
            LayoutItem& item = layout_items[idx];
            if (item.category) {
                // Draw a big box covering the category + all mods/settings
                s32 cat_top = y;
                s32 cat_bottom = y;
                // Find the bottom of this category by looking ahead in layout_items
                for (u32 i = idx + 1; i < layout_items.size(); ++i) {
                    if (layout_items[i].category) break;
                    cat_bottom = offset_y + layout_items[i].y_top + layout_items[i].height + pad;
                }
                core::rect<s32> cat_box(offset_x + pad/2, cat_top, offset_x + pad + max_width + pad/2, cat_bottom);
                driver->draw2DRoundedRectangle(cat_box, current_theme.background, 20);
                driver->draw2DRoundedRectangleOutline(cat_box, current_theme.border, 2, 20);

                // Draw category text
                core::dimension2du cat_size = font->getDimension(
                    (std::wstring(L"Category: ") + core::stringw(item.category->m_name.c_str()).c_str()).c_str());
                core::rect<s32> cat_text_rect(offset_x + pad, y, offset_x + pad + (s32)cat_size.Width, y + (s32)cat_size.Height);
                font->draw((std::wstring(L"Category: ") + core::stringw(item.category->m_name.c_str()).c_str()).c_str(),
                        cat_text_rect, current_theme.text, true, true);
                y += cat_size.Height + pad;
            } 
            else if (item.mod) {
                // Draw medium box for mod + settings
                s32 mod_top = y;
                s32 mod_bottom = mod_top;
                // Look ahead for settings
                for (u32 i = idx + 1; i < layout_items.size(); ++i) {
                    if (layout_items[i].category || layout_items[i].mod) break;
                    mod_bottom = offset_y + layout_items[i].y_top + layout_items[i].height + pad;
                }
                core::rect<s32> mod_box(offset_x + pad, mod_top, offset_x + pad + max_width, mod_bottom);
                driver->draw2DRoundedRectangle(mod_box, current_theme.background_top, 15);
                driver->draw2DRoundedRectangleOutline(mod_box, current_theme.border, 1, 15);

                // Draw mod text
                core::dimension2du mod_size = font->getDimension(
                    (std::wstring(L"  Mod: ") + core::stringw(item.mod->m_name.c_str()).c_str()).c_str());
                core::rect<s32> mod_text_rect(offset_x + pad, y, offset_x + pad + (s32)mod_size.Width, y + (s32)mod_size.Height);
                font->draw((std::wstring(L"  Mod: ") + core::stringw(item.mod->m_name.c_str()).c_str()).c_str(),
                        mod_text_rect, current_theme.text, true, true);
                y += mod_size.Height + pad;
            } 
            else if (item.setting) {
                // Just draw setting text
                core::dimension2du set_size = font->getDimension(
                    (std::wstring(L"    Setting: ") + core::stringw(item.setting->m_name.c_str()).c_str()).c_str());
                core::rect<s32> set_rect(offset_x + pad, y, offset_x + pad + (s32)set_size.Width, y + (s32)set_size.Height);
                font->draw((std::wstring(L"    Setting: ") + core::stringw(item.setting->m_name.c_str()).c_str()).c_str(),
                        set_rect, current_theme.text, true, true);
                y += set_size.Height + pad;
            }
        }
    }

} 