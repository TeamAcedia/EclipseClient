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
                    setting_id = "unique.mod.identifier",
                    settings = {
                        {
                            name = "Setting Name",
                            description = "Short description of the setting",
                            type = "bool" | "slider_int" | "slider_float" | "text" | "selectionbox",
                            default = <default_value>,
                            min = <min_value>,       -- for sliders
                            max = <max_value>,       -- for sliders
                            steps = <num_steps>,     -- for sliders
                            size = <field_height>,   -- for text fields
                            options = { "Option1", "Option2" } -- for dropdown/selectionbox
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

core.mod_categories = {
    {
        name = "Client",
        mods = {
            {
                name = "Appearance",
                description = "Customize Client Appearance",
                icon = "appearance.png",
                setting_id = "eclipse_appearance",
                settings = {
                    {
                        name = "Theme",
                        description = "Select Theme",
                        type = "dropdown",
                        options = {"Default", "Default Light", "Midnight", "Moss", "Ocean", "Outdoors"},
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
            }
        }
    },
    {
        name = "Placeholder Test",
        mods = {
            {
                name = "All Setting Types",
                description = "Demonstrates every type of setting",
                icon = "placeholder.png",
                setting_id = "placeholder.all_settings",
                settings = {
                    {
                        name = "Enable Feature",
                        description = "A boolean toggle example",
                        type = "bool",
                        default = true,
                        setting_id = "placeholder.enable_feature"
                    },
                    {
                        name = "Integer Slider",
                        description = "Example integer slider",
                        type = "slider_int",
                        default = 5,
                        min = 0,
                        max = 10,
                        steps = 10,
                        setting_id = "placeholder.int_slider"
                    },
                    {
                        name = "Float Slider",
                        description = "Example float slider",
                        type = "slider_float",
                        default = 0.5,
                        min = 0.0,
                        max = 1.0,
                        steps = 100,
                        setting_id = "placeholder.float_slider"
                    },
                    {
                        name = "Text Input",
                        description = "Example text field",
                        type = "text",
                        default = "Hello",
                        size = 20,
                        setting_id = "placeholder.text_input"
                    },
                    {
                        name = "Selection Box",
                        description = "Example selection box",
                        type = "selectionbox",
                        default = "Option1",
                        options = {"Option1", "Option2", "Option3"},
                        setting_id = "placeholder.selection_box"
                    }
                }
            }
        }
    }
}
