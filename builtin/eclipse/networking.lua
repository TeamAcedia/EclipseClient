-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

local get_http_api = core.get_http_api
core.get_http_api = nil

local SESSION_TOKEN_SETTING_NAME = "session_token"
local API_SERVER_ADDRESS = "http://teamacedia.baselinux.net:22222/"
--local API_SERVER_ADDRESS = "http://127.0.0.1:22222/"

local session_token = core.settings:get(SESSION_TOKEN_SETTING_NAME)

networking = {}

networking.EclipseUsers = {}
networking.Capes = {}
networking.SelectedCapes = {}
networking.AccountTypes = {}

networking.is_user_on_eclipse = function(joined_name_to_check)
    for _, user in ipairs(networking.EclipseUsers) do
        if user.joined_name == joined_name_to_check then
            return true
        end
    end
    return false
end

networking.get_user_account_name = function(joined_name_to_check)
    for _, user in ipairs(networking.EclipseUsers) do
        if user.joined_name == joined_name_to_check then
            return user.username
        end
    end
    return false
end

networking.delete_user_by_username = function(teamacedia_username)
    for i, user in ipairs(networking.EclipseUsers) do
        if user.username == teamacedia_username then
            table.remove(networking.EclipseUsers, i)
            return true
        end
    end
    return false
end


if session_token == "" or session_token == nil then return end

local function announce_join(username, server_address, server_port)
	core.log("action", username)
	local http = get_http_api()
	if not http then
		return false
	end

	-- Fire-and-forget join request
	local handle = http.fetch_async({
		url = API_SERVER_ADDRESS .. "api/server/join/",
		timeout = 15,
		post_data = core.write_json({
			token = session_token,
			joined_username = username,
			server_address = server_address,
			server_port = server_port
		}),
		extra_headers = {
			"Content-Type: application/json"
		}
	})

	return true
end

local function announce_leave(username, server_address, server_port)
	local http = get_http_api()
	if not http then
		return
	end

	local session_token = core.settings:get(SESSION_TOKEN_SETTING_NAME)
	if not session_token or session_token == "" then
		return
	end

	-- Fire-and-forget leave request
	http.fetch_async({
		url = API_SERVER_ADDRESS .. "api/server/leave/",
		timeout = 15,
		post_data = core.write_json({
			token = session_token,
			joined_username = username,
			server_address = server_address,
			server_port = server_port
		}),
		extra_headers = {
			"Content-Type: application/json"
		}
	})

	-- clear the session token
	core.settings:set(SESSION_TOKEN_SETTING_NAME, "")
end

local function fetch_Eclipse_users(server_address, server_port)
	local http = get_http_api()
	if not http then
		return
	end

	local handle = http.fetch_async({
		url = API_SERVER_ADDRESS .. "api/server/players/",
		timeout = 15,
		post_data = core.write_json({
			token = session_token,
			server_address = server_address,
			server_port = server_port
		}),
		extra_headers = {
			"Content-Type: application/json"
		}
	})

	local function check_result()
		local result = http.fetch_async_get(handle)
		if not result.completed then
			core.after(0.25, check_result)
			return
		end

		if result.succeeded and result.code == 200 then
			local ok, data = pcall(core.parse_json, result.data)
			if ok and data.players then
				-- Clear previous data
				networking.EclipseUsers = {}

				-- Save each player in the table
				for _, p in ipairs(data.players) do
					table.insert(networking.EclipseUsers, {
						joined_name = p.joined_name,
						username = p.username
					})
				end
			end
		end
	end
	check_result()
end

local function fetch_capes()
	local http = get_http_api()
	if not http then
		return
	end

	local handle = http.fetch_async({
		url = API_SERVER_ADDRESS .. "api/cosmetics/capes/",
		timeout = 15,
		post_data = core.write_json({
			token = session_token
		}),
		extra_headers = {
			"Content-Type: application/json"
		}
	})

	local function check_result()
		local result = http.fetch_async_get(handle)
		if not result.completed then
			core.after(0.25, check_result)
			return
		end

		if result.succeeded and result.code == 200 then
			local ok, data = pcall(core.parse_json, result.data)
			if ok and type(data) == "table" then
				networking.Capes = {}

				for _, c in ipairs(data) do
					table.insert(networking.Capes, {
						CapeID      	= c.CapeID,
						CapeTexture 	= c.CapeTexture,
						CapePreview 	= c.CapePreview,
						CapeAnimLength 	= c.CapeAnimLength
					})
				end
			end
		end
	end
	check_result()
