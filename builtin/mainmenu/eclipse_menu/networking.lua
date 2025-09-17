-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

API_SERVER_ADDRESS = "http://teamacedia.baselinux.net:22222/"
--API_SERVER_ADDRESS = "http://127.0.0.1:22222/"

SESSION_TOKEN_SETTING_NAME = "session_token"

networking = {}

networking.Capes = {}
networking.SelectedCape = ""

function register_account(login_username, hashed_pw)
	local http = core.get_http_api()
	if not http then
		return "HTTP API not available."
	end

	local handle = http.fetch_async({
		url = API_SERVER_ADDRESS .. "api/register/",
		timeout = 5,
		post_data = core.write_json({
			username = login_username,
			password = hashed_pw
		}),
		extra_headers = {
			"Content-Type: application/json"
		}
	})

	local result = http.fetch_async_get(handle)
	while not result.completed do
		result = http.fetch_async_get(handle)
	end

	if result.succeeded and result.code == 201 then
		return true
	else
		
		return result and result.data or "Network error: Failed to contact login server."
	end
end

function set_selected_cape(cape_id)

	local session_token = core.settings:get(SESSION_TOKEN_SETTING_NAME)

	local http = core.get_http_api()
	if not http then
		return
	end

	local handle = http.fetch_async({
		url = API_SERVER_ADDRESS .. "api/users/capes/set_selected/",
		timeout = 15,
		post_data = core.write_json({
			token = session_token,
			cape = cape_id
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
		if ok then
			networking.SelectedCape = cape_id
			return true
		end
	end
	return false
end