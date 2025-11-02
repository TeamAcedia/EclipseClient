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
    
    // In OnEvent
    if (event.EventType == EET_MOUSE_INPUT_EVENT) 
    {
        if (event.MouseInput.Event == EMIE_MOUSE_MOVED) 
        {
            m_current_mouse_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
        }

        if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && m_cat_bar_rect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
        {
            m_dragging_category = true;
            m_last_mouse_x = event.MouseInput.X;
            m_mouse_down_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
            m_category_scroll_velocity = 0.0f;
        }
        else if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && m_mods_list_rect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
        {
            m_dragging_mods = true;
            m_mods_last_mouse_y = event.MouseInput.Y;
            m_mouse_down_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
            m_mods_scroll_velocity = 0.0f;
        }
        else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) 
        {
            m_dragging_mods = false;
            m_dragging_category = false;

            s32 dx = abs(event.MouseInput.X - m_mouse_down_pos.X);
            s32 dy = abs(event.MouseInput.Y - m_mouse_down_pos.Y);

            if (dx < m_drag_threshold && dy < m_drag_threshold) 
            {
                // Handle click
                for (size_t i = 0; i < m_category_boxes.size(); ++i) 
                {
                    if (m_category_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                    {
                        m_mods_scroll = 0.0f; // reset mods scroll when changing category
                        m_mods_scroll_velocity = 0.0f;
                        g_settings->set("eclipse.current_category", m_category_names[i]);
                        break;
                    }
                }
            }
        }
        else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && m_dragging_category) 
        {
            s32 dx = event.MouseInput.X - m_last_mouse_x;
            m_category_scroll += dx;
            m_category_scroll_velocity = dx; // for momentum after release
            m_last_mouse_x = event.MouseInput.X;
        }
        else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && m_dragging_mods) 
        {
            s32 dy = event.MouseInput.Y - m_mods_last_mouse_y;
            m_mods_scroll += dy;
            m_mods_scroll_velocity = dy; // for momentum after release
            m_mods_last_mouse_y = event.MouseInput.Y;
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
    float start_opacity = 0.3f, // starting opacity
    core::rect<s32> *clip = 0
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
            radiusBottomLeft,
            clip
        );
    }

    // Draw main rectangle on top
    driver->draw2DRoundedRectangle(rect, color, radiusTopLeft, radiusTopRight, radiusBottomRight, radiusBottomLeft, clip);
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
    float anim_speed = 4.0f;
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

    // handle generic animations
    for (size_t i = 0; i < m_animations.size(); ++i) {
        float diff = m_animation_targets[i] - m_animations[i];
        float step = anim_speed * dtime;

        if (std::abs(diff) <= step) {
            // close enough to snap to target
            m_animations[i] = m_animation_targets[i];
        } else {
            // move toward target by step
            m_animations[i] += (diff > 0 ? step : -step);
        }
    }
}

void EclipseMenu::setAnimationTarget(std::string id, double target)
{
    for (size_t i = 0; i < m_animation_ids.size(); ++i) {
        if (m_animation_ids[i] == id) {
            m_animation_targets[i] = target;
            return;
        }
    }
    m_animation_ids.emplace_back(id);
    m_animations.emplace_back(target);
    m_animation_targets.emplace_back(target);
}

double EclipseMenu::getAnimation(std::string id)
{
    for (size_t i = 0; i < m_animation_ids.size(); ++i) {
        if (m_animation_ids[i] == id) {
            return m_animations[i];
        }
    }
	return 0.0;
}

