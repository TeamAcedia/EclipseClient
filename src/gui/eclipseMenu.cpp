// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#include "eclipseMenu.h"
#include "client/fontengine.h"
#include <cstdlib>

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
    bool is_main_menu,
    ISimpleTextureSource *texture_src
)
    : IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, core::rect<s32>(0, 0, 0, 0)), 
    m_texture_src(texture_src),
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
    bool is_main_menu,
    ISimpleTextureSource *texture_src
)
    : IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, core::rect<s32>(0, 0, 0, 0)), 
    m_texture_src(texture_src),
    m_menumgr(menumgr),
    m_client(client),
    m_is_main_menu(is_main_menu)
{    
    infostream << "[EclipseMenu] Successfully created" << std::endl;
    this->env = env;
}

EclipseMenu::~EclipseMenu()
{
    if (m_color_gradient_texture) {
        m_color_gradient_texture->drop();
        m_color_gradient_texture = nullptr;
    }

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

void EclipseMenu::loadHSV(const std::string& key, float &H, float &S, float &V)
{
    std::string value = g_settings->get(key);

    H = 0.0f; S = 1.0f; V = 1.0f; // default values

    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

    std::replace(value.begin(), value.end(), ',', ' ');

    std::istringstream iss(value);
    float h, s, v;
    if (iss >> h >> s >> v) {
        H = std::clamp(h, 0.0f, 1.0f);
        S = std::clamp(s, 0.0f, 1.0f);
        V = std::clamp(v, 0.0f, 1.0f);
    }
}

void saveHSV(const std::string& key, float H, float S, float V)
{
    H = std::clamp(H, 0.0f, 1.0f);
    S = std::clamp(S, 0.0f, 1.0f);
    V = std::clamp(V, 0.0f, 1.0f);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << H << "," << S << "," << V;

    g_settings->set(key, oss.str());
}

void RGBtoHSV(int r, int g, int b, float &h, float &s, float &v)
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxc = std::max(rf, std::max(gf, bf));
    float minc = std::min(rf, std::min(gf, bf));
    float range = maxc - minc;

    // Value
    v = maxc;

    // Saturation
    if (maxc < 0.00001f) {
        s = 0.0f;
        h = 0.0f;
        return;
    }
    s = (range / maxc);

    // Hue
    if (range < 0.00001f) {
        h = 0.0f;
    } else if (maxc == rf) {
        h = fmodf((gf - bf) / range, 6.0f);
    } else if (maxc == gf) {
        h = ((bf - rf) / range) + 2.0f;
    } else {
        h = ((rf - gf) / range) + 4.0f;
    }

    h /= 6.0f;
    if (h < 0.0f) h += 1.0f;
}

void HSVtoRGB(float h, float s, float v, int &r, int &g, int &b)
{
    float c = v * s;        // chroma
    float x = c * (1 - fabsf(fmodf(h * 6.0f, 2.0f) - 1));
    float m = v - c;

    float rf, gf, bf;
    float h6 = h * 6.0f;

    if (h6 < 1)      { rf = c; gf = x; bf = 0; }
    else if (h6 < 2) { rf = x; gf = c; bf = 0; }
    else if (h6 < 3) { rf = 0; gf = c; bf = x; }
    else if (h6 < 4) { rf = 0; gf = x; bf = c; }
    else if (h6 < 5) { rf = x; gf = 0; bf = c; }
    else             { rf = c; gf = 0; bf = x; }

    r = (int)((rf + m) * 255.0f + 0.5f);
    g = (int)((gf + m) * 255.0f + 0.5f);
    b = (int)((bf + m) * 255.0f + 0.5f);
}

// Convert HSV to SColor
video::SColor EclipseMenu::HSVtoSColor(float h, float s, float v)
{
    h = core::clamp(h, 0.0f, 1.0f);
    s = core::clamp(s, 0.0f, 1.0f);
    v = core::clamp(v, 0.0f, 1.0f);

    int r, g, b;
    HSVtoRGB(h, s, v, r, g, b);
    return video::SColor(255, r, g, b);
}


// Generate color gradient texture for a color picker
video::ITexture* EclipseMenu::generateColorGradientTexture(video::IVideoDriver* driver, float hue, const core::dimension2d<u32>& size)
{
    if (!driver) return nullptr;
    if (size.Width == 0 || size.Height == 0) return nullptr;

    video::IImage* image = driver->createImage(video::ECF_A8R8G8B8, size);

    for (u32 y = 0; y < size.Height; ++y)
    {
        float value = 1.0f - (float)y / (float)(size.Height - 1); // left = 0, right = 1
        for (u32 x = 0; x < size.Width; ++x)
        {
            float saturation = (float)x / (float)(size.Width - 1); // top = 1, bottom = 0
            image->setPixel(x, y, HSVtoSColor(hue, saturation, value));
        }
    }

    video::ITexture* texture = driver->addTexture("colorGradient", image);
    image->drop(); // driver now owns it

    return texture;
}

