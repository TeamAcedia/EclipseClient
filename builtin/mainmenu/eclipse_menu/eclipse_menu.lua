-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

LOGIN_USERNAME_SETTING_NAME = "login_username"
LOGIN_PASSWORD_SETTING_NAME = "login_password"
local login_username = ""
local login_password = "" -- unhashed password stored temporarily while logging in
local accept_tos = "false"
local confirm_password = "" -- unhashed password stored temporarily while registering

local old_event_handler = core.event_handler
local old_button_handler = core.button_handler

local state = "init"
local current_loading_message = ""
local current_loading_progress = 0
local target_loading_progress = 0
local display_message = {}

local ui_update = nil

local function show_loading()
	local formspec = table.concat({
		"formspec_version[6]",
		"size[5,3]",
		"bgcolor[;neither;]",
		
        "image[-2.15,0;9.3,1;" .. core.formspec_escape(defaulttexturedir .. "menu_header.png") .. "]",

		"box[0,1.5;5,0.5;#888888]",
		"box[0,1.5;" .. (5 * (current_loading_progress / 100)) .. ",0.5;#00FF00]",
		"real_coordinates[true]",
		"hypertext[0,1.6;5,2;;<global size=20><center>" .. core.formspec_escape(current_loading_message) .. "</center>]",
	}, "\n")
	core.update_formspec(formspec)
end

local function show_sign_in()
	local formspec = table.concat({
		"formspec_version[6]",
		"size[5,4.5]",
		"bgcolor[;neither;]",
		
        "image[-2.15,0;9.3,1;" .. core.formspec_escape(defaulttexturedir .. "menu_header.png") .. "]",

		"style_type[image_button;border=false;textcolor=white;font_size=*1.5;padding=0;font=bold;" ..
		"bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. ";" ..
		"bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_button_hovered.png") .. "]",

		"image_button[0,1.6;5,0.8;;login;" .. fgettext("Log In") .. "]",
		"image_button[0,2.5;5,0.8;;create;" .. fgettext("Create Account") .. "]",
		"image_button[0,3.4;5,0.8;;exit;" .. fgettext("Exit") .. "]"
	}, "\n")
	core.update_formspec(formspec)
end

local function show_login()
	local formspec = table.concat({
		"formspec_version[8]",
		"size[5,6]",
		"bgcolor[;neither;]",

		-- Header image
        "image[-2.15,0;9.3,1;" .. core.formspec_escape(defaulttexturedir .. "menu_header.png") .. "]",

		-- Field styling to match buttons
		"style_type[*;border=false;textcolor=white;font_size=*1.5;padding=0;font=bold]",

		-- Username label and field
		"label[0.5,1.5;" .. fgettext("Username:") .. "]",
		"image[0.5,1.8;4,0.8;" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. "]",
		"field[0.5,1.8;4,0.8;username;;" .. core.formspec_escape(login_username or "") .. "]",

		-- Password label and field
		"label[0.5,2.9;" .. fgettext("Password:") .. "]",
		"image[0.5,3.1;4,0.8;" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. "]",
		"field[0.5,3.1;4,0.8;password;;" .. core.formspec_escape(login_password or "") .. "]",

		-- Styled buttons
		"style_type[image_button;border=false;textcolor=white;font_size=*1.5;padding=0;font=bold;" ..
		"bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. ";" ..
		"bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_button_hovered.png") .. "]",

		"image_button[0.5,4.2;4,0.8;;login;" .. fgettext("Log In") .. "]",
		"image_button[0.5,5.1;4,0.8;;back;" .. fgettext("Back") .. "]"
	}, "\n")
	core.update_formspec(formspec)
end

