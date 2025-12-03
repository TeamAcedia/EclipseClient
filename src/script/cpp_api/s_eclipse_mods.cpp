// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#include "s_eclipse_mods.h"
#include <settings.h>


// ModSetting class that stores one mod setting

ModSetting::ModSetting(const std::string &name, const std::string &setting_id, const std::string &default_value)
{
	m_name = name;
	m_setting_id = setting_id;
	m_default = default_value;
	g_settings->setDefault(m_setting_id, default_value);
}

ModSetting::~ModSetting()
{
	// Delete all options
	for (auto option : m_options) {
		delete option;
	}
	m_options.clear();
}

void ModSetting::toggle()
{
	bool current = g_settings->getBool(m_setting_id);
	g_settings->setBool(m_setting_id, !current);
}

void ModSetting::set_value(const bool &value)
{
	g_settings->setBool(m_setting_id, value);
}

void ModSetting::set_value(const double &value)
{
	g_settings->setFloat(m_setting_id, value);
}

void ModSetting::set_value(const std::string &value)
{
	g_settings->set(m_setting_id, value);
}


// Mods class that stores mod information and settings

Mod::Mod(const std::string &name, const std::string &setting_id, const std::string &description, const std::string &icon, const std::string &default_value, const std::string &settings_only)
{
	m_name = name;
	m_setting_id = setting_id;
	m_description = description;
	m_icon = icon;
	m_default = (default_value == "true");
	m_settings_only = (settings_only == "true");
	g_settings->setDefault(m_setting_id, default_value);
}

Mod::~Mod()
{
	// Delete all settings
	for (auto setting : m_mod_settings) {
		delete setting;
	}
	m_mod_settings.clear();
}

bool Mod::is_enabled()
{
	return g_settings->getBool(m_setting_id);
}

void Mod::toggle()
{
	bool current = is_enabled();
	g_settings->setBool(m_setting_id, !current);
}

bool Mod::has_settings()
{
	return !m_mod_settings.empty();
}


// Categories class that stores individual mods

ModCategory::~ModCategory()
{
	// Delete all mods
	for (auto mod : mods) {
		delete mod;
	}
	mods.clear();
}


// Base class that stores categories

ScriptApiEclipseMods::~ScriptApiEclipseMods()
{
	// Delete all categories
	for (auto category : m_categories) {
		delete category;
	}
	m_categories.clear();
}