// Generate color gradient texture for a color picker
video::ITexture* EclipseMenu::generateHueGradientTexture(video::IVideoDriver* driver, const core::dimension2d<u32>& size)
{
    if (!driver) return nullptr;
    if (size.Width == 0 || size.Height == 0) return nullptr;

    video::IImage* image = driver->createImage(video::ECF_A8R8G8B8, size);

    for (u32 y = 0; y < size.Height; ++y)
    {
        for (u32 x = 0; x < size.Width; ++x)
        {
            float hue = 1.0f - (float)x / (float)(size.Width - 1); // left = 0, right = 1
            image->setPixel(x, y, HSVtoSColor(hue, 1, 1));
        }
    }

    video::ITexture* texture = driver->addTexture("hueGradient", image);
    image->drop(); // driver now owns it

    return texture;
}

void getColorAtPos(float H, const core::dimension2d<u32>& size, u32 x, u32 y, float *S, float *V)
{
    x = std::clamp(x, 0u, size.Width - 1);
    y = std::clamp(y, 0u, size.Height - 1);

    *S = static_cast<float>(x) / static_cast<float>(size.Width - 1);
    *V = 1.0f - static_cast<float>(y) / static_cast<float>(size.Height - 1);
}

core::position2d<u32> getPosAtColor(const core::dimension2d<u32>& size, float S, float V)
{
    S = std::clamp(S, 0.0f, 1.0f);
    V = std::clamp(V, 0.0f, 1.0f);

    u32 x = static_cast<u32>(S * (size.Width - 1) + 0.5f);
    u32 y = static_cast<u32>((1.0f - V) * (size.Height - 1) + 0.5f);

    return core::position2d<u32>(x, y);
}

void getHueAtPos(const core::dimension2d<u32>& size, u32 x, float *H)
{
    x = std::clamp(x, 0u, size.Width - 1);

    *H = 1.0f - static_cast<float>(x) / static_cast<float>(size.Width - 1);
}