local function show_message()
	local formspec = table.concat({
		"formspec_version[6]",
		"size[5,5]",
		"bgcolor[;neither;]",
		
		"image[-2.15,0;9.3,1;" .. core.formspec_escape(defaulttexturedir .. "menu_header.png") .. "]",

		"box[0,1.5;5,2.2;#222222]",
		"real_coordinates[true]",
		"hypertext[0,1.6;5,2;;<global size=20><center>" .. core.formspec_escape(display_message.text or "") .. "</center>]",
		
		"style_type[image_button;border=false;textcolor=white;font_size=*1.5;padding=0;font=bold;" ..
		"bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. ";" ..
		"bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_button_hovered.png") .. "]",

		"image_button[0,3.9;5,0.8;;try_quit;" .. fgettext("OK") .. "]"
	}, "\n")
	core.update_formspec(formspec)
end

local function show_register()
	local formspec = table.concat({
		"formspec_version[8]",
		"size[5,8]",
		"bgcolor[;neither;]",	

		-- Header image
		"image[-2.15,0;9.3,1;" .. core.formspec_escape(defaulttexturedir .. "menu_header.png") .. "]",

		-- Field styling
		"style_type[*;border=false;textcolor=white;font_size=*1.5;padding=0;font=bold]",

		-- Username
		"label[0.5,1.3;" .. fgettext("Username:") .. "]",
		"image[0.5,1.55;4,0.8;" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. "]",
		"field[0.5,1.55;4,0.8;username;;" .. core.formspec_escape(login_username) .. "]",

		-- Password
		"label[0.5,2.55;" .. fgettext("Password:") .. "]",
		"image[0.5,2.8;4,0.8;" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. "]",
		"field[0.5,2.8;4,0.8;password;;" .. core.formspec_escape(login_password or "") .. "]",

		-- Confirm Password
		"label[0.5,3.8;" .. fgettext("Confirm Password:") .. "]",
		"image[0.5,4.05;4,0.8;" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. "]",
		"field[0.5,4.05;4,0.8;confirm;;" .. core.formspec_escape(confirm_password or "") .. "]",

		-- Terms checkbox and view button
		"checkbox[0.5,5.3;agree_tos;" .. fgettext("I agree to the ToS &\nPrivacy Policy") .. ";" .. core.formspec_escape(accept_tos or "false") .. "]",
		"image_button[-1,5.7;7,0.4;;view_terms;" .. fgettext("View ToS & Privacy Policy") .. "]",

		-- Styled buttons
		"style_type[image_button;border=false;textcolor=white;font_size=*2;padding=0;font=bold;" ..
		"bgimg=" .. core.formspec_escape(defaulttexturedir .. "menu_button.png") .. ";" ..
		"bgimg_hovered=" .. core.formspec_escape(defaulttexturedir .. "menu_button_hovered.png") .. "]",

		"image_button[0.5,6.2;4,0.8;;register;" .. fgettext("Register") .. "]",
		"image_button[0.5,7.2;4,0.8;;back;" .. fgettext("Back") .. "]"
	}, "\n")
	core.update_formspec(formspec)
end

local function trigger_message(text)
	display_message.text = text
	display_message.old_state = state
	state = "message"
end

function start_startup_menu()
	-- Sketchy workaround to disable ui updates while we run the startup menu
	ui_update = ui.update
	ui.update = function() end
	ui.childlist = {}
	
	mm_game_theme.clear_single("header")
	mm_game_theme.clear_single("footer")

	current_loading_progress = 0
	target_loading_progress = 0
	current_loading_message = "Connecting to server..."
	state = "init"
	core.event_handler("Step")
end

