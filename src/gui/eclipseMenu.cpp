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

double roundToNearestStep(double value, double min, double max, double steps) {
    if (steps <= 1.0) {
        return value; // No rounding needed
    }

    double step_size = (max - min) / (steps - 1);
    double rounded_value = std::round((value - min) / step_size) * step_size + min;

    // Clamp to min and max
    if (rounded_value < min) {
        rounded_value = min;
    } else if (rounded_value > max) {
        rounded_value = max;
    }

    return rounded_value;
}

double mapValue(double value, double oldMin, double oldMax, double newMin, double newMax) {
    return newMin + (value - oldMin) * (newMax - newMin) / (oldMax - oldMin);
}

double calculateSliderValueFromPosition(const core::rect<s32>& sliderBarRect, const core::position2d<s32>& pointerPosition, double m_min, double m_max, double m_steps)
{

    s32 clampedX = std::clamp(pointerPosition.X, sliderBarRect.UpperLeftCorner.X, sliderBarRect.LowerRightCorner.X);


    double sliderValue = mapValue(clampedX, sliderBarRect.UpperLeftCorner.X, sliderBarRect.LowerRightCorner.X, m_min, m_max);

    return roundToNearestStep(sliderValue, m_min, m_max, m_steps);
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

ColorTheme lerpTheme(const ColorTheme& a, const ColorTheme& b, float t) {
    ColorTheme result;
    result.background = lerpColor(a.background, b.background, t);
    result.background_bottom = lerpColor(a.background_bottom, b.background_bottom, t);
    result.background_top = lerpColor(a.background_top, b.background_top, t);
    result.border = lerpColor(a.border, b.border, t);
    result.enabled = lerpColor(a.enabled, b.enabled, t);
    result.disabled = lerpColor(a.disabled, b.disabled, t);
    result.primary = lerpColor(a.primary, b.primary, t);
    result.primary_muted = lerpColor(a.primary_muted, b.primary_muted, t);
    result.secondary = lerpColor(a.secondary, b.secondary, t);
    result.secondary_muted = lerpColor(a.secondary_muted, b.secondary_muted, t);
    result.text = lerpColor(a.text, b.text, t);
    result.text_muted = lerpColor(a.text_muted, b.text_muted, t);
    result.wallpaper = lerpColor(a.wallpaper, b.wallpaper, t);
    return result;
}

void EclipseMenu::updateTheming()
{
    if (g_settings->get("eclipse_appearance.theme") != last_theme_name) {
        old_theme = target_theme;
        current_theme_name = g_settings->get("eclipse_appearance.theme");
        target_theme = theme_manager.GetThemeByName(current_theme_name);
        last_theme_name = current_theme_name;
        setAnimationInstant("theme_transition", 0.0);
        setAnimationTarget("theme_transition", 1.0);
    }

    double theme_transition = getAnimation("theme_transition");
    current_theme = lerpTheme(old_theme, target_theme, theme_transition);
}

void EclipseMenu::updateScaling()
{
    std::string scaling_factor_str = g_settings->get("eclipse_appearance.menu_scale");
    if (!scaling_factor_str.empty() && scaling_factor_str.back() == '%') {
        scaling_factor_str.pop_back();
    }
    target_base_scaling_factor = std::stod(scaling_factor_str) / 100.0;
    if (target_base_scaling_factor != last_base_scaling_factor) {
        old_base_scaling_factor = last_base_scaling_factor;
        last_base_scaling_factor = target_base_scaling_factor;

        setAnimationInstant("scaling_transition", 0.0);
        setAnimationTarget("scaling_transition", 1.0);
    }

    double scaling_transition = easeInOutCubic(getAnimation("scaling_transition"));

    base_scaling_factor = old_base_scaling_factor + (target_base_scaling_factor - old_base_scaling_factor) * scaling_transition;

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
        target_theme = current_theme;
        last_theme_name = current_theme_name;
        old_theme = current_theme;
        setAnimationInstant("theme_transition", 1.0);

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
        if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown)
        {
            if (m_selecting_dropdown) {
                // Close dropdown if open
                for (auto cat : categories) {
                    for (auto mod : cat->mods) {
                        if (mod->m_name == g_settings->get("eclipse.current_module")) {
                            for (auto setting : mod->m_mod_settings) {
                                if (setting->m_name == m_settings_dropdown_names[m_selecting_dropdown_index]) {
                                    setAnimationTarget(setting->m_name + "_dropdown_open", 0.0);
                                    m_selecting_dropdown = false;
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
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
            for (size_t i = 0; i < m_mods_toggle_boxes.size(); ++i) 
            {
                if (!m_mods_toggle_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                {
                    setAnimationTarget(m_mods_names[i] + "_toggle_pressed", 0.0);
                }
            }
            for (size_t i = 0; i < m_settings_toggle_boxes.size(); ++i) 
            {
                if (!m_settings_toggle_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                {
                    setAnimationTarget(m_settings_toggle_names[i] + "_toggle_pressed", 0.0);
                }
            }
        }
        if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && m_selecting_dropdown) {
            bool option_clicked = false;
            for (size_t i = 0; i < m_dropdown_option_boxes.size(); ++i) 
            {
                if (m_dropdown_option_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                {
                    for (auto cat : categories) {
                        for (auto mod : cat->mods) {
                            if (mod->m_name == g_settings->get("eclipse.current_module")) {
                                for (auto setting : mod->m_mod_settings) {
                                    if (setting->m_name == m_settings_dropdown_names[m_selecting_dropdown_index]) {
                                        g_settings->set(setting->m_setting_id, m_dropdown_option_names[i]);
                                        setAnimationTarget(setting->m_name + "_dropdown_open", 0.0);
                                        m_selecting_dropdown = false;
                                        option_clicked = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!option_clicked) {
                // Clicked outside options, close dropdown
                for (auto cat : categories) {
                    for (auto mod : cat->mods) {
                        if (mod->m_name == g_settings->get("eclipse.current_module")) {
                            for (auto setting : mod->m_mod_settings) {
                                if (setting->m_name == m_settings_dropdown_names[m_selecting_dropdown_index]) {
                                    setAnimationTarget(setting->m_name + "_dropdown_open", 0.0);
                                    m_selecting_dropdown = false;
                                    m_released_dropdown = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }
        if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && m_cat_bar_rect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
        {
            m_dragging_category = true;
            m_last_mouse_x = event.MouseInput.X;
            m_mouse_down_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
            m_category_scroll_velocity = 0.0f;
        }
        if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && m_mods_list_rect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
        {
            m_dragging_mods = true;
            m_last_mouse_y = event.MouseInput.Y;
            m_mouse_down_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
            m_mods_scroll_velocity = 0.0f;
        }
        if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && m_module_settings_rect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y)))
        {
            bool dragging_handled = false;
            for (size_t i = 0; i < m_settings_slider_boxes.size(); ++i) 
            {
                if (m_settings_slider_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y)) ||
                    m_settings_slider_knob_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                {
                    m_sliding_slider = true;
                    m_sliding_slider_index = i;
                    for (auto cat : categories) {
                        for (auto mod : cat->mods) {
                            if (mod->m_name == g_settings->get("eclipse.current_module")) {
                                for (auto setting : mod->m_mod_settings) {
                                    if (setting->m_name == m_settings_slider_names[i]) {
                                        setAnimationTarget(setting->m_name + "_slider_knob_pressed", 1.0);
                                        g_settings->setFloat(
                                            setting->m_setting_id,
                                            static_cast<float>(calculateSliderValueFromPosition(
                                                    m_settings_slider_boxes[i],
                                                    core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y),
                                                    setting->m_min,
                                                    setting->m_max,
                                                    setting->m_steps
                                                )
                                            )
                                        );
                                        dragging_handled = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (dragging_handled)
                return true;
            m_dragging_settings = true;
            m_last_mouse_y = event.MouseInput.Y;
            m_mouse_down_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
            m_settings_scroll_velocity = 0.0f;
        }
        if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) 
        {
            m_sliding_slider = false;
            m_dragging_mods = false;
            m_dragging_category = false;
            m_dragging_settings = false;
            if (m_released_dropdown) {
                m_released_dropdown = false;
                return true;
            }
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
                bool clicked_toggle = false;
                for (size_t i = 0; i < m_mods_toggle_boxes.size(); ++i) 
                {
                    if (m_mods_toggle_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                    {
                        setAnimationTarget(m_mods_names[i] + "_toggle_pressed", 0.0);
                        std::string current_category_name = g_settings->get("eclipse.current_category");
                        for (auto cat : categories) {
                            if (cat->m_name == current_category_name) {
                                for (auto mod : cat->mods) {
                                    if (mod->m_name == m_mods_names[i]) {
                                        mod->toggle();
                                        clicked_toggle = true;
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                if (!clicked_toggle) {
                    for (size_t i = 0; i < m_mods_boxes.size(); ++i) 
                    {
                        if (m_mods_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                        {
                            std::string current_category_name = g_settings->get("eclipse.current_category");
                            for (auto cat : categories) {
                                if (cat->m_name == current_category_name) {
                                    for (auto mod : cat->mods) {
                                        if (mod->m_name == m_mods_names[i]) {
                                            g_settings->set("eclipse.current_module", mod->m_name);
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }   
            
                for (size_t i = 0; i < m_settings_toggle_boxes.size(); ++i) 
                {
                    if (m_settings_toggle_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                    {
                        setAnimationTarget(m_settings_toggle_names[i] + "_toggle_pressed", 0.0);
                        std::string current_module_name = g_settings->get("eclipse.current_module");
                        for (auto cat : categories) {
                            for (auto mod : cat->mods) {
                                if (mod->m_name == current_module_name) {
                                    for (auto setting : mod->m_mod_settings) {
                                        if (setting->m_name == m_settings_toggle_names[i]) {
                                            setting->toggle();
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                for (size_t i = 0; i < m_settings_dropdown_boxes.size(); ++i) 
                {
                    if (m_settings_dropdown_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                    {
                        std::string current_module_name = g_settings->get("eclipse.current_module");
                        for (auto cat : categories) {
                            for (auto mod : cat->mods) {
                                if (mod->m_name == current_module_name) {
                                    for (auto setting : mod->m_mod_settings) {
                                        if (setting->m_name == m_settings_dropdown_names[i]) {
                                            if (m_selecting_dropdown) {
                                                m_selecting_dropdown = false;
                                            } else {
                                                m_selecting_dropdown = true;
                                                m_selecting_dropdown_index = i;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) 
        {
            for (size_t i = 0; i < m_mods_toggle_boxes.size(); ++i) 
            {
                setAnimationTarget(m_mods_names[i] + "_toggle_pressed", m_mods_toggle_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y)) ? 1.0 : 0.0);
            }
            for(size_t i = 0; i < m_settings_toggle_boxes.size(); ++i) 
            {
                setAnimationTarget(m_settings_toggle_names[i] + "_toggle_pressed", m_settings_toggle_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y)) ? 1.0 : 0.0);
            }
            if(current_path_rect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
            {
                g_settings->set("eclipse.current_module", "");
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
            s32 dy = event.MouseInput.Y - m_last_mouse_y;
            m_mods_scroll += dy;
            m_mods_scroll_velocity = dy; // for momentum after release
            m_last_mouse_y = event.MouseInput.Y;
        }
        else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && m_dragging_settings) 
        {
            s32 dy = event.MouseInput.Y - m_last_mouse_y;
            m_settings_scroll += dy;
            m_settings_scroll_velocity = dy; // for momentum after release
            m_last_mouse_y = event.MouseInput.Y;
        }
        else if (event.MouseInput.Event == EMIE_MOUSE_MOVED && m_sliding_slider) 
        {
            for (auto cat : categories) {
                for (auto mod : cat->mods) {
                    if (mod->m_name == g_settings->get("eclipse.current_module")) {
                        for (auto setting : mod->m_mod_settings) {
                            if (setting->m_name == m_settings_slider_names[m_sliding_slider_index]) {
                                g_settings->setFloat(
                                    setting->m_setting_id,
                                    static_cast<float>(calculateSliderValueFromPosition(
                                            m_settings_slider_boxes[m_sliding_slider_index],
                                            core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y),
                                            setting->m_min,
                                            setting->m_max,
                                            setting->m_steps
                                        )
                                    )
                                );
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return Parent ? Parent->OnEvent(event) : false; 
}

void draw2DThickLine(
    video::IVideoDriver* driver,
    core::vector2d<s32> start,
    core::vector2d<s32> end,
    video::SColor color,
    f32 width,
    const core::rect<s32>* clip = nullptr)
{
    if (!driver || width <= 0.0f) return;

    core::array<video::S3DVertex> verts;
    core::array<u16> indices;

    // Helper lambda for clipping vertices
    auto clampToClip = [&](f32 &x, f32 &y) {
        if (clip && clip->getArea() != 0)
        {
            x = core::clamp(x, (f32)clip->UpperLeftCorner.X, (f32)clip->LowerRightCorner.X);
            y = core::clamp(y, (f32)clip->UpperLeftCorner.Y, (f32)clip->LowerRightCorner.Y);
        }
    };

    // Compute direction vector
    f32 dx = (f32)(end.X - start.X);
    f32 dy = (f32)(end.Y - start.Y);

    f32 length = sqrtf(dx*dx + dy*dy);
    if (length == 0.0f) return;

    // Normalize direction
    dx /= length;
    dy /= length;

    // Perpendicular vector (for width)
    f32 px = -dy * width * 0.5f;
    f32 py = dx * width * 0.5f;

    // Compute four corners of the rectangle
    f32 x0 = (f32)start.X + px;
    f32 y0 = (f32)start.Y + py;
    f32 x1 = (f32)start.X - px;
    f32 y1 = (f32)start.Y - py;
    f32 x2 = (f32)end.X - px;
    f32 y2 = (f32)end.Y - py;
    f32 x3 = (f32)end.X + px;
    f32 y3 = (f32)end.Y + py;

    // Clamp to clip if provided
    clampToClip(x0, y0);
    clampToClip(x1, y1);
    clampToClip(x2, y2);
    clampToClip(x3, y3);

    // Add vertices
    u16 base = verts.size();
    verts.push_back(video::S3DVertex(x0, y0, 0,0,0,-1, color, 0, 0));
    verts.push_back(video::S3DVertex(x1, y1, 0,0,0,-1, color, 0, 0));
    verts.push_back(video::S3DVertex(x2, y2, 0,0,0,-1, color, 0, 0));
    verts.push_back(video::S3DVertex(x3, y3, 0,0,0,-1, color, 0, 0));

    // Add two triangles for the rectangle
    indices.push_back(base);
    indices.push_back(base+1);
    indices.push_back(base+2);

    indices.push_back(base);
    indices.push_back(base+2);
    indices.push_back(base+3);

    // Draw
    video::SMaterial m;
    m.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
    driver->setMaterial(m);
    driver->draw2DVertexPrimitiveList(verts.const_pointer(), verts.size(),
                                      indices.const_pointer(), indices.size()/3,
                                      video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);
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
        float step = (anim_speed * m_animation_speeds[i]) * dtime;

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
    m_animation_speeds.emplace_back(1.0);
}

void EclipseMenu::setAnimationSpeed(std::string id, double speed)
{
    for (size_t i = 0; i < m_animation_ids.size(); ++i) {
        if (m_animation_ids[i] == id) {
            m_animation_speeds[i] = speed;
            return;
        }
    }
    m_animation_ids.emplace_back(id);
    m_animations.emplace_back(0.0);
    m_animation_targets.emplace_back(0.0);
    m_animation_speeds.emplace_back(speed);
}

void EclipseMenu::setAnimationInstant(std::string id, double value)
{
    for (size_t i = 0; i < m_animation_ids.size(); ++i) {
        if (m_animation_ids[i] == id) {
            m_animations[i] = value;
            m_animation_targets[i] = value;
            return;
        }
    }
    m_animation_ids.emplace_back(id);
    m_animations.emplace_back(value);
    m_animation_targets.emplace_back(value);
    m_animation_speeds.emplace_back(1.0);
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
    m_mods_toggle_boxes.clear();

    const s32 mod_padding = applyScalingFactorS32(15);
    const s32 num_mods_per_row = 4;
    const s32 total_padding = num_mods_per_row * 2 * mod_padding;
    const s32 mod_width = (clip.getWidth() - total_padding) / num_mods_per_row;
    const s32 mod_height = (mod_width * 1);
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

        // Text rect (top)
        core::rect<s32> text_rect(
            draw_x,
            draw_y,
            draw_x + mod_width,
            draw_y + (mod_height * 0.25f)
        );

        // Icon area (middle)
        core::rect<s32> mod_icon_rect(
            draw_x,
            draw_y + (mod_height * 0.25f),
            draw_x + mod_width,
            draw_y + (mod_height * 0.75f)
        );

        // Toggle rect (bottom)
        
        core::rect<s32> toggle_rect(
            draw_x + (mod_width * 0.35f),
            draw_y + (mod_height * 0.8f),
            draw_x + (mod_width * 0.65f),
            draw_y + (mod_height * 0.95f)
        );
        m_mods_toggle_boxes.push_back(toggle_rect);
        setAnimationTarget(mod->m_name + "_toggle_hover", toggle_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
        f32 adjustment = static_cast<f32>((0.02f * easeInOutCubic(getAnimation(mod->m_name + "_toggle_hover"))) - (0.01f * easeInOutCubic(getAnimation(mod->m_name + "_toggle_pressed"))));
        toggle_rect = core::rect<s32>(
            draw_x + (mod_width * (0.35f - adjustment)),
            draw_y + (mod_height * (0.8f - adjustment)),
            draw_x + (mod_width * (0.65f + adjustment)),
            draw_y + (mod_height * (0.95f + adjustment))
        );

        m_mods_boxes.push_back(mod_rect);
        m_mods_names.push_back(mod->m_name);

        // Background
        drawRoundedRectShadow(driver, mod_rect, theme.secondary, corner_radius, corner_radius, corner_radius, corner_radius, 4, 6, 0.1f, &clip);

        // Hover overlay
        setAnimationTarget(mod->m_name + "_hover", mod_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
        u32 highlight_alpha = (u32)(easeInOutCubic(getAnimation(mod->m_name + "_hover")) * 64);
        video::SColor highlight = theme.secondary_muted;
        highlight.setAlpha(highlight_alpha);
        driver->draw2DRoundedRectangle(mod_rect, highlight, corner_radius, &clip);

        // Draw label text
        font->draw(
            wname.c_str(),
            text_rect,
            theme.text,
            true, true, &clip
        );

        // Draw toggle switch
        
        setAnimationTarget(mod->m_name + "_enabled", mod->is_enabled() ? 1.0 : 0.0);
        video::SColor toggle_knob = lerpColor(theme.text_muted, theme.enabled, easeInOutCubic(getAnimation(mod->m_name + "_enabled")));
        
        // Draw toggle background
        driver->draw2DRoundedRectangle(
            toggle_rect,
            theme.text,
            toggle_rect.getHeight() / 6,
            &clip
        );

        // create knob rect
        s32 knob_diameter = toggle_rect.getHeight() - 4;
        // lerp based on animation target s32 knob_x = mod->is_enabled() ? toggle_rect.LowerRightCorner.X - knob_diameter - 2 : toggle_rect.UpperLeftCorner.X + 2;
        s32 knob_x = static_cast<s32>(
            toggle_rect.UpperLeftCorner.X + 2 +
            (toggle_rect.getWidth() - knob_diameter - 4) * easeInOutCubic(getAnimation(mod->m_name + "_enabled"))
        );

        core::rect<s32> knob_rect(
            knob_x,
            toggle_rect.UpperLeftCorner.Y + 2,
            knob_x + knob_diameter,
            toggle_rect.LowerRightCorner.Y - 2
        );
        driver->draw2DRoundedRectangle(
            knob_rect,
            toggle_knob,
            knob_diameter / 6,
            &clip
        );

        // Draw checkmark
        f32 checkmark_progress = easeInOutCubic(getAnimation(mod->m_name + "_enabled"));

        f32 checkmark_small_progress = std::clamp(checkmark_progress / 0.3f, 0.0f, 1.0f);

        f32 checkmark_large_progress = std::clamp((checkmark_progress - 0.4f) / 0.6f, 0.0f, 1.0f);

        f32 checkmark_width = knob_diameter / 10;

        s32 center_x = (knob_rect.UpperLeftCorner.X + knob_rect.LowerRightCorner.X) / 2;
        s32 center_y = (knob_rect.UpperLeftCorner.Y + knob_rect.LowerRightCorner.Y) / 2;
        s32 checkmark_size = knob_diameter / 1.25;
        s32 offset = checkmark_size / 12;

        // Small line (bottom-left stroke)
        if (checkmark_small_progress > 0.0f) {
            s32 x1 = (center_x - (checkmark_size / 4)) - offset;
            s32 y1 = center_y;
            s32 x2 = x1 + static_cast<s32>((checkmark_size / 4) * checkmark_small_progress);
            s32 y2 = y1 + static_cast<s32>((checkmark_size / 4) * checkmark_small_progress);
            draw2DThickLine(driver, {x1, y1}, {x2, y2}, theme.text, checkmark_width, &clip);
        }

        // Large line (upper-right stroke)
        if (checkmark_large_progress > 0.0f) {
            s32 corner_x = (center_x - 1) - offset;
            s32 corner_y = center_y + (checkmark_size / 4) - 1;

            s32 x2 = corner_x + static_cast<s32>((checkmark_size / 2) * checkmark_large_progress);
            s32 y2 = corner_y - static_cast<s32>((checkmark_size / 2) * checkmark_large_progress);
            draw2DThickLine(driver, {corner_x, corner_y}, {x2, y2}, theme.text, checkmark_width, &clip);
        }


        // Layout positioning
        x_index++;
        if (x_index >= num_mods_per_row) {
            x_index = 0;
            y_index++;
        }
        draw_x = clip.UpperLeftCorner.X + mod_padding + (x_index * (mod_width + (mod_padding * 2)));
        draw_y = clip.UpperLeftCorner.Y + mod_padding + (y_index * (mod_height + mod_padding)) + m_mods_scroll;
    }
}

s32 get_setting_render_height(ModSetting& setting)
{

    // valid types are: "bool", "slider_int", "slider_float", "text", "dropdown"

    const s32 base_height = 50;
    if (setting.m_type == "bool")
        return base_height;

    else if (setting.m_type == "slider_int" || setting.m_type == "slider_float")
        return base_height * 2;

    else if (setting.m_type == "text")
        return base_height * 6;

    else if (setting.m_type == "dropdown")
        return base_height * 2;

    return base_height;
}

void draw_text_shrink_to_fit(
    video::IVideoDriver* driver,
    const s32 max_font_size,
    const std::wstring& text,
    core::rect<s32> rect,
    video::SColor color,
    const core::rect<s32>* clip = nullptr
) {
    gui::IGUIFont* font = g_fontengine->getFont(max_font_size, FM_Standard);
    if (!font) return;

    core::dimension2du text_size = font->getDimension(text.c_str());
    s32 font_size = max_font_size;
    while (((static_cast<s32>(text_size.Width) > rect.getWidth()) || (static_cast<s32>(text_size.Height) > rect.getHeight())) && font_size > 4) {
        font_size -= 2;
        font = g_fontengine->getFont(font_size, FM_Standard);
        if (!font) break;
        text_size = font->getDimension(text.c_str());
    }
    if (font) {
        font->draw(
            text.c_str(),
            rect,
            color,
            true, true,
            clip
        );
    }
}

void EclipseMenu::draw_dropdown_options(video::IVideoDriver *driver, gui::IGUIFont *font, ColorTheme theme, std::vector<ModCategory *> categories)
{
    std::string current_module_name = g_settings->get("eclipse.current_module");

    m_dropdown_option_boxes.clear();
    m_dropdown_option_names.clear();

    for (auto cat : categories) {
        for (auto mod : cat->mods) {
            if (mod->m_name == current_module_name) {
                for (auto setting : mod->m_mod_settings) {
                    if (m_selecting_dropdown_index + 1 <= m_settings_dropdown_names.size() && setting->m_name == m_settings_dropdown_names[m_selecting_dropdown_index]) {

                        setAnimationSpeed(setting->m_name + "_dropdown_open", 2.0);
                        double anim_progress = getAnimation(setting->m_name + "_dropdown_open");

                        if (anim_progress >= 0.05f) {
                            for(size_t i = 0; i < setting->m_options.size(); ++i) 
                            {
                                bool is_last_option = (i == setting->m_options.size() - 1);

                                s32 option_height = applyScalingFactorS32(40) * anim_progress;
                                core::rect<s32> option_rect(
                                    m_settings_dropdown_boxes[m_selecting_dropdown_index].UpperLeftCorner.X,
                                    m_settings_dropdown_boxes[m_selecting_dropdown_index].LowerRightCorner.Y + (i * option_height),
                                    m_settings_dropdown_boxes[m_selecting_dropdown_index].LowerRightCorner.X,
                                    m_settings_dropdown_boxes[m_selecting_dropdown_index].LowerRightCorner.Y + ((i + 1) * option_height)
                                );

                                m_dropdown_option_boxes.push_back(option_rect);
                                m_dropdown_option_names.push_back(*setting->m_options[i]);

                                if (is_last_option) {// Draw background
                                    drawRoundedRectShadow(
                                        driver,
                                        option_rect,
                                        theme.secondary,
                                        0,
                                        0,
                                        applyScalingFactorS32(5),
                                        applyScalingFactorS32(5),
                                        2, 4, 0.1f
                                    );
                                    core::rect<s32> outline_rect(
                                        m_settings_dropdown_boxes[m_selecting_dropdown_index].UpperLeftCorner.X,
                                        m_settings_dropdown_boxes[m_selecting_dropdown_index].LowerRightCorner.Y,
                                        option_rect.LowerRightCorner.X,
                                        option_rect.LowerRightCorner.Y
                                    );
                                    double color_anim = easeInOutCubic(getAnimation(setting->m_name + "_dropdown_hover"));
                                    video::SColor outline_color = lerpColor(theme.text_muted, theme.text, static_cast<f32>(color_anim));
                                    driver->draw2DRoundedRectangleOutline(
                                        outline_rect,
                                        outline_color,
                                        applyScalingFactorS32(2),
                                        0,
                                        0,
                                        applyScalingFactorS32(4),
                                        applyScalingFactorS32(4)
                                    );
                                } else {
                                    driver->draw2DRectangle(
                                        theme.secondary,
                                        option_rect
                                    );
                                }
                                

                                // Hover overlay
                                setAnimationSpeed(setting->m_name + "_option_" + std::to_string(i) + "_hover", 2.0);
                                setAnimationTarget(setting->m_name + "_option_" + std::to_string(i) + "_hover", option_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
                                u32 highlight_alpha = (u32)(easeInOutCubic(getAnimation(setting->m_name + "_option_" + std::to_string(i) + "_hover")) * 64);
                                video::SColor highlight = theme.secondary_muted;
                                highlight.setAlpha(highlight_alpha);
                                driver->draw2DRoundedRectangle(option_rect, highlight, applyScalingFactorS32(5));

                                // Draw option text
                                const std::string opt_str = *setting->m_options[i];
                                std::wstring woption = utf8_to_wide(opt_str);
                                draw_text_shrink_to_fit(
                                    driver,
                                    applyScalingFactorS32(20),
                                    woption,
                                    option_rect,
                                    theme.text
                                );
                            }
                        }
                    }
                }
            }
        }
    }
}

void EclipseMenu::draw_module_settings(video::IVideoDriver *driver, core::rect<s32> clip, core::rect<s32> topbar_clip, gui::IGUIFont *font, std::vector<ModCategory *> categories, ColorTheme theme, float dtime)
{
    m_settings_toggle_boxes.clear();
    m_settings_toggle_names.clear();
    m_settings_slider_boxes.clear();
    m_settings_slider_names.clear();
    m_settings_slider_knob_boxes.clear();
    m_settings_dropdown_boxes.clear();
    m_settings_dropdown_names.clear();

    std::string current_category_name = g_settings->get("eclipse.current_category");
    ModCategory* current_category = nullptr;

    std::string current_module_name = g_settings->get("eclipse.current_module");
    Mod* current_module = nullptr;

    std::string setting_path = "No module selected";

    // Find category by name
    for (auto* cat : categories) {
        if (cat->m_name == current_category_name) {
            current_category = cat;
            break;
        }
    }

    if (current_category) {
        // Find module by name
        for (auto* cat : categories) {
            for (auto* mod : cat->mods) {
                if (mod->m_name == current_module_name) {
                    current_module = mod;
                    setting_path = cat->m_name + " ► " + mod->m_name;
                    break;
                }
            }
        }
    }

    // Draw top bar with module path
    std::wstring wsetting_path = utf8_to_wide(setting_path);


    // get font dimension
    core::dimension2du text_size = font->getDimension(wsetting_path.c_str());

    // draw box behind text centered vertically and horizontally in topbar
    current_path_rect = core::rect<s32> (
        topbar_clip.getCenter().X - (text_size.Width / 2) - applyScalingFactorS32(10),
        topbar_clip.getCenter().Y - (text_size.Height / 2) - applyScalingFactorS32(5),
        topbar_clip.getCenter().X + (text_size.Width / 2) + applyScalingFactorS32(10),
        topbar_clip.getCenter().Y + (text_size.Height / 2) + applyScalingFactorS32(5)
    );
    
    core::rect<s32> settings_area(
        clip.UpperLeftCorner.X + applyScalingFactorS32(15),
        topbar_clip.LowerRightCorner.Y + applyScalingFactorS32(15),
        clip.LowerRightCorner.X - applyScalingFactorS32(15),
        clip.LowerRightCorner.Y - applyScalingFactorS32(15)
    );

    // Draw tab background
    drawRoundedRectShadow(
        driver,
        current_path_rect,
        theme.secondary,
        applyScalingFactorS32(10),
        applyScalingFactorS32(10),
        applyScalingFactorS32(10),
        applyScalingFactorS32(10),
        2, 4, 0.1f,
        &topbar_clip
    );

    // Draw tab hover overlay
    setAnimationTarget("module_settings_rect_hover", current_path_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
    u32 highlight_alpha = (u32)(easeInOutCubic(getAnimation("module_settings_rect_hover")) * 127);

    video::SColor highlight = theme.secondary_muted;
    highlight.setAlpha(highlight_alpha);
    driver->draw2DRoundedRectangle(current_path_rect, highlight, applyScalingFactorS32(10), &topbar_clip);

    font->draw(
        wsetting_path.c_str(),
        topbar_clip,
        theme.text,
        true, true, &topbar_clip
    );

    m_module_settings_rect = settings_area;

    // Calculate total height of settings content
    const s32 setting_spacing = applyScalingFactorS32(15);
    s32 total_height = setting_spacing;
    if (current_module) {
        for (auto& setting : current_module->m_mod_settings) {
            total_height += applyScalingFactorS32(get_setting_render_height(*setting)) + setting_spacing;
        }
    } else {
        const int lineHeight = font->getDimension(L"A").Height;  // height of one line
        total_height = lineHeight * 3;
    }

    float clip_h = static_cast<float>(clip.getHeight());
    float content_h = static_cast<float>(total_height);

    float top_bound = 0.0f;
    float bottom_bound = std::min(0.0f, clip_h - content_h);

    if (!m_dragging_settings)
    {
        float overscroll_top = std::max(0.0f, m_settings_scroll - top_bound);
        float overscroll_bottom = std::min(0.0f, m_settings_scroll - bottom_bound);

        float spring_force = -overscroll_top - overscroll_bottom;

        float spring_strength = 0.05f;
        m_settings_scroll_velocity += spring_force * spring_strength;

        m_settings_scroll_velocity *= 0.92f;
        if (std::abs(m_settings_scroll_velocity) < 0.1f)
            m_settings_scroll_velocity = 0.0f;
        m_settings_scroll += m_settings_scroll_velocity * dtime * 10.0f;
    }


    if (!current_module) {
        const std::string message = 
            "No module selected.\n"
            "Click a module to\n"
            "view its settings.";

        // Convert to wide string
        std::wstring wmessage = utf8_to_wide(message);

        // Split into lines
        std::vector<std::wstring> lines;
        {
            std::wstringstream ss(wmessage);
            std::wstring line;
            while (std::getline(ss, line, L'\n')) {
                lines.push_back(line);
            }
        }

        // Vertical layout parameters
        const int lineHeight = font->getDimension(L"A").Height;  // height of one line
        const int totalHeight = lineHeight * lines.size();
        const int startY = (clip.UpperLeftCorner.Y + (clip.getHeight() - totalHeight) / 2) + static_cast<int>(m_settings_scroll);

        // Draw each line centered horizontally
        for (size_t i = 0; i < lines.size(); i++) {
            int y = startY + i * lineHeight;

            core::rect<s32> lineRect(
                clip.UpperLeftCorner.X,
                y,
                clip.LowerRightCorner.X,
                y + lineHeight
            );

            font->draw(
                lines[i].c_str(),
                lineRect,
                theme.text_muted,
                true,
                true,
                &settings_area
            );
        }

        return;
    } else {
        int current_y = settings_area.UpperLeftCorner.Y + m_settings_scroll;
        for (auto& setting : current_module->m_mod_settings) {
            s32 setting_height = applyScalingFactorS32(get_setting_render_height(*setting));
            core::rect<s32> setting_rect(
                settings_area.UpperLeftCorner.X,
                current_y,
                settings_area.LowerRightCorner.X,
                current_y + setting_height
            );

            // Draw setting background
            drawRoundedRectShadow(
                driver,
                setting_rect,
                theme.primary,
                applyScalingFactorS32(10),
                applyScalingFactorS32(10),
                applyScalingFactorS32(10),
                applyScalingFactorS32(10),
                4, 6, 0.1f,
                &settings_area
            );

            // Draw hover overlay
            setAnimationTarget(setting->m_name + "_hover", setting_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
            u32 highlight_alpha = (u32)(easeInOutCubic(getAnimation(setting->m_name + "_hover")) * 64);
            video::SColor highlight = theme.secondary_muted;
            highlight.setAlpha(highlight_alpha);
            driver->draw2DRoundedRectangle(setting_rect, highlight, applyScalingFactorS32(10), &settings_area);

            // Draw the setting itself
            if(setting->m_type == "bool") {
                core::rect<s32> text_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y,
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(78),
                    setting_rect.LowerRightCorner.Y
                );
                std::wstring wsetting_name = utf8_to_wide(setting->m_name);
                draw_text_shrink_to_fit(
                    driver,
                    applyScalingFactorS32(20),
                    wsetting_name,
                    text_rect,
                    theme.text,
                    &settings_area
                );

                // Draw toggle switch
                core::rect<s32> toggle_rect(
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(68),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(11),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(11)
                );

                m_settings_toggle_boxes.push_back(toggle_rect);
                m_settings_toggle_names.push_back(setting->m_name);
                setAnimationTarget(setting->m_name + "_toggle_hover", toggle_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
                f32 adjustment = static_cast<f32>((easeInOutCubic(getAnimation(setting->m_name + "_toggle_hover"))) - (0.5f * easeInOutCubic(getAnimation(setting->m_name + "_toggle_pressed"))));
                toggle_rect = core::rect<s32>(
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(68) - static_cast<s32>(applyScalingFactorDouble(4.0 * adjustment)),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(11) - static_cast<s32>(applyScalingFactorDouble(4.0 * adjustment)),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10) + static_cast<s32>(applyScalingFactorDouble(4.0 * adjustment)),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(11) + static_cast<s32>(applyScalingFactorDouble(4.0 * adjustment))
                );

                setAnimationTarget(setting->m_name + "_enabled", g_settings->getBool(setting->m_setting_id) ? 1.0 : 0.0);
                video::SColor toggle_knob = lerpColor(theme.text_muted, theme.enabled, easeInOutCubic(getAnimation(setting->m_name + "_enabled")));
                // Draw toggle background
                driver->draw2DRoundedRectangle(
                    toggle_rect,
                    theme.text,
                    toggle_rect.getHeight() / 6,
                    &settings_area
                );
                // create knob rect
                s32 knob_diameter = toggle_rect.getHeight() - 4;
                s32 knob_x = static_cast<s32>(
                    toggle_rect.UpperLeftCorner.X + 2 +
                    (toggle_rect.getWidth() - knob_diameter - 4) * easeInOutCubic(getAnimation(setting->m_name + "_enabled"))
                );
                core::rect<s32> knob_rect(
                    knob_x,
                    toggle_rect.UpperLeftCorner.Y + 2,
                    knob_x + knob_diameter,
                    toggle_rect.LowerRightCorner.Y - 2
                );
                driver->draw2DRoundedRectangle(
                    knob_rect,
                    toggle_knob,
                    knob_diameter / 6,
                    &settings_area
                );

                // Draw checkmark
                f32 checkmark_progress = easeInOutCubic(getAnimation(setting->m_name + "_enabled"));
                f32 checkmark_small_progress = std::clamp(checkmark_progress / 0.3f, 0.0f, 1.0f);
                f32 checkmark_large_progress = std::clamp((checkmark_progress - 0.4f) / 0.6f, 0.0f, 1.0f);
                f32 checkmark_width = knob_diameter / 10;
                s32 center_x = (knob_rect.UpperLeftCorner.X + knob_rect.LowerRightCorner.X) / 2;
                s32 center_y = (knob_rect.UpperLeftCorner.Y + knob_rect.LowerRightCorner.Y) / 2;
                s32 checkmark_size = knob_diameter / 1.25;
                s32 offset = checkmark_size / 12;
                // Small line (bottom-left stroke)
                if (checkmark_small_progress > 0.0f) {
                    s32 x1 = (center_x - (checkmark_size / 4)) - offset;
                    s32 y1 = center_y;
                    s32 x2 = x1 + static_cast<s32>((checkmark_size / 4) * checkmark_small_progress);
                    s32 y2 = y1 + static_cast<s32>((checkmark_size / 4) * checkmark_small_progress);
                    draw2DThickLine(driver, {x1, y1}, {x2, y2}, theme.text, checkmark_width, &settings_area);
                }
                // Large line (upper-right stroke)
                if (checkmark_large_progress > 0.0f) {
                    s32 corner_x = (center_x - 1) - offset;
                    s32 corner_y = center_y + (checkmark_size / 4) - 1;
                    s32 x2 = corner_x + static_cast<s32>((checkmark_size / 2) * checkmark_large_progress);
                    s32 y2 = corner_y - static_cast<s32>((checkmark_size / 2) * checkmark_large_progress);
                    draw2DThickLine(driver, {corner_x, corner_y}, {x2, y2}, theme.text, checkmark_width, &settings_area);
                }

            } else if (setting->m_type == "slider_int" || setting->m_type == "slider_float") {
                // Draw text label at top
                core::rect<s32> text_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y + (setting_rect.getHeight() / 2) - applyScalingFactorS32(10)
                );
                std::wstring wsetting_name = utf8_to_wide(setting->m_name);
                draw_text_shrink_to_fit(
                    driver,
                    applyScalingFactorS32(20),
                    wsetting_name,
                    text_rect,
                    theme.text,
                    &settings_area
                );

                // Draw current value in box at middle right
                std::wstring wsetting_value;
                if (setting->m_type == "slider_int") {
                    int value = g_settings->getS32(setting->m_setting_id);
                    wsetting_value = std::to_wstring(value);
                } else { // slider_float
                    float value = g_settings->getFloat(setting->m_setting_id);
                    wsetting_value = std::to_wstring(value);
                    // Trim all trailing 0s after decimal point
                    size_t decimal_pos = wsetting_value.find(L'.');
                    if (decimal_pos != std::wstring::npos) {
                        size_t end_pos = wsetting_value.size() - 1;
                        while (end_pos > decimal_pos + 1 && wsetting_value[end_pos] == L'0') {
                            end_pos--;
                        }
                        if (end_pos == decimal_pos) {
                            end_pos++; // keep one decimal place
                        }
                        wsetting_value = wsetting_value.substr(0, end_pos + 1);
                    }
                }
                core::rect<s32> value_rect = core::rect<s32>(
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(70),
                    setting_rect.UpperLeftCorner.Y + (setting_rect.getHeight() / 2) + applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(10)
                );

                drawRoundedRectShadow(
                    driver,
                    value_rect,
                    theme.secondary,
                    applyScalingFactorS32(6),
                    applyScalingFactorS32(6),
                    applyScalingFactorS32(6),
                    applyScalingFactorS32(6),
                    2, 4, 0.1f,
                    &settings_area
                );

                draw_text_shrink_to_fit(
                    driver,
                    applyScalingFactorS32(20),
                    wsetting_value,
                    value_rect,
                    theme.text,
                    &settings_area
                );

                // Draw slider bar at bottom
                core::rect<s32> slider_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(20),
                    setting_rect.UpperLeftCorner.Y + (setting_rect.getHeight() / 2) + applyScalingFactorS32(20),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(80),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(20)
                );
                f32 slider_value = 0.0f;
                slider_value = (static_cast<f32>(g_settings->getFloat(setting->m_setting_id)) - setting->m_min) / (setting->m_max - setting->m_min);
                core::rect<s32> filled_rect = core::rect<s32>(
                    slider_rect.UpperLeftCorner.X,
                    slider_rect.UpperLeftCorner.Y,
                    slider_rect.UpperLeftCorner.X + static_cast<s32>(slider_rect.getWidth() * slider_value),
                    slider_rect.LowerRightCorner.Y
                );
                s32 knob_size = slider_rect.getHeight();
                s32 knob_x = filled_rect.LowerRightCorner.X - knob_size / 2;
                core::rect<s32> knob_rect(
                    knob_x,
                    slider_rect.UpperLeftCorner.Y - applyScalingFactorS32(10),
                    knob_x + knob_size,
                    slider_rect.LowerRightCorner.Y + applyScalingFactorS32(10)
                );

                m_settings_slider_boxes.push_back(slider_rect);
                m_settings_slider_knob_boxes.push_back(knob_rect);
                m_settings_slider_names.push_back(setting->m_name);

                bool is_sliding = false;
                if (m_sliding_slider) {
                    if (m_sliding_slider_index >= 0 && m_sliding_slider_index < m_settings_slider_names.size()) {
                        if (m_settings_slider_names[m_sliding_slider_index] == setting->m_name) {
                            is_sliding = true;
                        }
                    }
                }

                setAnimationTarget(setting->m_name + "_slider_hover", slider_rect.isPointInside(m_current_mouse_pos) || knob_rect.isPointInside(m_current_mouse_pos) || is_sliding ? 1.0 : 0.0);
                setAnimationTarget(setting->m_name + "_slider_knob_pressed", is_sliding ? 1.0 : 0.0);
                f32 adjustment = static_cast<f32>((easeInOutCubic(getAnimation(setting->m_name + "_slider_hover"))) - (0.5f * easeInOutCubic(getAnimation(setting->m_name + "_slider_knob_pressed"))));
                slider_rect = core::rect<s32>(
                    slider_rect.UpperLeftCorner.X - static_cast<s32>(applyScalingFactorDouble(3.0 * adjustment)),
                    slider_rect.UpperLeftCorner.Y - static_cast<s32>(applyScalingFactorDouble(3.0 * adjustment)),
                    slider_rect.LowerRightCorner.X + static_cast<s32>(applyScalingFactorDouble(3.0 * adjustment)),
                    slider_rect.LowerRightCorner.Y + static_cast<s32>(applyScalingFactorDouble(3.0 * adjustment))
                );
                filled_rect = core::rect<s32>(
                    slider_rect.UpperLeftCorner.X,
                    slider_rect.UpperLeftCorner.Y,
                    slider_rect.UpperLeftCorner.X + static_cast<s32>(slider_rect.getWidth() * slider_value),
                    slider_rect.LowerRightCorner.Y
                );
                // Draw filled background
                driver->draw2DRoundedRectangle(
                    slider_rect,
                    theme.text_muted,
                    applyScalingFactorS32(4),
                    &settings_area
                );

                // Draw remaining background

                driver->draw2DRoundedRectangle(
                    filled_rect,
                    theme.enabled,
                    applyScalingFactorS32(4),
                    &settings_area
                );

                // Draw slider square knob
                setAnimationTarget(setting->m_name + "_slider_knob_hover", knob_rect.isPointInside(m_current_mouse_pos) || is_sliding ? 1.0 : 0.0);
                adjustment = static_cast<f32>((easeInOutCubic(getAnimation(setting->m_name + "_slider_knob_hover"))) - (0.5f * easeInOutCubic(getAnimation(setting->m_name + "_slider_knob_pressed"))));
                knob_rect = core::rect<s32>(
                    knob_rect.UpperLeftCorner.X - static_cast<s32>(applyScalingFactorDouble(3.0 * adjustment)),
                    knob_rect.UpperLeftCorner.Y - static_cast<s32>(applyScalingFactorDouble(4.0 * adjustment)),
                    knob_rect.LowerRightCorner.X + static_cast<s32>(applyScalingFactorDouble(3.0 * adjustment)),
                    knob_rect.LowerRightCorner.Y + static_cast<s32>(applyScalingFactorDouble(4.0 * adjustment))
                );
                driver->draw2DRoundedRectangle(
                    knob_rect,
                    theme.text,
                    applyScalingFactorS32(4),
                    &settings_area
                );
                
            } else if (setting->m_type == "text") {
            //    TODO
            } else if (setting->m_type == "dropdown") {
                m_settings_dropdown_names.push_back(setting->m_name);
                size_t current_index = m_settings_dropdown_names.size() - 1;
                setAnimationTarget(setting->m_name + "_dropdown_open", m_selecting_dropdown && m_selecting_dropdown_index == current_index ? 1.0 : 0.0);
                // Draw text label at top
                core::rect<s32> text_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y + (setting_rect.getHeight() / 2) - applyScalingFactorS32(10)
                );
                std::wstring wsetting_name = utf8_to_wide(setting->m_name);
                draw_text_shrink_to_fit(
                    driver,
                    applyScalingFactorS32(20),
                    wsetting_name,
                    text_rect,
                    theme.text,
                    &settings_area
                );

                // Draw dropdown box at bottom
                core::rect<s32> dropdown_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(20),
                    setting_rect.UpperLeftCorner.Y + (setting_rect.getHeight() / 2),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(20),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(10)
                );

                m_settings_dropdown_boxes.push_back(dropdown_rect);

                double anim_progress = easeInOutCubic(getAnimation(setting->m_name + "_dropdown_open"));
                // Draw dropdown background
                drawRoundedRectShadow(
                    driver,
                    dropdown_rect,
                    theme.secondary,
                    applyScalingFactorS32(4),
                    applyScalingFactorS32(4),
                    applyScalingFactorS32(4) * (1 - anim_progress),
                    applyScalingFactorS32(4) * (1 - anim_progress),
                    2, 4, 0.1f,
                    &settings_area
                );

                // Draw current selected option
                std::string current_option = g_settings->get(setting->m_setting_id);
                std::wstring wcurrent_option = utf8_to_wide(current_option);
                core::rect<s32> option_text_rect = core::rect<s32>(
                    dropdown_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    dropdown_rect.UpperLeftCorner.Y,
                    dropdown_rect.LowerRightCorner.X - (dropdown_rect.getHeight() + applyScalingFactorS32(10)),
                    dropdown_rect.LowerRightCorner.Y
                );
                draw_text_shrink_to_fit(
                    driver,
                    applyScalingFactorS32(20),
                    wcurrent_option,
                    option_text_rect,
                    theme.text,
                    &settings_area
                );

                // Draw dropdown arrow
                core::rect<s32> arrow_rect = core::rect<s32>(
                    dropdown_rect.LowerRightCorner.X - dropdown_rect.getHeight(),
                    dropdown_rect.UpperLeftCorner.Y,
                    dropdown_rect.LowerRightCorner.X,
                    dropdown_rect.LowerRightCorner.Y
                );

                s32 arrow_center_x = arrow_rect.getCenter().X;
                s32 arrow_center_y = arrow_rect.getCenter().Y;
                s32 arrow_size = (dropdown_rect.getHeight() / 3) & ~1; // Ensure this is always even
                double anim_offset = (easeInOutCubic(getAnimation(setting->m_name + "_dropdown_open")) * 2.0f) - 1.0f;
                s32 arrow_offset = ((arrow_size / 2) * anim_offset);
                core::vector2d<s32> arrow_center = core::vector2d<s32>(arrow_center_x, (arrow_center_y + arrow_offset));
                core::vector2d<s32> arrow_left = core::vector2d<s32>((arrow_center_x - arrow_size), (arrow_center_y - arrow_offset));
                core::vector2d<s32> arrow_right = core::vector2d<s32>((arrow_center_x + arrow_size), (arrow_center_y - arrow_offset));
                bool is_open = m_selecting_dropdown && m_selecting_dropdown_index == current_index;
                setAnimationTarget(setting->m_name + "_dropdown_hover", dropdown_rect.isPointInside(m_current_mouse_pos) || is_open ? 1.0 : 0.0);
                double color_anim = easeInOutCubic(getAnimation(setting->m_name + "_dropdown_hover"));
                video::SColor arrow_color = lerpColor(theme.text_muted, theme.text, static_cast<f32>(color_anim));
                draw2DThickLine(
                    driver,
                    arrow_left,
                    arrow_center,
                    arrow_color,
                    applyScalingFactorDouble(2.0),
                    &settings_area
                );
                draw2DThickLine(
                    driver,
                    arrow_center,
                    arrow_right,
                    arrow_color,
                    applyScalingFactorDouble(2.0),
                    &settings_area
                );

                // Draw dropdown outline
                driver->draw2DRoundedRectangleOutline(
                    dropdown_rect,
                    arrow_color,
                    applyScalingFactorS32(2),
                    applyScalingFactorS32(4),
                    applyScalingFactorS32(4),
                    applyScalingFactorS32(4) * (1 - anim_progress),
                    applyScalingFactorS32(4) * (1 - anim_progress),
                    &settings_area
                );
            }

            current_y += setting_height + setting_spacing;
        }
    }
}

void EclipseMenu::draw() 
{
    GET_CATEGORIES_OR_RETURN(categories);

    g_settings->setDefault("eclipse.current_category", categories[0]->m_name);
    std::string current_category_name = g_settings->get("eclipse.current_category");
    ModCategory* current_category = nullptr;

    g_settings->setDefault("eclipse.current_module", "");

    updateScaling();
    updateTheming();

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

        const core::dimension2di module_settings_size((menusize.Width * 0.25) - (section_gap / 2), menusize.Height - 1);

        const core::rect<s32> module_settings_rect(
            menurect.UpperLeftCorner.X,                         //x1 ( Upper left )
            menurect.UpperLeftCorner.Y,                         //y1
            menurect.UpperLeftCorner.X + module_settings_size.Width,    //x2 ( Lower right )
            menurect.UpperLeftCorner.Y + module_settings_size.Height    //y2
        );

        const core::rect<s32> module_settings_topbar_rect(
            module_settings_rect.UpperLeftCorner.X,                         //x1 ( Upper left )
            module_settings_rect.UpperLeftCorner.Y,                         //y1
            module_settings_rect.LowerRightCorner.X,                        //x2 ( Lower right )
            module_settings_rect.UpperLeftCorner.Y + top_bar_height         //y2
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
        driver->draw2DRoundedRectangle(module_settings_rect, theme.background, corner_radius, 0, 0, corner_radius); // module_settings background
        drawRoundedRectShadow(driver, module_settings_topbar_rect, theme.background_top, corner_radius, 0, 0, corner_radius, 4, 6, 0.1f); // module_settings top bar
        driver->draw2DRoundedRectangle(modsrect, theme.background, 0, corner_radius, corner_radius, 0); // Mods background
        drawRoundedRectShadow(driver, mods_topbar_rect, theme.background_top, 0, corner_radius, corner_radius, 0, 4, 6, 0.1f); // Mods top bar

        draw_categories_bar(driver, m_cat_bar_rect, font, current_category, theme, categories, dtime);
        draw_mods_list(driver, m_mods_list_rect, font, current_category, theme, dtime);
        draw_module_settings(driver, module_settings_rect, module_settings_topbar_rect, font, categories, theme, dtime);
        draw_dropdown_options(driver, font, theme, categories);
    }
} 