core::position2d<u32> getPosAtHue(const core::dimension2d<u32>& size, float H)
{
    H = std::clamp(H, 0.0f, 1.0f);

    u32 x = static_cast<u32>((1.0f - H) * (size.Width - 1) + 0.5f);

    return core::position2d<u32>(x, 0);
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
    for (auto& pair : m_settings_textboxes_map) {
        EclipseEditBox* edit_box = pair.second.textbox;
        delete edit_box;
    }
    m_settings_textboxes_map.clear();

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

    // Check edit boxes first
    if (!m_sliding_slider && !m_selecting_dropdown && !m_picking_color) {
        for (auto& pair : m_settings_textboxes_map) {
            EclipseEditBox* edit_box = pair.second.textbox;
            std::string parent_mod_name = pair.second.parent_mod_name;
            std::string parent_category_name = pair.second.parent_category_name;
            core::rect<s32> clipRect = pair.second.clipRect;
            if (g_settings->get("eclipse.current_module") != parent_mod_name || g_settings->get("eclipse.current_category") != parent_category_name) {
                continue;
            }
            // if mouse event ensure its in clip rect
            if (event.EventType == EET_MOUSE_INPUT_EVENT && !(event.MouseInput.Event == EMIE_MOUSE_WHEEL)) {
                if (!clipRect.isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                    edit_box->loseFocus();
                    continue;
                }
            }

            if (edit_box->handleEvent(event)) {
                g_settings->set( pair.second.setting_id, edit_box->getTextUtf8());
                return true;
            }
        }
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
            if (m_picking_color) {
                m_picking_color = false;
                m_current_mouse_pos = m_color_picker_mouse_pos;
                return true;
            }
            close();
            return true; 
        }
    }
    
    if (m_picking_color) {
        // Handle color picker events
        if (event.EventType == EET_MOUSE_INPUT_EVENT) {
            if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
                core::position2d<s32> mousePos(event.MouseInput.X, event.MouseInput.Y);
                if (!m_color_selector_menu_box.isPointInside(mousePos)) {
                    // Clicked outside color picker, close it
                    m_picking_color = false;
                    m_current_mouse_pos = mousePos;
                    return true;
                }
                if (m_color_picker_exit_box.isPointInside(mousePos)) {
                    m_picking_color = false;
                    m_current_mouse_pos = mousePos;
                    return true;
                }
                if (m_color_selector_box.isPointInside(mousePos)) {
                    // Calculate selected color
                    u32 w = std::max(1, m_color_selector_box.getWidth());
                    u32 h = std::max(1, m_color_selector_box.getHeight());
                    s32 localX = mousePos.X - m_color_selector_box.UpperLeftCorner.X;
                    s32 localY = mousePos.Y - m_color_selector_box.UpperLeftCorner.Y;
                    // Load current color
                    float H, S, V;
                    loadHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    getColorAtPos(H, core::dimension2d<u32>(w, h), localX, localY, &S, &V);
                    // Set the color in settings
                    if (m_picking_color_setting) {
                        saveHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    }
                    m_color_selector_dragging = true;
                    return true;
                }
                if (m_hue_slider_box.isPointInside(mousePos)) {
                    // Calculate selected hue
                    u32 w = std::max(1, m_hue_slider_box.getWidth());
                    s32 localX = mousePos.X - m_hue_slider_box.UpperLeftCorner.X;
                    // Load current color
                    float H, S, V;
                    loadHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    getHueAtPos(core::dimension2d<u32>(w, 1), localX, &H);
                    // Set the color in settings
                    if (m_picking_color_setting) {
                        saveHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    }
                    m_hue_selector_dragging = true;
                    return true;
                }
            } else if (event.MouseInput.Event == EMIE_MOUSE_MOVED) {
                m_color_picker_mouse_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
                if (m_color_selector_dragging) {
                    // Calculate selected color
                    u32 w = std::max(1, m_color_selector_box.getWidth());
                    u32 h = std::max(1, m_color_selector_box.getHeight());
                    s32 localX = event.MouseInput.X - m_color_selector_box.UpperLeftCorner.X;
                    s32 localY = event.MouseInput.Y - m_color_selector_box.UpperLeftCorner.Y;
                    // Clamp localX/Y
                    localX = std::clamp(localX, 0, static_cast<s32>(w - 1));
                    localY = std::clamp(localY, 0, static_cast<s32>(h - 1));
                    // Load current color
                    float H, S, V;
                    loadHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    getColorAtPos(H, core::dimension2d<u32>(w, h), localX, localY, &S, &V);
                    // Set the color in settings
                    if (m_picking_color_setting) {
                        saveHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    }
                    return true;
                }
                if (m_hue_selector_dragging) {
                    // Calculate selected hue
                    u32 w = std::max(1, m_hue_slider_box.getWidth());
                    s32 localX = event.MouseInput.X - m_hue_slider_box.UpperLeftCorner.X;
                    // Clamp localX
                    localX = std::clamp(localX, 0, static_cast<s32>(w - 1));
                    // Load current color
                    float H, S, V;
                    loadHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    getHueAtPos(core::dimension2d<u32>(w, 1), localX, &H);
                    // Set the color in settings
                    if (m_picking_color_setting) {
                        saveHSV(m_picking_color_setting->m_setting_id, H, S, V);
                    }
                    return true;
                }
            } else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
                m_color_selector_dragging = false;
                m_hue_selector_dragging = false;
                return true;
            }
        }
        return false;
    }

    // In OnEvent
    if (event.EventType == EET_MOUSE_INPUT_EVENT) 
    {
        if (event.MouseInput.Event == EMIE_MOUSE_MOVED) 
        {
            m_current_mouse_pos = core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y);
            m_color_picker_mouse_pos = m_current_mouse_pos;
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

                for (size_t i = 0; i < m_settings_color_boxes.size(); ++i) 
                {
                    if (m_settings_color_boxes[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) 
                    {
                        std::string current_module_name = g_settings->get("eclipse.current_module");
                        for (auto cat : categories) {
                            for (auto mod : cat->mods) {
                                if (mod->m_name == current_module_name) {
                                    for (auto setting : mod->m_mod_settings) {
                                        if (setting->m_name == m_settings_color_names[i]) {
                                            m_picking_color = true;
                                            m_picking_color_setting = setting;
                                            return true;
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

static void clipAgainstEdge(
    core::array<core::vector2df> &poly,
    const core::rect<s32> &clip,
    int edge)
{
    core::array<core::vector2df> out;

    auto inside = [&](const core::vector2df &p)
    {
        switch (edge)
        {
            case 0: return p.X >= clip.UpperLeftCorner.X;        // left
            case 1: return p.X <= clip.LowerRightCorner.X;       // right
            case 2: return p.Y >= clip.UpperLeftCorner.Y;        // top
            case 3: return p.Y <= clip.LowerRightCorner.Y;       // bottom
        }
        return true;
    };

    auto intersect = [&](const core::vector2df &a, const core::vector2df &b)
    {
        float dx = b.X - a.X;
        float dy = b.Y - a.Y;

        switch (edge)
        {
            case 0: // left
            {
                float t = (clip.UpperLeftCorner.X - a.X) / dx;
                return core::vector2df(clip.UpperLeftCorner.X, a.Y + t*dy);
            }
            case 1: // right
            {
                float t = (clip.LowerRightCorner.X - a.X) / dx;
                return core::vector2df(clip.LowerRightCorner.X, a.Y + t*dy);
            }
            case 2: // top
            {
                float t = (clip.UpperLeftCorner.Y - a.Y) / dy;
                return core::vector2df(a.X + t*dx, clip.UpperLeftCorner.Y);
            }
            case 3: // bottom
            {
                float t = (clip.LowerRightCorner.Y - a.Y) / dy;
                return core::vector2df(a.X + t*dx, clip.LowerRightCorner.Y);
            }
        }
        return a;
    };

    for (u32 i = 0; i < poly.size(); i++)
    {
        core::vector2df A = poly[i];
        core::vector2df B = poly[(i+1) % poly.size()];

        bool Ain = inside(A);
        bool Bin = inside(B);

        if (Ain && Bin)
        {
            out.push_back(B);
        }
        else if (Ain && !Bin)
        {
            out.push_back(intersect(A, B)); 
        }
        else if (!Ain && Bin)
        {
            out.push_back(intersect(A, B)); 
            out.push_back(B);
        }
    }

    poly = out;
}

void draw2DThickLine(
    video::IVideoDriver* driver,
    core::vector2d<s32> start,
    core::vector2d<s32> end,
    video::SColor color,
    f32 width,
    const core::rect<s32>* clip)
{
    if (!driver || width <= 0.0f) return;

    float dx = float(end.X - start.X);
    float dy = float(end.Y - start.Y);
    float len = sqrtf(dx*dx + dy*dy);
    if (len <= 0.0f) return;
    dx /= len; dy /= len;

    float px = -dy * width * 0.5f;
    float py =  dx * width * 0.5f;

    core::array<core::vector2df> poly;
    poly.push_back(core::vector2df(start.X + px, start.Y + py));
    poly.push_back(core::vector2df(start.X - px, start.Y - py));
    poly.push_back(core::vector2df(end.X - px,   end.Y - py));
    poly.push_back(core::vector2df(end.X + px,   end.Y + py));

    if (clip)
    {
        for (int edge = 0; edge < 4; edge++)
            clipAgainstEdge(poly, *clip, edge);

        if (poly.size() < 3)
            return; // fully clipped away
    }

    core::array<video::S3DVertex> verts;
    core::array<u16> indices;

    for (u32 i = 0; i < poly.size(); i++) {
        const core::vector2df &p = poly[i];
        verts.push_back(video::S3DVertex(p.X, p.Y, 0, 0,0,-1, color, 0,0));
    }

    for (u32 i = 1; i+1 < poly.size(); i++)
    {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i+1);
    }

    video::SMaterial m;
    m.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
    driver->setMaterial(m);

    driver->draw2DVertexPrimitiveList(
        verts.const_pointer(), verts.size(),
        indices.const_pointer(), indices.size()/3,
        video::EVT_STANDARD,
        scene::EPT_TRIANGLES,
        video::EIT_16BIT);
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

    if (!m_dragging_mods)
    {
        float overscroll_top = std::max(0.0f, m_category_scroll - left_bound);
        float overscroll_bottom = std::min(0.0f, m_category_scroll - right_bound);

        float overscroll = overscroll_top + overscroll_bottom;

        const float k = 100.0f;
        const float d = 20.0f;
        const float mass = 1.0f;

        float accel = (-k * overscroll - d * m_category_scroll_velocity) / mass;

        m_category_scroll_velocity += accel * dtime;
        m_category_scroll += m_category_scroll_velocity * dtime;
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

        float overscroll = overscroll_top + overscroll_bottom;

        const float k = 100.0f;
        const float d = 20.0f;
        const float mass = 1.0f;

        float accel = (-k * overscroll - d * m_mods_scroll_velocity) / mass;

        m_mods_scroll_velocity += accel * dtime;
        m_mods_scroll += m_mods_scroll_velocity * dtime;
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

        core::rect<s32> mod_icon_rect(
            draw_x,
            draw_y + (mod_height * 0.25f),
            draw_x + mod_width,
            draw_y + (mod_height * 0.75f)
        );
        if (mod->m_settings_only) {
            // Icon area (middle)
            mod_icon_rect = core::rect<s32>(
                draw_x,
                draw_y + (mod_height * 0.25f),
                draw_x + mod_width,
                draw_y + (mod_height * 0.95)
            );
        }

        core::rect<s32> toggle_rect(
            draw_x + (mod_width * 0.35f),
            draw_y + (mod_height * 0.8f),
            draw_x + (mod_width * 0.65f),
            draw_y + (mod_height * 0.95f)
        );
        if (!mod->m_settings_only) {
            // Toggle rect (bottom)
            m_mods_toggle_boxes.push_back(toggle_rect);
            setAnimationTarget(mod->m_name + "_toggle_hover", toggle_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
            f32 adjustment = static_cast<f32>((0.02f * easeInOutCubic(getAnimation(mod->m_name + "_toggle_hover"))) - (0.01f * easeInOutCubic(getAnimation(mod->m_name + "_toggle_pressed"))));
            toggle_rect = core::rect<s32>(
                draw_x + (mod_width * (0.35f - adjustment)),
                draw_y + (mod_height * (0.8f - adjustment)),
                draw_x + (mod_width * (0.65f + adjustment)),
                draw_y + (mod_height * (0.95f + adjustment))
            );
        } else {
            toggle_rect = core::rect<s32>(
                draw_x,
                draw_y,
                draw_x,
                draw_y
            );
            m_mods_toggle_boxes.push_back(toggle_rect);
        }

        m_mods_boxes.push_back(mod_rect);
        m_mods_names.push_back(mod->m_name);

        // Background
        drawRoundedRectShadow(driver, mod_rect, theme.secondary, corner_radius, corner_radius, corner_radius, corner_radius, 4, 6, 0.1f, &clip);

        // Hover overlay
        setAnimationTarget(mod->m_name + "_hover", mod_rect.isPointInside(m_current_mouse_pos) && clip.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
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

        
        if (!mod->m_settings_only) {
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
        }

        // Draw mod icon if available
        video::ITexture *icon_texture = nullptr;

        if (!mod->m_icon.empty()) {
            if (m_is_main_menu) { // if on main menu, get full path
                icon_texture = m_texture_src->getTexture(porting::path_share + DIR_DELIM + "textures" + DIR_DELIM + "base" + DIR_DELIM + "pack" + DIR_DELIM + mod->m_icon);
            } else {
                icon_texture = m_texture_src->getTexture(mod->m_icon);
            }
        }
        if (!icon_texture || (icon_texture->getSize().Width == 1 && icon_texture->getSize().Height == 1)) {
            if (m_is_main_menu) { // if on main menu, get full path
                icon_texture = m_texture_src->getTexture(porting::path_share + DIR_DELIM + "textures" + DIR_DELIM + "base" + DIR_DELIM + "pack" + DIR_DELIM + "eclipse_icon_placeholder.png");
            } else {
                icon_texture = m_texture_src->getTexture("eclipse_icon_placeholder.png");
            }
        }
        if (icon_texture) {
            core::dimension2d<u32> size = icon_texture->getSize();
            float icon_aspect = (float)size.Width / size.Height;
            float rect_aspect = (float)mod_icon_rect.getWidth() / mod_icon_rect.getHeight();

            core::rect<s32> icon_draw_rect = mod_icon_rect;

            if (icon_aspect > rect_aspect) {
                s32 new_height = (s32)(mod_icon_rect.getWidth() / icon_aspect);
                s32 y_offset = (mod_icon_rect.getHeight() - new_height) / 2;
                icon_draw_rect.UpperLeftCorner.Y += y_offset;
                icon_draw_rect.LowerRightCorner.Y = icon_draw_rect.UpperLeftCorner.Y + new_height;
            } else {
                s32 new_width = (s32)(mod_icon_rect.getHeight() * icon_aspect);
                s32 x_offset = (mod_icon_rect.getWidth() - new_width) / 2;
                icon_draw_rect.UpperLeftCorner.X += x_offset;
                icon_draw_rect.LowerRightCorner.X = icon_draw_rect.UpperLeftCorner.X + new_width;
            }
            const video::SColor temp[4] = {
                theme.text,
                theme.text,
                theme.text,
                theme.text,
            };

            driver->draw2DImage(
                icon_texture,
                icon_draw_rect,
                core::rect<s32>(core::vector2d<s32>(0, 0), size),
                &clip,
                temp,
                true
            );
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

    // valid types are: "bool", "slider_int", "slider_float", "text", "dropdown", "color_picker"

    const s32 base_height = 50;
    if (setting.m_type == "bool")
        return base_height;

    else if (setting.m_type == "slider_int" || setting.m_type == "slider_float")
        return base_height * 2;

    else if (setting.m_type == "text")
        return (base_height * 2) + static_cast<s32>(base_height * setting.m_size);

    else if (setting.m_type == "dropdown")
        return base_height * 2;

    else if (setting.m_type == "color_picker")
        return base_height;

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
    m_settings_color_boxes.clear();
    m_settings_color_names.clear();

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
    s32 total_height = setting_spacing * 2;
    if (current_module) {
        for (auto& setting : current_module->m_mod_settings) {
            total_height += (applyScalingFactorS32(get_setting_render_height(*setting)) + setting_spacing * 2);
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

        float overscroll = overscroll_top + overscroll_bottom;

        const float k = 100.0f;
        const float d = 20.0f;
        const float mass = 1.0f;

        float accel = (-k * overscroll - d * m_settings_scroll_velocity) / mass;

        m_settings_scroll_velocity += accel * dtime;
        m_settings_scroll += m_settings_scroll_velocity * dtime;
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
            setAnimationTarget(setting->m_name + "_hover", setting_rect.isPointInside(m_current_mouse_pos) && settings_area.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
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
                // Draw text label at top
                core::rect<s32> text_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y,
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(50)
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

                // Create eclipse edit box if not existing
                core::rect<s32> edit_box_rect = core::rect<s32>(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(50),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(10)
                );
                if (m_settings_textboxes_map.find(setting->m_name) == m_settings_textboxes_map.end()) {
                    EclipseEditBox* edit_box = new EclipseEditBox(Environment);
                    m_settings_textboxes_map[setting->m_name] = {edit_box, settings_area, current_module_name, current_category_name, setting->m_setting_id};
                    edit_box->setTextUtf8(g_settings->get(setting->m_setting_id));
                    edit_box->setEditorRect(edit_box_rect);
                }

                // Draw edit box
                EclipseEditBox* edit_box = m_settings_textboxes_map[setting->m_name].textbox;
                m_settings_textboxes_map[setting->m_name] = {edit_box, settings_area, current_module_name, current_category_name, setting->m_setting_id};
                edit_box->setEditorRect(edit_box_rect);
                edit_box->draw(driver, dtime, font, settings_area, theme, applyScalingFactorS32(4));

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
            } else if (setting->m_type == "color_picker") {
                // Draw setting text
                core::rect<s32> text_rect(
                    setting_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
                    setting_rect.UpperLeftCorner.Y,
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(60),
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

                float H, S, V;
                loadHSV(setting->m_setting_id, H, S, V);
                int R, G, B;
                HSVtoRGB(H, S, V, R, G, B);
                video::SColor current_color(255, R, G, B);

                // Draw color preview box
                core::rect<s32> color_box_rect(
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(50),
                    setting_rect.UpperLeftCorner.Y + applyScalingFactorS32(10),
                    setting_rect.LowerRightCorner.X - applyScalingFactorS32(20),
                    setting_rect.LowerRightCorner.Y - applyScalingFactorS32(10)
                );

                driver->draw2DRectangle(
                    current_color,
                    color_box_rect,
                    &settings_area
                );

                setAnimationTarget(setting->m_name + "_color_box_hover", color_box_rect.isPointInside(m_current_mouse_pos) ? 1.0 : 0.0);
                video::SColor outline_color = lerpColor(theme.text_muted, theme.text, static_cast<f32>(easeInOutCubic(getAnimation(setting->m_name + "_color_box_hover"))));
                driver->draw2DRectangleOutline(
                    color_box_rect,
                    outline_color,
                    applyScalingFactorS32(2),
                    &settings_area
                );

                m_settings_color_boxes.push_back(color_box_rect);
                m_settings_color_names.push_back(setting->m_name);
            }
            current_y += setting_height + setting_spacing;
        }
    }
}

void EclipseMenu::draw_color_picker(video::IVideoDriver* driver, gui::IGUIFont* font, ColorTheme current_theme, std::vector<ModCategory *> categories) {
    setAnimationTarget("color_picker_open", m_picking_color ? 1.0 : 0.0);

    if (m_picking_color_setting == nullptr) {
       return;
    }

    double anim_progress = easeInOutCubic(getAnimation("color_picker_open"));
    if (getAnimation("color_picker_open") > 0.0) {
        ColorTheme theme = current_theme.withAlpha(anim_progress);
        // Darken background
        u32 alpha = (u32)std::round(127.0 * anim_progress);
        video::SColor overlay_color(alpha, 0, 0, 0);

        driver->draw2DRectangle(
            overlay_color,
            core::rect<s32>(0, 0, driver->getScreenSize().Width, driver->getScreenSize().Height)
        );

        // Draw color picker background

        const core::dimension2du screensize = driver->getScreenSize();
        const core::dimension2di menusize(applyScalingFactorS32(300), applyScalingFactorS32(static_cast<s32>(325 * anim_progress)));
        const core::rect<s32> menurect(
            (screensize.Width / 2) - (menusize.Width / 2),      //x1
            (screensize.Height / 2) - (menusize.Height / 2),    //y1
            (screensize.Width / 2) + (menusize.Width / 2),      //x2
            (screensize.Height / 2) + (menusize.Height / 2)     //y2
        );
        m_color_selector_menu_box = menurect;
        const s32 corner_radius = applyScalingFactorS32(15);
        drawRoundedRectShadow(
            driver,
            menurect,
            theme.background,
            corner_radius,
            corner_radius,
            corner_radius,
            corner_radius,
            4, 6, 0.5f,
            nullptr
        );

        // Draw text and close button
        const s32 top_bar_height = applyScalingFactorS32(40);
        const core::rect<s32> topbar_rect(
            menurect.UpperLeftCorner.X,
            menurect.UpperLeftCorner.Y,
            menurect.LowerRightCorner.X,
            menurect.UpperLeftCorner.Y + top_bar_height
        );
        // Draw title text
        std::wstring wtitle = utf8_to_wide("Color Picker");
        core::rect<s32> title_rect(
            topbar_rect.UpperLeftCorner.X + applyScalingFactorS32(10),
            topbar_rect.UpperLeftCorner.Y,
            topbar_rect.LowerRightCorner.X - top_bar_height - applyScalingFactorS32(20),
            topbar_rect.LowerRightCorner.Y
        );
        draw_text_shrink_to_fit(
            driver,
            applyScalingFactorS32(24),
            wtitle,
            title_rect,
            theme.text,
            nullptr
        );
        // Draw close button
        core::rect<s32> close_button_rect(
            topbar_rect.LowerRightCorner.X - top_bar_height - applyScalingFactorS32(10),
            topbar_rect.UpperLeftCorner.Y,
            topbar_rect.LowerRightCorner.X - applyScalingFactorS32(10),
            topbar_rect.LowerRightCorner.Y
        );
        m_color_picker_exit_box = close_button_rect;
        setAnimationTarget("color_picker_close_hover", close_button_rect.isPointInside(m_color_picker_mouse_pos) ? 1.0 : 0.0);
        video::SColor close_color = lerpColor(theme.text_muted, theme.text, static_cast<f32>(easeInOutCubic(getAnimation("color_picker_close_hover"))));
        // Draw X
        s32 padding = applyScalingFactorS32(10);
        core::vector2d<s32> line1_start(
            close_button_rect.UpperLeftCorner.X + padding,
            close_button_rect.UpperLeftCorner.Y + padding
        );
        core::vector2d<s32> line1_end(
            close_button_rect.LowerRightCorner.X - padding,
            close_button_rect.LowerRightCorner.Y - padding
        );
        core::vector2d<s32> line2_start(
            close_button_rect.UpperLeftCorner.X + padding,
            close_button_rect.LowerRightCorner.Y - padding
        );
        core::vector2d<s32> line2_end(
            close_button_rect.LowerRightCorner.X - padding,
            close_button_rect.UpperLeftCorner.Y + padding
        );
        draw2DThickLine(
            driver,
            line1_start,
            line1_end,
            close_color,
            applyScalingFactorDouble(2.0),
            nullptr
        );
        draw2DThickLine(
            driver,
            line2_start,
            line2_end,
            close_color,
            applyScalingFactorDouble(2.0),
            nullptr
        );

        // Draw color selection area
        core::rect<s32> color_area(
            menurect.UpperLeftCorner.X + applyScalingFactorS32(30),
            menurect.UpperLeftCorner.Y + top_bar_height + applyScalingFactorS32(20),
            menurect.LowerRightCorner.X - applyScalingFactorS32(30),
            menurect.UpperLeftCorner.Y + menurect.getHeight() * 0.75f
        );
        m_color_selector_box = color_area;

        // compute safe dimensions
        u32 w = std::max(1, color_area.getWidth());
        u32 h = std::max(1, color_area.getHeight());

        core::dimension2du gradient_dim(w, h);

        // Load current color
        float H, S, V;
        loadHSV(m_picking_color_setting->m_setting_id, H, S, V);
        int r, g, b;
        HSVtoRGB(H, S, V, r, g, b);
        video::SColor current_color(255, r, g, b);

        // Draw color gradient box
        // Create gradient texture if not existing or size changed
        if (m_color_gradient_texture == nullptr || m_color_gradient_texture->getSize().Width != gradient_dim.Width || m_color_gradient_texture->getSize().Height != gradient_dim.Height || m_last_gradient_hue != H) {
            m_last_gradient_hue = H;
            if (m_color_gradient_texture != nullptr) {
                driver->removeTexture(m_color_gradient_texture);  // remove from driver
                m_color_gradient_texture = nullptr;
            }
            m_color_gradient_texture = generateColorGradientTexture(driver, H, gradient_dim);
        }
        // Draw gradient texture
        if (m_color_gradient_texture != nullptr) {
            driver->draw2DImage(
                m_color_gradient_texture,
                color_area,
                core::rect<s32>(0, 0, m_color_gradient_texture->getSize().Width, m_color_gradient_texture->getSize().Height),
                nullptr,
                nullptr,
                false
            );
        }
        driver->draw2DRectangleOutline(
            color_area,
            theme.text_muted,
            applyScalingFactorS32(2),
            nullptr
        );

        // Draw selected color indicator
        core::position2d indicator_position = getPosAtColor(gradient_dim, S, V);
        s32 indicator_size = applyScalingFactorS32(15);
        core::rect<s32> indicator_rect(
            color_area.UpperLeftCorner.X + indicator_position.X - (indicator_size / 2),
            color_area.UpperLeftCorner.Y + indicator_position.Y - (indicator_size / 2),
            color_area.UpperLeftCorner.X + indicator_position.X + (indicator_size / 2),
            color_area.UpperLeftCorner.Y + indicator_position.Y + (indicator_size / 2)
        );
        driver->draw2DRectangle(
            current_color,
            indicator_rect,
            &menurect
        );
        driver->draw2DRectangleOutline(
            indicator_rect,
            theme.text,
            applyScalingFactorS32(2),
            &menurect
        );

        // Draw "Hue" label
        std::wstring whue_label = utf8_to_wide("Hue");
        core::rect<s32> hue_label_rect(
            menurect.UpperLeftCorner.X + applyScalingFactorS32(30),
            menurect.UpperLeftCorner.Y + menurect.getHeight() * 0.78f,
            menurect.LowerRightCorner.X - applyScalingFactorS32(30),
            menurect.UpperLeftCorner.Y + menurect.getHeight() * 0.86f
        );
        draw_text_shrink_to_fit(
            driver,
            applyScalingFactorS32(20),
            whue_label,
            hue_label_rect,
            theme.text,
            nullptr
        );

        // Draw hue slider
        core::rect<s32> hue_slider_rect(
            menurect.UpperLeftCorner.X + applyScalingFactorS32(30),
            menurect.UpperLeftCorner.Y + menurect.getHeight() * 0.87f,
            menurect.LowerRightCorner.X - applyScalingFactorS32(30),
            menurect.UpperLeftCorner.Y + menurect.getHeight() * 0.94f
        );

        core::dimension2du hue_gradient_dim(
            std::max(1, hue_slider_rect.getWidth()),
            std::max(1, hue_slider_rect.getHeight())
        );
        m_hue_slider_box = hue_slider_rect;

        // Create hue gradient texture if not existing or size changed
        if (m_hue_gradient_texture == nullptr || m_hue_gradient_texture->getSize().Width != hue_gradient_dim.Width || m_hue_gradient_texture->getSize().Height != hue_gradient_dim.Height) {
            if (m_hue_gradient_texture != nullptr) {
                driver->removeTexture(m_hue_gradient_texture);  // remove from driver
                m_hue_gradient_texture = nullptr;
            }
            m_hue_gradient_texture = generateHueGradientTexture(driver, hue_gradient_dim);
        }
        // Draw hue gradient texture
        if (m_hue_gradient_texture != nullptr) {
            driver->draw2DImage(
                m_hue_gradient_texture,
                hue_slider_rect,
                core::rect<s32>(0, 0, m_hue_gradient_texture->getSize().Width, m_hue_gradient_texture->getSize().Height),
                nullptr,
                nullptr,
                false
            );
        }

        driver->draw2DRectangleOutline(
            hue_slider_rect,
            theme.text_muted,
            applyScalingFactorS32(2)
        );

        // Draw hue slider indicator
        core::position2d<u32> pos = getPosAtHue(hue_gradient_dim, H);
        s32 hue_indicator_width = applyScalingFactorS32(5);
        core::rect<s32> hue_indicator_rect(
            (hue_slider_rect.UpperLeftCorner.X + pos.X) - hue_indicator_width,
            hue_slider_rect.UpperLeftCorner.Y,
            (hue_slider_rect.UpperLeftCorner.X + pos.X) + hue_indicator_width,
            hue_slider_rect.LowerRightCorner.Y
        );
        video::SColor hue_indicator_color = HSVtoSColor(H, 1.0f, 1.0f);
        driver->draw2DRectangle(
            hue_indicator_color,
            hue_indicator_rect
        );
        driver->draw2DRectangleOutline(
            hue_indicator_rect,
            theme.text,
            applyScalingFactorS32(3)
        );

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
        draw_color_picker(driver, font, theme, categories);
    }
} 