-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

core.load_media("placehold.obj")
core.load_media("placehold.png")
core.load_media("font_atlas.png")

function core.get_nearby_objects(radius)
	local player = core.localplayer
	if not player then return end

	return core.get_objects_inside_radius(player:get_pos(), radius)
end

eclipse = {
	account_type_color_codes = {
		Developer = "darkcyan",
		Partner = "aqua",
		User = "gainsboro",
		Guest = "gray",
	}
}

local scriptpath = core.get_builtin_path()
local eclipsepath = scriptpath .. "eclipse" .. DIR_DELIM
local font_map = dofile(eclipsepath .. "font_map.lua")

function eclipse.make_text_texture(text)
    local parts = {}
    local offset_x = 0
    local current_color = nil

    local i = 1
    while i <= #text do
        local ch = text:sub(i,i)

        -- Check for color code pattern :<color>:
        if ch == ":" then
            local next_colon = text:find(":", i + 1)
            if next_colon then
                local color_code = text:sub(i + 1, next_colon - 1)
                -- If color_code is non-empty, set current_color
                if color_code ~= "" then
                    current_color = color_code
                end
                i = next_colon + 1
            else
                i = i + 1
            end
        else
            local info = font_map.chars[ch]
            if info then
                local part = string.format(
                    ":%d,0=(font_atlas.png\\^[sheet\\:%dx%d\\:%d,%d)",
                    offset_x,
                    font_map.cols,
                    font_map.rows,
                    info.x,
                    info.y
                )
                offset_x = offset_x + info.width

                if current_color then
                    part = part .. "\\^[multiply\\:" .. current_color
                end

                table.insert(parts, part)
            end
            i = i + 1
        end
    end

    local texture = string.format(
        "[combine:%dx%d%s",
        offset_x + font_map.chars["A"].width,
        font_map.tile_size.y,
        table.concat(parts)
    )

    return {texture = texture, width = offset_x + font_map.chars["A"].width, height = font_map.tile_size.y}
end




function eclipse.on_connect(func)
	if not core.localplayer then core.after(0,function() eclipse.on_connect(func) end) return end
	if func then func() end
end

core.register_chatcommand("list_eclipse_users", {
	description = core.gettext("List online eclipse users"),
	func = function(param)
		local users = networking.EclipseUsers
		if not users or #users == 0 then
			return true, core.gettext("No Eclipse users online.")
		end

		local user_list = {}
		for _, user in ipairs(users) do
			table.insert(user_list, user.username .. " as " .. user.joined_name)
		end
		local user_string = table.concat(user_list, ", ")
		return true, core.gettext("Eclipse users online: ") .. user_string
	end
})

core.register_chatcommand("list_capes", {
	description = core.gettext("List all loaded capes"),
	func = function(param)
		local capes = networking.Capes
		if not capes or #capes == 0 then
			return true, core.gettext("No capes loaded.")
		end

		local cape_list = {}
		for _, cape in ipairs(capes) do
			table.insert(cape_list, cape.CapeID)
		end
		local cape_string = table.concat(cape_list, ", ")
		return true, core.gettext("Loaded capes: ") .. cape_string
	end
})