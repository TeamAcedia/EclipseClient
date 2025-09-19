// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#pragma once

#include "s_eclipse_mods.h"
#include "script/cpp_api/s_base.h"
#include "script/cpp_api/s_internal.h"
#include <vector>
#include <string>

// ModSetting class that stores one mod setting
class ModSetting
{
public:
	ModSetting(const std::string &name, const std::string &setting_id, const std::string &default_value);
	~ModSetting();
	void toggle();
	void set_value(const bool &value);
	void set_value(const double &value);
	void set_value(const std::string &value);
	std::string m_name;
	std::string m_description;
	std::string m_type;
	std::string m_default;
	double m_min;
	double m_max;
	double m_steps;
	double m_size;
	std::vector<std::string *> m_options;
	std::string m_setting_id;
};

// Mods class that stores mod information and settings

class Mod
{
public:
	Mod(const std::string &name, const std::string &setting_id, const std::string &description, const std::string &icon, const std::string &default_value);
	~Mod();
	std::string m_name;
	std::string m_description;
	std::string m_icon;
	bool m_default;
	bool is_enabled();
	void toggle();
	bool has_settings();
	std::vector<ModSetting *> m_mod_settings;
	std::string m_setting_id;
};


// Categories class that stores individual mods

class ModCategory
{
public:
	ModCategory(std::string name) : m_name(name) {}
	~ModCategory();
	std::string m_name;
	std::vector<Mod *> mods;
};

// Base class that stores categories

class ScriptApiEclipseMods
		: virtual public ScriptApiBase
{
public:
	virtual ~ScriptApiEclipseMods();
	void init_mods();
	void init_mods_mainmenu();
	void print_all_mod_settings();

	std::vector<ModCategory *> m_categories;
};