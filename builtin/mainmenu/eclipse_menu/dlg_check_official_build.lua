-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

local SETTING_NAME = "no_unofficial_build_notification"
local DOWNLOAD_URL = "https://github.com/TeamAcedia/EclipseClient/releases"

function check_official_build(parent)
	local official_build = networking.verify_official_build()

	if cache_settings:get_bool(SETTING_NAME) or official_build then
		return parent
	end

	local dlg = create_unofficial_build_dlg()
	dlg:set_parent(parent)
	parent:hide()
	dlg:show()
	ui.update()

	return dlg
end

local function get_formspec(dialogdata)
	local markup = table.concat({
		"<center>",
		"<big><b>", fgettext("Unofficial Client Detected"), "</b></big>\n\n",
		fgettext("You are running an unofficial or modified build of this client."), "\n\n",
		fgettext("This build may be unstable or incompatible with online services."), "\n\n",
		fgettext("For the best experience, please download the official release."),
		"</center>",
	})


	return table.concat({
		"formspec_version[6]",
		"size[13.8,6.6]",

		"hypertext[0.4,0.4;12.2,4.6;text;",
			core.formspec_escape(markup), "]",

		"container[0.4,5.2]",

		"button[0,0;4,0.9;dont_show;",
			fgettext("Don't show again"), "]",

		"button[4.25,0;4.5,0.9;dismiss;",
			fgettext("Dismiss"), "]",

		"style[download;bgcolor=#2d7dd2]",
		"button[9,0;4,0.9;download;",
			fgettext("Download official build"), "]",

		"container_end[]",
	})
end

local function buttonhandler(this, fields)
	local parent = this.parent

	if fields.dismiss then
		this:delete()
		parent:show()
		return true
	end

	if fields.dont_show then
		cache_settings:set_bool(SETTING_NAME, true)
		this:delete()
		parent:show()
		return true
	end

	if fields.download then
		core.open_url(DOWNLOAD_URL)
		return true
	end
end

local function eventhandler(event)
	if event == "DialogShow" then
		mm_game_theme.set_engine()
		return true
	elseif event == "MenuQuit" then
		-- Prevent ESC close
		return true
	end
	return false
end

function create_unofficial_build_dlg()
	return dialog_create(
		"dlg_unofficial_build",
		get_formspec,
		buttonhandler,
		eventhandler
	)
end
