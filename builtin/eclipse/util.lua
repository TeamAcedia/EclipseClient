-- Eclipse
-- Copyright (C) 2025 ProunceDev
-- MIT License

core.load_media("placehold.obj")
core.load_media("placehold.png")

function core.get_nearby_objects(radius)
	local player = core.localplayer
	if not player then return end

	return core.get_objects_inside_radius(player:get_pos(), radius)
end

eclipse = {}

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