end


networking.get_selected_cape = function(ingame_username)
	local teamacedia_username = networking.get_user_account_name(ingame_username)
	if teamacedia_username == false then
		return "unknown"
	end
	if networking.SelectedCapes[teamacedia_username] ~= nil then
		return networking.SelectedCapes[teamacedia_username]
	else
		networking.SelectedCapes[teamacedia_username] = "unknown"
		local http = get_http_api()
		if not http then
			return "unknown"
		end

		local handle = http.fetch_async({
			url = API_SERVER_ADDRESS .. "api/users/capes/get_selected/",
			timeout = 15,
			post_data = core.write_json({
				token = session_token,
				user = teamacedia_username
			}),
			extra_headers = {
				"Content-Type: application/json"
			}
		})

		local function check_result()
			local result = http.fetch_async_get(handle)
			if not result.completed then
				core.after(0.25, check_result)
				return
			end

			if result.succeeded and result.code == 200 then
				local ok, data = pcall(core.parse_json, result.data)
				if ok and type(data) == "table" then
					networking.SelectedCapes[teamacedia_username] = data.selected_cape
				end
			end
		end
		check_result()
		return "unknown"
	end
end

-- Helper to get cape data by ID
networking.get_cape_data = function(cape_id)
    if not networking.Capes then
        return nil
    end

    for _, c in ipairs(networking.Capes) do
        if c.CapeID == cape_id then
            return c
        end
    end

    return nil
end

networking.clear_player_cache = function(ingame_username)
	local teamacedia_username = networking.get_user_account_name(ingame_username)
	if teamacedia_username == false then
		return false
	else
		networking.SelectedCapes[teamacedia_username] = nil
		return networking.delete_user_by_username(teamacedia_username)
	end
end


networking.get_account_type = function(ingame_username)
	local teamacedia_username = networking.get_user_account_name(ingame_username)
	if teamacedia_username == false then
		return "Guest"
	end
	if networking.AccountTypes[teamacedia_username] ~= nil then
		return networking.AccountTypes[teamacedia_username]
	else
		networking.AccountTypes[teamacedia_username] = "Guest"
		local http = get_http_api()
		if not http then
			return "Guest"
		end

		local handle = http.fetch_async({
			url = API_SERVER_ADDRESS .. "api/users/get_account_type/",
			timeout = 15,
			post_data = core.write_json({
				token = session_token,
				user = teamacedia_username
			}),
			extra_headers = {
				"Content-Type: application/json"
			}
		})

		local function check_result()
			local result = http.fetch_async_get(handle)
			if not result.completed then
				core.after(0.25, check_result)
				return
			end

			if result.succeeded and result.code == 200 then
				local ok, data = pcall(core.parse_json, result.data)
				if ok and type(data) == "table" then
					networking.AccountTypes[teamacedia_username] = data.account_type
				end
			end
		end
		check_result()
		return "Guest"
	end
end

eclipse.on_connect(function()
	local server_info = core.get_server_info()
	local username = core.localplayer:get_name()
	local server_address = server_info.ip
	local server_port = tostring(server_info.port)

	announce_join(username, server_address, server_port)
	fetch_capes()
	core.register_on_shutdown(function()
		announce_leave(username, server_address, server_port)
	end)

	local update_interval = 5
	local timer = 5 -- Don't wait the first time

	core.register_globalstep(function(dtime)
		timer = timer + dtime
		if timer < update_interval then return end
		timer = 0

		local server_info = core.get_server_info()
		local server_address = server_info.ip
		local server_port = tostring(server_info.port)
		fetch_Eclipse_users(server_address, server_port)
	end)
end)

local last_online_players = {}
local leave_timer = 0

core.register_globalstep(function(dtime)
    leave_timer = leave_timer + dtime
    if leave_timer < 1 then
        return
    end
    leave_timer = 0

    local current_players = core.get_player_names() or {}

    local current_set = {}
    for _, name in ipairs(current_players) do
        current_set[name] = true
    end

    for name, _ in pairs(last_online_players) do
        if not current_set[name] then
            networking.clear_player_cache(name)
        end
    end

    last_online_players = current_set
end)