local function on_menustep(dtime)
	current_loading_progress = current_loading_progress + (target_loading_progress - current_loading_progress) * dtime * 4

	if current_loading_progress > 99 and target_loading_progress >= 100 then
		current_loading_progress = 100
		state = "finished"
		if ui_update then
			ui.update = ui_update
			ui_update = nil
		end
		init_globals()
		return
	end

	if state == "init" then
		target_loading_progress = 10
		current_loading_message = "Verifying Login Credentials..."
		state = "verify_credentials"
	end
	if state == "verify_credentials" then
		login_username = cache_settings:get(LOGIN_USERNAME_SETTING_NAME) or ""
		local hashed_pw = cache_settings:get(LOGIN_PASSWORD_SETTING_NAME)

		if login_username and login_username ~= "" and hashed_pw and hashed_pw ~= "" then
			state = "logging_in"
			core.handle_async(function(p)
				local function login_account(username, hashed_pw)
					local http = core.get_http_api()
					if not http then
						return { success = false, error = "HTTP API not available." }
					end

					local result = http.fetch_sync({
						url = p.server_addr .. "api/login/",
						timeout = 5,
						post_data = core.write_json({
							username = username,
							password = hashed_pw,
						}),
						extra_headers = {
							"Content-Type: application/json"
						}
					})

					if result.succeeded and result.code == 200 then
						local ok, data = pcall(core.parse_json, result.data)
						if ok and data.session_token then
							return {
								success = true,
								session_token = data.session_token,
								username = username,
								password = hashed_pw
							}
						end
						return { success = false, error = "Invalid response" }
					else
						return { success = false, error = result and result.data or "Network error" }
					end
				end
				local result = login_account(p.username, p.password)
				return result
			end, {
				username = login_username,
				password = hashed_pw,
				server_addr = API_SERVER_ADDRESS,
			}, function(result)
				if result.success == true then
					core.settings:set(SESSION_TOKEN_SETTING_NAME, result.session_token)
					state = "download_media"
				else
					state = "sign_in"
					trigger_message(fgettext("Login failed: ") .. result.error)
				end
			end)
		else
			state = "sign_in"
		end
	end
	if state == "sign_in" then
		target_loading_progress = 30
		current_loading_message = "Awaiting User Sign In..."
	end
	if state == "download_media" then
		target_loading_progress = 90
		current_loading_message = "Downloading Assets..."
		state = "logging_in"
			core.handle_async(function(p)
				local function fetch_capes()
					local Capes = {}
					local http = core.get_http_api()
					if not http then
						return
					end

					local handle = http.fetch_async({
						url = p.server_addr .. "api/users/capes/",
						timeout = 15,
						post_data = core.write_json({
							token = p.session_token
						}),
						extra_headers = {
							"Content-Type: application/json"
						}
					})

					-- Poll until request completes
					local result = http.fetch_async_get(handle)
					while not result.completed do
						result = http.fetch_async_get(handle)
					end

					if result.succeeded and result.code == 200 then
						local ok, data = pcall(core.parse_json, result.data)
						if ok and type(data) == "table" then
							Capes = {}

							for _, c in ipairs(data) do
								table.insert(Capes, {
									CapeID      	= c.CapeID,
									CapeTexture 	= c.CapeTexture,
									CapePreview 	= c.CapePreview,
									CapeAnimLength 	= c.CapeAnimLength
								})
							end
						end
					end
					return Capes
				end

				local function get_selected_cape()
					local http = core.get_http_api()
					if not http then
						return
					end

					local handle = http.fetch_async({
						url = p.server_addr .. "api/users/capes/get_selected/",
						timeout = 15,
						post_data = core.write_json({
							token = p.session_token
						}),
						extra_headers = {
							"Content-Type: application/json"
						}
					})

					-- Poll until request completes
					local result = http.fetch_async_get(handle)
					while not result.completed do
						result = http.fetch_async_get(handle)
					end

					if result.succeeded and result.code == 200 then
						local ok, data = pcall(core.parse_json, result.data)
						if ok and type(data) == "table" then
							return data.selected_cape
						end
					end
				end
				local capes = fetch_capes()
				local selected_cape = get_selected_cape()
				return { capes = capes, selected_cape = selected_cape }
			end, {
				server_addr = API_SERVER_ADDRESS,
				session_token = core.settings:get(SESSION_TOKEN_SETTING_NAME)
			}, function(data)
				networking.Capes = data.capes or {}
				networking.SelectedCape = data.selected_cape or nil
				core.log("action", "Capes downloaded: " .. tostring(#networking.Capes))
				target_loading_progress = 100
				current_loading_message = "Loading Main Menu..."
			end)
	end

	if state == "sign_in" then
		show_sign_in()
	elseif state == "login" then
		show_login()
	elseif state == "register" then
		show_register()
	elseif state == "message" then
		show_message()
	else
		show_loading()
	end
end


--------------------------------------------------------
-- Handlers
--------------------------------------------------------

core.event_handler = function(event)
	if state == "finished" then
		return old_event_handler(event)
	end

	if event == "Step" then
		local step_start = os.clock()
		core.handle_async(function()
			local t0 = os.clock()
			while os.clock() - t0 < (1 / 30) do end
			return true
		end, {}, function()
			local step_end = os.clock()
			local dtime = step_end - step_start
			on_menustep(dtime)
			core.event_handler("Step")
		end)
		return true
	end

	return false
end

core.button_handler = function(fields)
	if state == "finished" then
		return old_button_handler(fields)
	elseif state == "sign_in" then
		if fields.login then
			state = "login"
			return true
		elseif fields.create then
			state = "register"
			return true
		elseif fields.exit then
			core.close()
			return true
		end
	elseif state == "login" then
		if fields.back or fields.try_quit then
			state = "sign_in"
			return true
		elseif fields.login then
			if fields.username then
				login_username = fields.username
			end
			if fields.password then
				login_password = fields.password
			end

			if login_username == "" or login_password == "" then
				trigger_message(fgettext("Please enter both a username and password."))
				return true
			end

			current_loading_message = "Logging In..."
			target_loading_progress = 40
			state = "logging_in"
			core.handle_async(function(p)
				local hashed_pw = core.sha256(p.password)
				
				local function login_account(username, hashed_pw)
					local http = core.get_http_api()
					if not http then
						return { success = false, error = "HTTP API not available." }
					end

					local result = http.fetch_sync({
						url = p.server_addr .. "api/login/",
						timeout = 5,
						post_data = core.write_json({
							username = username,
							password = hashed_pw,
						}),
						extra_headers = {
							"Content-Type: application/json"
						}
					})

					if result.succeeded and result.code == 200 then
						local ok, data = pcall(core.parse_json, result.data)
						if ok and data.session_token then
							return {
								success = true,
								session_token = data.session_token,
								username = username,
								password = hashed_pw
							}
						end
						return { success = false, error = "Invalid response" }
					else
						return { success = false, error = result and result.data or "Network error" }
					end
				end
				local result = login_account(p.username, hashed_pw)
				return result
			end, {
				username = login_username,
				password = login_password,
				server_addr = API_SERVER_ADDRESS,
			}, function(result)
				if result.success == true then
					-- Save username and hashed password to settings
					cache_settings:set(LOGIN_USERNAME_SETTING_NAME, result.username)
					cache_settings:set(LOGIN_PASSWORD_SETTING_NAME, result.password)
					core.settings:set(SESSION_TOKEN_SETTING_NAME, result.session_token)

					-- Clear unhashed password from memory
					login_password = nil

					state = "download_media"
				else
					state = "login"
					target_loading_progress = 30
					trigger_message(fgettext("Login failed: ") .. result.error)
				end
			end)
		end
	elseif state == "register" then
		if fields.back or fields.try_quit then
			state = "sign_in"
			return true
		elseif fields.view_terms then
			trigger_message(fgettext(
				"Terms of Service and Privacy Policy\n\n" ..
				"By creating an account, you agree to the following:\n\n" ..
				"We only store your username and a securely hashed password used to log into our services, exclusively for login purposes.\n\n" ..
				"We do not collect personal information or IP addresses.\n\n" ..
				"We may store cosmetic preferences (such as skins, colors, or avatars) tied to your account to personalize your experience.\n\n" ..
				"If in-game messaging or social features are introduced, message content may be stored temporarily to support communication.\n\n" ..
				"Abuse, impersonation, or malicious behavior may result in account termination.\n\n" ..
				"You may request account deletion at any time through our Discord. Upon deletion, all associated account data will be removed within 7 business days.\n\n" ..
				"TeamAcedia reserves the right to terminate accounts at any time, with or without notice.\n\n" ..
				"This service is provided as-is, without warranty or guarantee of uptime or availability."
			))
			return true
		
		elseif fields.agree_tos then
			if fields.username then
				login_username = fields.username
			end
			if fields.password then
				login_password = fields.password
			end
			if fields.confirm then
				confirm_password = fields.confirm
			end
			accept_tos = fields.agree_tos
		elseif fields.register then
			if fields.username then
				login_username = fields.username
			end
			if fields.password then
				login_password = fields.password
			end
			if fields.confirm then
				confirm_password = fields.confirm
			end
			if login_username == "" or login_password == "" or confirm_password == "" then
				trigger_message(fgettext("Please fill in all fields."))
				return true
			end
			if login_password ~= confirm_password then
				trigger_message(fgettext("Passwords do not match."))
				return true
			end
			if accept_tos ~= "true" then
				trigger_message(fgettext("You must agree to the Terms of Service and Privacy Policy to register."))
				return true
			end
			current_loading_message = "Creating Account..."
			target_loading_progress = 40
			state = "registering"
			core.handle_async(function(p)
				local hashed_pw = core.sha256(p.password)
				
				local function register_account(username, hashed_pw)
					local http = core.get_http_api()
					if not http then
						return { success = false, error = "HTTP API not available." }
					end

					local result = http.fetch_sync({
						url = p.server_addr .. "api/register/",
						timeout = 5,
						post_data = core.write_json({
							username = username,
							password = hashed_pw,
						}),
						extra_headers = {
							"Content-Type: application/json"
						}
					})

					if result.succeeded and result.code == 201 then
						local ok, data = pcall(core.parse_json, result.data)
						if ok then
							return {
								success = true,
								username = username,
								password = hashed_pw
							}
						end
						return { success = false, error = "Invalid response" }
					else
						return { success = false, error = result and result.data or "Network error" }
					end
				end

				local function login_account(username, hashed_pw)
					local http = core.get_http_api()
					if not http then
						return { success = false, error = "HTTP API not available." }
					end

					local result = http.fetch_sync({
						url = p.server_addr .. "api/login/",
						timeout = 5,
						post_data = core.write_json({
							username = username,
							password = hashed_pw,
						}),
						extra_headers = {
							"Content-Type: application/json"
						}
					})

					if result.succeeded and result.code == 200 then
						local ok, data = pcall(core.parse_json, result.data)
						if ok and data.session_token then
							return {
								success = true,
								session_token = data.session_token,
								username = username,
								password = hashed_pw
							}
						end
						return { success = false, error = "Invalid response" }
					else
						return { success = false, error = result and result.data or "Network error" }
					end
				end

				local result = register_account(p.username, hashed_pw)
				if result.success == false then
					return result
				end
				return login_account(p.username, hashed_pw)
			end, {
				username = login_username,
				password = login_password,
				server_addr = API_SERVER_ADDRESS,
			}, function(result)
				if result.success == true then
					-- Save username and hashed password to settings
					cache_settings:set(LOGIN_USERNAME_SETTING_NAME, result.username)
					cache_settings:set(LOGIN_PASSWORD_SETTING_NAME, result.password)

					-- Clear unhashed passwords from memory
					login_password = nil
					confirm_password = nil

					state = "download_media"
				else
					state = "register"
					target_loading_progress = 30
					trigger_message(fgettext("Account creation failed: ") .. result.error)
				end
			end)
		end
	elseif state == "message" then
		if fields.try_quit then
			state = display_message.old_state
			return true
		elseif fields.back then
			state = display_message.old_state
			return true
		end
	end
	if fields.try_quit then
		core.close()
		return true
	end

	return false
end