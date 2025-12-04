-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

--[[ 
    Eclipse Mod Definitions File

    This file defines all client-side mod categories and their mods.
    Only table assignments to core.mod_categories are allowed in this file.
    
    Structure:
    core.mod_categories = {
        {
            name = "Category Name",
            mods = {
                {
                    name = "Mod Name",
                    description = "Short description of the mod",
                    icon = "icon_file.png",
                    default = true | false, -- optional, defaults to false
                    settings_only = true | false, -- optional, defaults to false ( if true, mod cannot be toggled, only settings can be changed )
                    setting_id = "unique.mod.identifier",
                    settings = {
                        {
                            name = "Setting Name",
                            description = "Short description of the setting",
                            type = "bool" | "slider_int" | "slider_float" | "text" | "dropdown" | "color_picker",
                            default = <default_value> | "1,0,0", -- must match type, color_picker uses "H,S,V" format where H=0-1, S=0-1, V=0-1
                            min = <min_value>,       -- for sliders
                            max = <max_value>,       -- for sliders
                            steps = <num_steps>,     -- for sliders
                            size = <field_height>,   -- for text fields
                            options = { "Option1", "Option2" } -- for dropdown
                            setting_id = "unique.setting.identifier"
                        }
                    }
                }
            }
        }
    }

    Notes:
    - 'type' determines the input widget in the client UI.
    - 'default' must match the type of setting.
    - Only include fields relevant to the type; others can be omitted.
--]]

local skybox_options = {"Dawn", "Day", "Dusk", "Night", "Galaxy", "Aurora", "Sunset", "Fog", "Clouds"}

core.mod_categories = {
    {
        name = "Client",
        mods = {
            {
                name = "Appearance",
                description = "Customize Client Appearance",
                icon = "eclipse_appearance.png",
                setting_id = "eclipse_appearance",
                settings_only = true,
                settings = {
                    {
                        name = "Theme",
                        description = "Select Theme",
                        type = "dropdown",
                        options = {"Default", "Default Light"},
                        default = "Default",
                        setting_id = "eclipse_appearance.theme"
                    },
                    {
                        name = "Menu Scale",
                        description = "Set Menu Scaling Factor",
                        type = "dropdown",
                        options = {"50%", "75%", "100%", "125%", "150%", "175%", "200%"},
                        default = "100%",
                        setting_id = "eclipse_appearance.menu_scale"
                    }
                }
            },
            {
                name = "Better Debug",
                description = "Improved debug information display",
                icon = "eclipse_debug.png",
                setting_id = "eclipse_better_debug",
                default = true,
                settings = {
                    {
                        name = "Text Color",
                        description = "Color of the debug text",
                        type = "color_picker",
                        default = "255,0,255,255",
                        setting_id = "eclipse_better_debug.text_color"
                    }
                }
            }
        }
    },
    {
        name = "Render",
        mods = {
            {
                name = "Skybox",
                description = "Combat assistance settings",
                icon = "eclipse_skybox.png",
                setting_id = "eclipse_skybox",
                default = true,
                settings = {
                    {
                        name = "Disable Clouds",
                        description = "Toggle whether or not to render clouds",
                        type = "bool",
                        default = false,
                        setting_id = "eclipse_skybox.disable_clouds"
                    },
                    {
                        name = "Disable Sun / Moon",
                        description = "Toggle whether or not to render the sun and moon",
                        type = "bool",
                        default = false,
                        setting_id = "eclipse_skybox.disable_sun_moon"
                    },
                    {
                        name = "Use Custom Skybox",
                        description = "Toggle whether or not to use a custom skybox",
                        type = "bool",
                        default = false,
                        setting_id = "eclipse_skybox.use_custom_skybox"
                    },
                    {
                        name = "Skybox Morning Texture",
                        description = "Texture to use for the skybox",
                        type = "dropdown",
                        options = skybox_options,
                        default = "Dawn",
                        setting_id = "eclipse_skybox.morning_texture"
                    },
                    {
                        name = "Skybox Day Texture",
                        description = "Texture to use for the skybox",
                        type = "dropdown",
                        options = skybox_options,
                        default = "Day",
                        setting_id = "eclipse_skybox.day_texture"
                    },
                    {
                        name = "Skybox Evening Texture",
                        description = "Texture to use for the skybox",
                        type = "dropdown",
                        options = skybox_options,
                        default = "Dusk",
                        setting_id = "eclipse_skybox.evening_texture"
                    },
                    {
                        name = "Skybox Night Texture",
                        description = "Texture to use for the skybox",
                        type = "dropdown",
                        options = skybox_options,
                        default = "Night",
                        setting_id = "eclipse_skybox.night_texture"
                    }
                }
            },
            {
                name = "Camera",
                description = "Customize camera settings",
                icon = "eclipse_camera.png",
                setting_id = "eclipse_camera",
                default = false,
                settings = {
                    {
                        name = "View Bobbing",
                        description = "Toggle view bobbing effect",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_camera.view_bobbing"
                    },
                    {
                        name = "View Bobbing Amount",
                        description = "Adjust the intensity of view bobbing",
                        type = "slider_float",
                        default = 1.0,
                        min = 0.0,
                        max = 8.0,
                        steps = 81,
                        setting_id = "view_bobbing_amount"
                    },
                    {
                        name = "Left Hand",
                        description = "Toggle left hand view",
                        type = "bool",
                        default = false,
                        setting_id = "eclipse_camera.left_hand"
                    },
                    {
                        name = "Hand Size",
                        description = "Adjust the size of the hand model",
                        type = "slider_float",
                        default = 1.0,
                        min = 0.5,
                        max = 2.0,
                        steps = 16,
                        setting_id = "eclipse_camera.hand_size"
                    },
                    {
                        name = "Hand Bobbing",
                        description = "Toggle hand bobbing effect",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_camera.hand_bobbing"
                    },
                    {
                        name = "Hand Bobbing Amount",
                        description = "Adjust the intensity of hand bobbing",
                        type = "slider_float",
                        default = 1.0,
                        min = 0.0,
                        max = 8.0,
                        steps = 81,
                        setting_id = "eclipse_camera.hand_bobbing_amount"
                    }
                }
            }
        }
    }
}

