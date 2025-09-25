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

void EclipseMenu::calculateActiveScaling(s32 screen_width, s32 screen_height) {
    // Reference resolution (designed for 2560x1440 @ 100% scale)
    const double reference_width  = 2560.0;
    const double reference_height = 1440.0;

    double scale_x = static_cast<double>(screen_width)  / reference_width;
    double scale_y = static_cast<double>(screen_height) / reference_height;

    double resolution_scale = std::min(scale_x, scale_y);

    active_scaling_factor = base_scaling_factor * resolution_scale;
}


void EclipseMenu::create()
{
    GET_CATEGORIES_OR_RETURN(categories);

    if (!m_initialized) {
        // Load client theming
        themes_path = porting::path_user + DIR_DELIM + "themes";
        theme_manager = ThemeManager();
        theme_manager.LoadThemes(themes_path);
        current_theme_name = g_settings->get("eclipse_appearance.theme");
        current_theme = theme_manager.GetThemeByName(current_theme_name);

        // Load client scaling
        std::string scaling_factor_str = g_settings->get("eclipse_appearance.menu_scale");
        if (!scaling_factor_str.empty() && scaling_factor_str.back() == '%') {
            scaling_factor_str.pop_back();
        }
        base_scaling_factor = std::stod(scaling_factor_str) / 100.0;

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

// Simple helper to draw a rounded rectangle with a drop shadow
void drawRoundedRectShadow(
    video::IVideoDriver* driver,
    const core::rect<s32>& rect,
    const video::SColor& color,
    s32 radiusTopLeft,
    s32 radiusTopRight,
    s32 radiusBottomLeft,
    s32 radiusBottomRight,
    s32 shadow_offset_y = 4,   // move shadow down
    s32 shadow_layers = 6,     // more layers = softer
    float start_opacity = 0.3f // starting opacity
) {
    // Draw shadow layers
    for (s32 i = 0; i < shadow_layers; ++i) {
        float alpha_factor = start_opacity * (1.0f - (float)i / shadow_layers);

        video::SColor layer_color = video::SColor(255, 5, 5, 5);
        layer_color.setAlpha(static_cast<u8>(layer_color.getAlpha() * alpha_factor));

        // Offset downward slightly per layer
        core::rect<s32> shadow_rect = rect;
        shadow_rect.UpperLeftCorner.Y += shadow_offset_y + i;
        shadow_rect.LowerRightCorner.Y += shadow_offset_y + i;

        driver->draw2DRoundedRectangle(
            shadow_rect,
            layer_color,
            radiusTopLeft,
            radiusTopRight,
            radiusBottomRight,
            radiusBottomLeft
        );
    }

    // Draw main rectangle on top
    driver->draw2DRoundedRectangle(rect, color, radiusTopLeft, radiusTopRight, radiusBottomRight, radiusBottomLeft);
}

// Simple helper to apply scaling factor
s32 EclipseMenu::applyScalingFactorS32(s32 value) {
    return static_cast<s32>(value * active_scaling_factor);
}

double EclipseMenu::applyScalingFactorDouble(double value) {
    return value * active_scaling_factor;
}

void EclipseMenu::updateAnimationProgress(float dtime) {
    float speed = 4.0f;
    if (m_is_open) {
        opening_animation_progress += speed * dtime;
        if (opening_animation_progress > 0.99) {
            opening_animation_progress = 1;
        }
    } else {
        opening_animation_progress -= speed * dtime;
        if (opening_animation_progress < 0.01) {
            opening_animation_progress = 0;
        }
    }

    // Clamp to 0-1
    opening_animation_progress = std::clamp(opening_animation_progress, 0.0f, 1.0f);
}

void EclipseMenu::draw() 
{
    GET_CATEGORIES_OR_RETURN(categories);

    // Initialize some basic variables

    Environment->is_eclipse_menu_open = m_is_open;

    float dtime = getDeltaTime();

    updateAnimationProgress(dtime);

    float eased_opening_progress = easeInOutCubic(opening_animation_progress);

    video::IVideoDriver* driver = Environment->getVideoDriver();

    const core::dimension2du screensize = driver->getScreenSize();

    gui::IGUIFont* font = g_fontengine->getFont(applyScalingFactorS32(24), FM_Standard);


    if (!font)
        return;
    
    if (opening_animation_progress != 0) { // Menu is open
        ColorTheme theme = current_theme.withAlpha(eased_opening_progress);
        calculateActiveScaling(screensize.Width, screensize.Height);

        const core::dimension2di menusize(applyScalingFactorS32(1200), applyScalingFactorS32(static_cast<s32>(700 * eased_opening_progress)));

        const core::rect<s32> menurect(
            (screensize.Width / 2) - (menusize.Width / 2),      //x1
            (screensize.Height / 2) - (menusize.Height / 2),    //y1
            (screensize.Width / 2) + (menusize.Width / 2),      //x2
            (screensize.Height / 2) + (menusize.Height / 2)     //y2
        );

        const s32 section_gap = applyScalingFactorS32(15);
        const s32 corner_radius = applyScalingFactorS32(25);
        const s32 top_bar_height = applyScalingFactorS32(55);

        const core::dimension2di profilessize((menusize.Width * 0.25) - (section_gap / 2), menusize.Height - 1);

        const core::rect<s32> profilesrect(
            menurect.UpperLeftCorner.X,                         //x1 ( Upper left )
            menurect.UpperLeftCorner.Y,                         //y1
            menurect.UpperLeftCorner.X + profilessize.Width,    //x2 ( Lower right )
            menurect.UpperLeftCorner.Y + profilessize.Height    //y2
        );

        const core::rect<s32> profiles_topbar_rect(
            profilesrect.UpperLeftCorner.X,                         //x1 ( Upper left )
            profilesrect.UpperLeftCorner.Y,                         //y1
            profilesrect.LowerRightCorner.X,                        //x2 ( Lower right )
            profilesrect.UpperLeftCorner.Y + top_bar_height         //y2
        ); 

        const core::dimension2di modssize((menusize.Width * 0.75) - (section_gap / 2), menusize.Height - 1);

        const core::rect<s32> modsrect(
            menurect.LowerRightCorner.X - modssize.Width,       //x1 ( Upper left )
            menurect.UpperLeftCorner.Y,                         //y1
            menurect.LowerRightCorner.X,                        //x2 ( Lower right )
            menurect.UpperLeftCorner.Y + modssize.Height        //y2
        );

        const core::rect<s32> mods_topbar_rect(
            modsrect.UpperLeftCorner.X,                         //x1 ( Upper left )
            modsrect.UpperLeftCorner.Y,                         //y1
            modsrect.LowerRightCorner.X,                        //x2 ( Lower right )
            modsrect.UpperLeftCorner.Y + top_bar_height         //y2
        ); 

        drawRoundedRectShadow(driver, menurect, theme.background_bottom, corner_radius, corner_radius, corner_radius, corner_radius, 4, 6, 0.5f); // Main background
        driver->draw2DRoundedRectangle(profilesrect, theme.background, corner_radius, 0, 0, corner_radius); // Profiles background
        drawRoundedRectShadow(driver, profiles_topbar_rect, theme.background_top, corner_radius, 0, 0, corner_radius, 4, 6, 0.1f); // Profiles top bar
        driver->draw2DRoundedRectangle(modsrect, theme.background, 0, corner_radius, corner_radius, 0); // Mods background
        drawRoundedRectShadow(driver, mods_topbar_rect, theme.background_top, 0, corner_radius, corner_radius, 0, 4, 6, 0.1f); // Mods top bar
    }
} 