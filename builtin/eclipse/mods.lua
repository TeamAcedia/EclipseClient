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
                            type = "bool" | "slider_int" | "slider_float" | "text" | "dropdown",
                            default = <default_value>,
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
                name = "Notifications",
                description = "Manage notifications",
                icon = "notifications.png",
                setting_id = "eclipse_notifications",
                settings = {
                    {
                        name = "Enable Notifications",
                        description = "Toggle all notifications",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_notifications.enabled"
                    },
                    {
                        name = "Notification Volume",
                        description = "Adjust volume for alerts",
                        type = "slider_float",
                        default = 0.5,
                        min = 0,
                        max = 1,
                        steps = 21,
                        setting_id = "eclipse_notifications.volume"
                    }
                }
            },
            {
                name = "Performance",
                description = "Client performance tweaks",
                icon = "performance.png",
                setting_id = "eclipse_performance",
                settings = {
                    {
                        name = "Enable FPS Cap",
                        description = "Limit the framerate",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_performance.fps_cap_enabled"
                    },
                    {
                        name = "FPS Limit",
                        description = "Set maximum FPS",
                        type = "slider_int",
                        default = 144,
                        min = 30,
                        max = 240,
                        steps = 21,
                        setting_id = "eclipse_performance.fps_limit"
                    }
                }
            }
        }
    },
    {
        name = "Gameplay",
        mods = {
            {
                name = "Combat",
                description = "Combat assistance settings",
                icon = "combat.png",
                setting_id = "eclipse_combat",
                settings = {
                    {
                        name = "Auto Aim",
                        description = "Enable auto targeting",
                        type = "bool",
                        default = false,
                        setting_id = "eclipse_combat.auto_aim"
                    },
                    {
                        name = "Reticle Size",
                        description = "Adjust reticle size",
                        type = "slider_int",
                        default = 10,
                        min = 5,
                        max = 20,
                        steps = 16,
                        setting_id = "eclipse_combat.reticle_size"
                    },
                    {
                        name = "Damage Display",
                        description = "Show damage numbers on hits",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_combat.show_damage"
                    }
                }
            },
            {
                name = "Movement",
                description = "Movement tweaks",
                icon = "movement.png",
                setting_id = "eclipse_movement",
                settings = {
                    {
                        name = "Sprint Toggle",
                        description = "Enable toggle for sprint",
                        type = "bool",
                        default = false,
                        setting_id = "eclipse_movement.sprint_toggle"
                    },
                    {
                        name = "Step Height",
                        description = "Height player can step up automatically",
                        type = "slider_float",
                        default = 0.5,
                        min = 0,
                        max = 2,
                        steps = 20,
                        setting_id = "eclipse_movement.step_height"
                    }
                }
            }
        }
    },
    {
        name = "Debug",
        mods = {
            {
                name = "Logging",
                description = "Adjust debug logging",
                icon = "logging.png",
                setting_id = "eclipse_logging",
                settings = {
                    {
                        name = "Enable Logging",
                        description = "Toggle debug logging",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_logging.enabled"
                    },
                    {
                        name = "Log Level",
                        description = "Select verbosity",
                        type = "dropdown",
                        options = {"Error", "Warning", "Info", "Debug"},
                        default = "Info",
                        setting_id = "eclipse_logging.level"
                    },
                    {
                        name = "Log File Path",
                        description = "Path to save log files",
                        type = "text",
                        default = "logs/client.log",
                        size = 1,
                        setting_id = "eclipse_logging.path"
                    }
                }
            },
            {
                name = "Network",
                description = "Network debug tools",
                icon = "network.png",
                setting_id = "eclipse_network",
                settings = {
                    {
                        name = "Enable Ping Display",
                        description = "Show server ping",
                        type = "bool",
                        default = true,
                        setting_id = "eclipse_network.ping_display"
                    },
                    {
                        name = "Simulate Lag",
                        description = "Add artificial lag for testing",
                        type = "slider_int",
                        default = 0,
                        min = 0,
                        max = 500,
                        steps = 50,
                        setting_id = "eclipse_network.simulated_lag"
                    }
                }
            }
        }
    }
}