void ScriptApiEclipseMods::init_mods()
{
    SCRIPTAPI_PRECHECKHEADER

    lua_getglobal(L, "core");
    lua_getfield(L, -1, "mod_categories");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    // Iterate over categories
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_istable(L, -1)) {
            // Category
            lua_getfield(L, -1, "name");
            std::string category_name = lua_isstring(L, -1) ? lua_tostring(L, -1) : "Unnamed";
            lua_pop(L, 1);

            ModCategory* category = new ModCategory(category_name);

            // Iterate mods inside category
            lua_getfield(L, -1, "mods");
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    if (lua_istable(L, -1)) {
                        // Mod
                        lua_getfield(L, -1, "name");
                        std::string mod_name = lua_isstring(L, -1) ? lua_tostring(L, -1) : "Unnamed Mod";
                        lua_pop(L, 1);

                        lua_getfield(L, -1, "description");
                        std::string mod_desc = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                        lua_pop(L, 1);

                        lua_getfield(L, -1, "icon");
                        std::string mod_icon = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                        lua_pop(L, 1);

                        lua_getfield(L, -1, "setting_id");
                        std::string mod_setting_id = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                        lua_pop(L, 1);

                        bool default_value = false;
                        lua_getfield(L, -1, "default");
                        if (lua_isboolean(L, -1))
                            default_value = lua_toboolean(L, -1);
                        lua_pop(L, 1);

						bool settings_only = false;
						lua_getfield(L, -1, "settings_only");
						if (lua_isboolean(L, -1))
							settings_only = lua_toboolean(L, -1);
						lua_pop(L, 1);

                        Mod* mod = new Mod(mod_name, mod_setting_id, mod_desc, mod_icon, default_value ? "true" : "false", settings_only ? "true" : "false");

                        // Iterate settings inside mod
						lua_getfield(L, -1, "settings");
						if (lua_istable(L, -1)) {
							lua_pushnil(L);
							while (lua_next(L, -2) != 0) {
								if (lua_istable(L, -1)) {
									lua_getfield(L, -1, "name");
									std::string setting_name = lua_isstring(L, -1) ? lua_tostring(L, -1) : "Unnamed Setting";
									lua_pop(L, 1);

									lua_getfield(L, -1, "setting_id");
									std::string setting_id = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
									lua_pop(L, 1);

									std::string default_val = "";
									lua_getfield(L, -1, "default");
									if (lua_isstring(L, -1))
										default_val = lua_tostring(L, -1);
									else if (lua_isnumber(L, -1))
										default_val = std::to_string(lua_tonumber(L, -1));
									else if (lua_isboolean(L, -1))
										default_val = lua_toboolean(L, -1) ? "true" : "false";
									lua_pop(L, 1);

									ModSetting* setting = new ModSetting(setting_name, setting_id, default_val);

									// Description, type, min, max, steps, size
									lua_getfield(L, -1, "description");
									if (lua_isstring(L, -1))
										setting->m_description = lua_tostring(L, -1);
									lua_pop(L, 1);

									lua_getfield(L, -1, "type");
									if (lua_isstring(L, -1))
										setting->m_type = lua_tostring(L, -1);
									lua_pop(L, 1);

									lua_getfield(L, -1, "min");
									if (lua_isnumber(L, -1)) setting->m_min = lua_tonumber(L, -1);
									lua_pop(L, 1);

									lua_getfield(L, -1, "max");
									if (lua_isnumber(L, -1)) setting->m_max = lua_tonumber(L, -1);
									lua_pop(L, 1);

									lua_getfield(L, -1, "steps");
									if (lua_isnumber(L, -1)) setting->m_steps = (int)lua_tonumber(L, -1);
									lua_pop(L, 1);

									lua_getfield(L, -1, "size");
									if (lua_isnumber(L, -1)) setting->m_size = (int)lua_tonumber(L, -1);
									lua_pop(L, 1);

									// Options
									lua_getfield(L, -1, "options");
									if (lua_istable(L, -1)) {
										lua_pushnil(L);
										while (lua_next(L, -2) != 0) {
											if (lua_isstring(L, -1))
												setting->m_options.push_back(new std::string(lua_tostring(L, -1)));
											lua_pop(L, 1);
										}
									}
									lua_pop(L, 1); // pop options

									mod->m_mod_settings.push_back(setting);
								}
								lua_pop(L, 1); // pop setting table
							}
						}
						lua_pop(L, 1); // pop settings table


                        category->mods.push_back(mod);
                    }
                    lua_pop(L, 1); // pop mod table
                }
            }
            lua_pop(L, 1); // pop mods table

            m_categories.push_back(category);
        }
        lua_pop(L, 1); // pop category table
    }

    lua_pop(L, 2); // pop mod_categories + core
}



void ScriptApiEclipseMods::init_mods_mainmenu()
{
	init_mods();
}

void ScriptApiEclipseMods::print_all_mod_settings()
{
    warningstream << "Printing all mod categories, mods, and settings:" << std::endl;

    for (ModCategory *category : m_categories) {
        if (!category) continue;

        warningstream << "Category: " << category->m_name << std::endl;

        for (Mod *mod : category->mods) {
            if (!mod) continue;

            warningstream << "  Mod: " << mod->m_name << std::endl;
            warningstream << "    Description: " << mod->m_description << std::endl;
            warningstream << "    Icon: " << mod->m_icon << std::endl;
            warningstream << "    Setting ID: " << mod->m_setting_id << std::endl;
            warningstream << "    Default Enabled: " << (mod->m_default ? "true" : "false") << std::endl;

            // Print mod settings if present
            if (!mod->m_mod_settings.empty()) {
                for (ModSetting *setting : mod->m_mod_settings) {
                    if (!setting) continue;

                    warningstream << "      Setting: " << setting->m_name << std::endl;
                    warningstream << "        Description: " << setting->m_description << std::endl;
                    warningstream << "        Type: " << setting->m_type << std::endl;
                    warningstream << "        Setting ID: " << setting->m_setting_id << std::endl;
                    warningstream << "        Default: " << setting->m_default << std::endl;

                    if (setting->m_type == "slider_int" || setting->m_type == "slider_float") {
                        warningstream << "        Min: " << setting->m_min << std::endl;
                        warningstream << "        Max: " << setting->m_max << std::endl;
                        warningstream << "        Steps: " << setting->m_steps << std::endl;
                    }
                    if (setting->m_type == "text") {
                        warningstream << "        Size: " << setting->m_size << std::endl;
                    }

                    if (!setting->m_options.empty()) {
                        warningstream << "        Options: ";
                        for (std::string *option : setting->m_options) {
                            if (option) {
                                warningstream << *option << " ";
                            }
                        }
                        warningstream << std::endl;
                    }
                }
            }
        }
    }

    warningstream << "Finished printing mod settings." << std::endl;
}
