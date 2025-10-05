-- Eclipse - Main Menu Dialog
-- Copyright (C) 2025 ProunceDev
-- MIT License

-- Define defaulttexturedir and dump if they aren't globally accessible.
-- For example:
-- local defaulttexturedir = core.get_current_modpath() .. "/textures/"
-- local dump = minetest.debug.dump -- Or your custom dump function

local function main_menu_formspec(this)

	if cache_settings:get(LOGIN_USERNAME_SETTING_NAME)  == nil then
		cache_settings:set(LOGIN_USERNAME_SETTING_NAME, "")	
	end

	if this.hidden or (this.parent ~= nil and this.parent.hidden) then
		return ""
	end
    -- The main menu formspec defines the overall size and includes all buttons.
    -- Assuming a fixed size for the main menu dialog for simplicity.
    local width = 5 -- Example width
    local height = 15 -- Example height
    local fixed_size = false -- Or true, depending on desired behavior

    -- The 'prepend' content from your original get_formspec forms the core of this dialog.
    local formspec_elements = {
        "formspec_version[8]",
        string.format("size[%f,%f,%s]", width, height, dump(fixed_size)),
        "position[0.015,0.015]",
        "anchor[0,0]",
		"padding[0,0]",
        "bgcolor[#0000]",
        -- Local Button
        "style_type[image_button;border=false;textcolor=white;font_size=*2;padding=0;font=bold;bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. ";bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_button_hovered.png") .. "]",
        "image_button[0,1;4,0.85;;local_btn;" .. fgettext("Local") .. "]",
        -- Online Button
        "image_button[0,2;4,0.85;;online;" .. fgettext("Online") .. "]",
        -- CSMs Button
        "image_button[0,3;4,0.85;;csm;" .. fgettext("CSMs") .. "]",
        -- Content Button
        "image_button[0,4;4,0.85;;content;" .. fgettext("Content") .. "]",
        -- Cosmetics Button
        "image_button[0,5;4,0.85;;cosmetics;" .. fgettext("Cosmetics") .. "]",
        -- Settings Button
        "image_button[0,6;4,0.85;;settings;" .. fgettext("Settings") .. "]",
        -- About Button
        "image_button[0,7;4,0.85;;about;" .. fgettext("About") .. "]",
        -- Header Image
        "image[-2.65,0;9.3,1;" .. core.formspec_escape(defaulttexturedir .. "menu_header.png") .. "]",

		-- Secondary menu ( at the bottom )

		-- Logged in as button
        "style_type[image_button;border=false;textcolor=white;font_size=*0.9;padding=0;font=bold;bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_account.png") .. ";bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_account_hovered.png") .. "]",
        "image_button[0,14;2.84,0.425;;logged_in;        " .. cache_settings:get(LOGIN_USERNAME_SETTING_NAME) .. "]",

		-- Exit button
		"style_type[image_button;border=false;textcolor=white;font_size=*0.9;padding=0;font=bold;bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_exit.png") .. ";bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_exit_hovered.png") .. "]",
        "image_button[2.9,14;1.4,0.425;;exit;        " .. fgettext("Exit") .. "]",

		-- github button

		"style_type[image_button;border=false;textcolor=white;font_size=*0.9;padding=0;font=bold;bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_github.png") .. ";bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_github_hovered.png") .. "]",
        "image_button[0,13.5;1.4,0.425;;github;         " .. fgettext("Github") .. "]",

		-- discord button

		"style_type[image_button;border=false;textcolor=white;font_size=*0.9;padding=0;font=bold;bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_discord.png") .. ";bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_discord_hovered.png") .. "]",
        "image_button[1.45,13.5;1.4,0.425;;discord;         " .. fgettext("Discord") .. "]",

		-- website button

		"style_type[image_button;border=false;textcolor=white;font_size=*0.9;padding=0;font=bold;bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_web.png") .. ";bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_web_hovered.png") .. "]",
        "image_button[2.9,13.5;1.4,0.425;;website;         " .. fgettext("Website") .. "]",
    }
	return table.concat(formspec_elements, "")
end

local function main_menu_buttonhandler(this, fields)
	if this.hidden then
		return false
	end
    if fields.local_btn then
        local dlg = create_local_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
		ui.update()
        return true
    elseif fields.online then
        local dlg = create_online_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
        return true
    elseif fields.csm then
        local dlg = create_csm_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
        return true
    elseif fields.cosmetics then
        local dlg = create_cosmetics_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
        return true
    elseif fields.content then
        local dlg = create_content_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
        return true
    elseif fields.settings then
        local dlg = create_settings_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
        return true
    elseif fields.about then
        local dlg = create_about_dlg()
		dlg:set_parent(this)
		this:hide()
		dlg:show()
        return true
	elseif fields.exit then
		core.close()
		return true
	elseif fields.logged_in then
		cache_settings:set(LOGIN_PASSWORD_SETTING_NAME, "")
		start_startup_menu()
		return true
	elseif fields.github then
		core.open_url_dialog("https://github.com/TeamAcedia/EclipseClient")
		return true
	elseif fields.discord then
		core.open_url_dialog("https://discord.gg/z2bW5h6PXQ")
		return true
	elseif fields.website then
		core.open_url_dialog("https://cloak-v4.web.app")
		return true
	elseif fields.try_quit then
		return true
    end
	
    core.log("error", "Main Menu: "..dump(fields))
    return false
end

local function main_menu_eventhandler(this, event)
	if this.hidden then
		return false
	end

	if event == "MenuQuit" then
		core.close()
	end
end

local function hide_menu(this)
	this.hidden=true
end

--------------------------------------------------------------------------------
local function show_menu(this)
	this.hidden=false
end

local tabview_metatable = {
	handle_buttons            = main_menu_buttonhandler,
	handle_events             = main_menu_eventhandler,
	get_formspec              = main_menu_formspec,
	show                      = show_menu,
	hide                      = hide_menu,
	delete                    = function(self) ui.delete(self) end,
	set_parent                = function(self,parent) self.parent = parent end,
	set_fixed_size			  = function(self,state) self.fixed_size = state end,
}

function create_main_menu(name)--, size, tabheaderpos)
	local self = {}

	self.name     = name
	self.type     = "toplevel"

	setmetatable(self, { __index = tabview_metatable })

	self.fixed_size     = true
	self.hidden         = true

	ui.add(self)
	return self
end