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
	gui::IGUIEnvironment* env, 
    gui::IGUIElement* parent, 
    s32 id, 
	IMenuManager* menumgr, 
    Client* client
)
    : IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, core::rect<s32>(0, 0, 0, 0)), 
    m_menumgr(menumgr),
    m_client(client)
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
    //GET_SCRIPT_POINTER

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

    // GET_SCRIPT_POINTER_BOOL  

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
	if (!m_client || m_client == nullptr) {
		m_is_main_menu = true; // Make sure we know we are running in the main menu so some functionionality may need to be disabled.
	} else {
		GET_SCRIPT_POINTER // If we aren't in mainmenu, get the script pointer to csm
	}
		

	// Initialize some basic variables

    // float dtime = getDeltaTime();

    video::IVideoDriver* driver = Environment->getVideoDriver();

    const core::dimension2du screensize = driver->getScreenSize();

    gui::IGUIFont* font = g_fontengine->getFont(FONT_SIZE_UNSPECIFIED, FM_Standard);

	if (!font)
		return;
    
    if (m_is_open) {
		// Draw placeholder text to verify it's working
		std::wstring text = L"Eclipse Menu OPEN";
		core::dimension2du text_size = font->getDimension(text.c_str());

		const s32 pad = 10;

		s32 x1 = (screensize.Width  - (s32)text_size.Width)  / 2 - pad;
		s32 y1 = (screensize.Height - (s32)text_size.Height) / 2 - pad;
		s32 x2 = x1 + (s32)text_size.Width  + pad * 2;
		s32 y2 = y1 + (s32)text_size.Height + pad * 2;

		core::rect<s32> box_rect(x1, y1, x2, y2);

		driver->draw2DRectangle(current_theme.primary, box_rect);

		core::rect<s32> text_rect(
			x1, y1,
			x2, y2
		);

		font->draw(text.c_str(), text_rect, current_theme.text, true, true, &box_rect);
		return;
	}
} 