video::SColor lerpColor(const video::SColor& start, const video::SColor& end, float progress)
{
    progress = std::clamp(progress, 0.0f, 1.0f);

    float a0 = static_cast<float>(start.getAlpha());
    float r0 = static_cast<float>(start.getRed());
    float g0 = static_cast<float>(start.getGreen());
    float b0 = static_cast<float>(start.getBlue());

    float a1 = static_cast<float>(end.getAlpha());
    float r1 = static_cast<float>(end.getRed());
    float g1 = static_cast<float>(end.getGreen());
    float b1 = static_cast<float>(end.getBlue());

    auto lerp = [&](float s, float e) {
        return s + (e - s) * progress;
    };

    float af = lerp(a0, a1);
    float rf = lerp(r0, r1);
    float gf = lerp(g0, g1);
    float bf = lerp(b0, b1);

    auto toByte = [](float v) -> u32 {
        return static_cast<u32>(std::round(std::clamp(v, 0.0f, 255.0f)));
    };

    u32 a = toByte(af);
    u32 r = toByte(rf);
    u32 g = toByte(gf);
    u32 b = toByte(bf);

    return video::SColor(a, r, g, b);
}

void EclipseMenu::draw_categories_bar(video::IVideoDriver* driver, core::rect<s32> clip, gui::IGUIFont* font, ModCategory* current_category, ColorTheme theme, std::vector<ModCategory*> categories, float dtime)
{
    m_category_boxes.clear();
    m_category_names.clear();

    const s32 tab_spacing = applyScalingFactorS32(15);
    const s32 category_tab_padding = applyScalingFactorS32(15);
    const s32 category_tab_height = clip.getHeight() - category_tab_padding;
    const s32 corner_radius = applyScalingFactorS32(10);

    s32 total_width = 0;

    // Loop through categories and calculate total width
    for (auto* cat : categories) {
        std::wstring wname = utf8_to_wide(cat->m_name);
        core::dimension2du text_size = font->getDimension(wname.c_str());
        s32 tab_width = text_size.Width + (category_tab_padding * 2);
        total_width += tab_width + tab_spacing;
    }

    float clip_w = static_cast<float>(clip.getWidth());
    float content_w = static_cast<float>(total_width);

    float left_bound = 0.0f;
    float right_bound = std::min(0.0f, clip_w - content_w);

    if (!m_dragging_category)
    {
        float overscroll_left = std::max(0.0f, m_category_scroll - left_bound);
        float overscroll_right = std::min(0.0f, m_category_scroll - right_bound);

        float spring_force = -overscroll_left - overscroll_right;

        float spring_strength = 0.02f;
        m_category_scroll_velocity += spring_force * spring_strength;

        m_category_scroll_velocity *= 0.92f;
        if (std::abs(m_category_scroll_velocity) < 0.1f)
            m_category_scroll_velocity = 0.0f;

        m_category_scroll += m_category_scroll_velocity * dtime * 10.0f;
    }

    s32 draw_x = ((clip.getWidth() / 2) - (total_width / 2)) + m_category_scroll;

    // Loop through categories and draw them
    for (auto* cat : categories) {
        std::wstring wname = utf8_to_wide(cat->m_name);
        core::dimension2du text_size = font->getDimension(wname.c_str());
        s32 tab_width = text_size.Width + (category_tab_padding * 2);

        core::rect<s32> tab_rect(
            clip.UpperLeftCorner.X + draw_x,
            clip.UpperLeftCorner.Y + (category_tab_padding / 2),
            clip.UpperLeftCorner.X + draw_x + tab_width,
            clip.UpperLeftCorner.Y + category_tab_height + (category_tab_padding / 2)
        );
        m_category_boxes.push_back(tab_rect);
        m_category_names.push_back(cat->m_name);

        // Draw tab background
        setAnimationTarget(cat->m_name + "_active", (cat == current_category) ? 1.0 : 0.0);
        video::SColor bg = lerpColor(theme.secondary, theme.primary, easeInOutCubic(getAnimation(cat->m_name + "_active")));
        drawRoundedRectShadow(driver, tab_rect, bg, corner_radius, corner_radius, corner_radius, corner_radius, 4, 6, 0.1f, &clip);

        setAnimationTarget(cat->m_name + "_hover", tab_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
        u32 highlight_alpha = (u32)(easeInOutCubic(getAnimation(cat->m_name + "_hover")) * 127);

        video::SColor highlight = theme.secondary_muted;
        highlight.setAlpha(highlight_alpha);
        driver->draw2DRoundedRectangle(tab_rect, highlight, corner_radius, &clip);

        // Draw tab text centered
        font->draw(
            wname.c_str(),
            tab_rect,
            theme.text,
            true, true, &clip
        );

        draw_x += tab_width + tab_spacing;
    }
}

void EclipseMenu::draw_mods_list(video::IVideoDriver *driver, core::rect<s32> clip, gui::IGUIFont *font, ModCategory *current_category, ColorTheme theme, float dtime)
{
    m_mods_boxes.clear();
    m_mods_names.clear();

    const s32 mod_padding = applyScalingFactorS32(15);
    const s32 num_mods_per_row = 3;
    const s32 total_padding = num_mods_per_row * 2 * mod_padding;
    const s32 mod_width = (clip.getWidth() - total_padding) / num_mods_per_row;
    const s32 mod_height = (mod_width * 0.75); // 4:3 aspect ratio
    const s32 corner_radius = applyScalingFactorS32(10);

    s32 x_index = 0;
    s32 y_index = 0;

    auto modules = current_category->mods;


    // Number of rows (ceil division)
    s32 num_rows = (modules.size() + num_mods_per_row - 1) / num_mods_per_row;
    s32 total_height = (num_rows * (mod_height + mod_padding)) + mod_padding;

    float clip_h = static_cast<float>(clip.getHeight());
    float content_h = static_cast<float>(total_height);

    float top_bound = 0.0f;
    float bottom_bound = std::min(0.0f, clip_h - content_h);

    if (!m_dragging_mods)
    {
        float overscroll_top = std::max(0.0f, m_mods_scroll - top_bound);
        float overscroll_bottom = std::min(0.0f, m_mods_scroll - bottom_bound);

        float spring_force = -overscroll_top - overscroll_bottom;

        float spring_strength = 0.05f;
        m_mods_scroll_velocity += spring_force * spring_strength;

        m_mods_scroll_velocity *= 0.92f;
        if (std::abs(m_mods_scroll_velocity) < 0.1f)
            m_mods_scroll_velocity = 0.0f;

        m_mods_scroll += m_mods_scroll_velocity * dtime * 10.0f;
    }

    s32 draw_x = clip.UpperLeftCorner.X + mod_padding;
    s32 draw_y = clip.UpperLeftCorner.Y + mod_padding + m_mods_scroll;

    // Loop through mods and draw them
    for (auto* mod : modules) {
        std::wstring wname = utf8_to_wide(mod->m_name);

        core::rect<s32> mod_rect(
            draw_x,
            draw_y,
            draw_x + mod_width,
            draw_y + mod_height
        );

        core::rect<s32> mod_text_rect(
            draw_x,
            draw_y + mod_height * 0.6,
            draw_x + mod_width,
            draw_y + mod_height * 0.8
        );

        core::rect<s32> mod_enabled_rect(
            draw_x,
            draw_y + mod_height * 0.8,
            draw_x + mod_width,
            draw_y + mod_height
        );

        m_mods_boxes.push_back(mod_rect);
        m_mods_names.push_back(mod->m_name);

        // Draw mod background
        setAnimationTarget(mod->m_name + "_active", (mod->is_enabled()) ? 1.0 : 0.0);
        video::SColor bg = lerpColor(theme.secondary, theme.primary, easeInOutCubic(getAnimation(mod->m_name + "_active")));
        drawRoundedRectShadow(driver, mod_rect, bg, corner_radius, corner_radius, corner_radius, corner_radius, 4, 6, 0.1f, &clip);

        setAnimationTarget(mod->m_name + "_hover", mod_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
        u32 highlight_alpha = (u32)(easeInOutCubic(getAnimation(mod->m_name + "_hover")) * 127);

        video::SColor highlight = theme.secondary_muted;
        highlight.setAlpha(highlight_alpha);
        driver->draw2DRoundedRectangle(mod_rect, highlight, corner_radius, &clip);

        // Draw mod enabled background
        video::SColor enabled_bg = mod->is_enabled() ? theme.enabled: theme.disabled;
        drawRoundedRectShadow(driver, mod_enabled_rect, enabled_bg, corner_radius / 2, corner_radius / 2, corner_radius / 2, corner_radius / 2, 2, 4, 0.1f, &clip);

        // Draw mod enabled text centered
        std::wstring wenabled = mod->is_enabled() ? utf8_to_wide("Enabled") : utf8_to_wide("Disabled");
        font->draw(
            wenabled.c_str(),
            mod_enabled_rect,
            theme.text,
            true, true, &clip
        );

        // Draw mod text centered
        font->draw(
            wname.c_str(),
            mod_text_rect,
            theme.text,
            true, true, &clip
        );

        x_index++;
        if (x_index >= num_mods_per_row) {
            x_index = 0;
            y_index++;
        }
        draw_x = clip.UpperLeftCorner.X + mod_padding + (x_index * (mod_width + (mod_padding * 2)));
        draw_y = clip.UpperLeftCorner.Y + mod_padding + (y_index * (mod_height + mod_padding)) + m_mods_scroll;
    }
}

void EclipseMenu::draw() 
{
    GET_CATEGORIES_OR_RETURN(categories);

    g_settings->setDefault("eclipse.current_category", categories[0]->m_name);
    std::string current_category_name = g_settings->get("eclipse.current_category");
    ModCategory* current_category = nullptr;

    // Find category by name
    for (auto* cat : categories) {
        if (cat->m_name == current_category_name) {
            current_category = cat;
            break;
        }
    }

    if (!current_category) {
        current_category = categories[0];
        g_settings->set("eclipse.current_category", current_category->m_name);
    }

    // Initialize some basic variables

    Environment->is_eclipse_menu_open = m_is_open;

    if (m_is_open) { // Ensure we always have focus when this window is open, this is dumb but it stops the client from closing when you press escape in main menu with eclipse menu open
        Environment->setFocus(this);
    }

    float dtime = getDeltaTime();

    updateAnimationProgress(dtime);

    float eased_opening_progress = easeInOutCubic(opening_animation_progress);

    video::IVideoDriver* driver = Environment->getVideoDriver();

    const core::dimension2du screensize = driver->getScreenSize();

    setRelativePosition(core::rect<s32>(0, 0, screensize.Width, screensize.Height));

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

        m_cat_bar_rect = core::rect<s32>(
            mods_topbar_rect.UpperLeftCorner.X + corner_radius,
            mods_topbar_rect.UpperLeftCorner.Y,
            mods_topbar_rect.LowerRightCorner.X - corner_radius,
            mods_topbar_rect.LowerRightCorner.Y
        );

        m_mods_list_rect = core::rect<s32>(
            menurect.LowerRightCorner.X - modssize.Width,       //x1 ( Upper left )
            m_cat_bar_rect.LowerRightCorner.Y + section_gap,    //y1
            menurect.LowerRightCorner.X,                        //x2 ( Lower right )
            menurect.UpperLeftCorner.Y + modssize.Height        //y2
        );

        drawRoundedRectShadow(driver, menurect, theme.background_bottom, corner_radius, corner_radius, corner_radius, corner_radius, 4, 6, 0.5f); // Main background
        driver->draw2DRoundedRectangle(profilesrect, theme.background, corner_radius, 0, 0, corner_radius); // Profiles background
        drawRoundedRectShadow(driver, profiles_topbar_rect, theme.background_top, corner_radius, 0, 0, corner_radius, 4, 6, 0.1f); // Profiles top bar
        driver->draw2DRoundedRectangle(modsrect, theme.background, 0, corner_radius, corner_radius, 0); // Mods background
        drawRoundedRectShadow(driver, mods_topbar_rect, theme.background_top, 0, corner_radius, corner_radius, 0, 4, 6, 0.1f); // Mods top bar

        draw_categories_bar(driver, m_cat_bar_rect, font, current_category, theme, categories, dtime);
        draw_mods_list(driver, m_mods_list_rect, font, current_category, theme, dtime);
    }
} 