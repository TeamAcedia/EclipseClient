-- Eclipse - Cosmetics.lua
-- Copyright (C) 2025 ProunceDev
-- MIT License

core.load_media("crown_cape.png")
core.load_media("cape_cosmetic.obj")

local update_interval = 0.016
local frame_update_interval = 0.15
local timer = 0
local frame_timer = 0
local frame_global = 0
local cape_data = {}
local world_wind = vector.new(0,0,0)
local world_wind_target = vector.new(math.random()*2-1,0,math.random()*2-1)

local function get_frame(frame_global, frame_count)
    return frame_global % frame_count
end

local function generate_texture_string(cape_id)
    local cape_data = networking.get_cape_data(cape_id)
    if not cape_data or not cape_data.CapeTexture then
		core.log("error", "Using fallback texture")
        return "crown_cape.png^[verticalframe:" .. 10 .. ":" .. get_frame(frame_global, 10)
    else
        return "[png:" .. cape_data.CapeTexture .. "^[verticalframe:" .. cape_data.CapeAnimLength .. ":" .. get_frame(frame_global, cape_data.CapeAnimLength)
    end
end


core.register_globalstep(function(dtime)
    timer = timer + dtime
	frame_timer = frame_timer + dtime
    if timer < update_interval then return end
    timer = 0

	if frame_timer > frame_update_interval then
		frame_timer = 0

		local objects = core.get_nearby_objects(100)
    	frame_global = frame_global + 1

		if objects then
			for _, obj in ipairs(objects) do
				if obj:is_player() and networking.is_user_on_eclipse(obj:get_name()) then
					local parent_id = obj:get_id()
					if cape_data[parent_id] == nil then
						local cape_id = networking.get_selected_cape(obj:get_name())
						if cape_id ~= "unknown" then
							local object_id = core.add_active_object()
							local frame_tex = generate_texture_string(cape_id)
							local cape_obj = core.get_active_object(object_id)
							cape_obj:set_attachment(parent_id, "Body", {x=0, y=0.63, z=0.12}, {x=5, y=180, z=0}, false)
							cape_obj:set_properties({
								textures = {
									frame_tex,
								},
								mesh = "cape_cosmetic.obj",
								visual = "mesh",
								visual_size = {
									x = 4,
									y = 4,
									z = 4,
								},
								backface_culling = false,
								physical = false,
								pointable = false,
							})

							cape_data[parent_id] = {object_id = object_id, current_rotation_x = 0, current_rotation_y = 0}
						end
					else
						local cape_obj = core.get_active_object(cape_data[parent_id].object_id)
						local parent_obj = core.get_active_object(parent_id)
						local frame_tex = generate_texture_string(networking.get_selected_cape(obj:get_name()))
						cape_obj:set_properties({
							textures = {
								frame_tex,
							},
							mesh = "cape_cosmetic.obj",
							visual = "mesh",
							visual_size = {
								x = 4,
								y = 4,
								z = 4,
							},
							backface_culling = false,
							physical = false,
							pointable = false,
						})
					end
				end
			end
		end
	end

	local function update_world_wind()
		local function vector_lerp(a, b, t)
			return {
				x = a.x + (b.x - a.x) * t,
				y = a.y + (b.y - a.y) * t,
				z = a.z + (b.z - a.z) * t,
			}
		end

		local interp_factor = 0.015
		world_wind = vector_lerp(world_wind, world_wind_target, interp_factor)

		if math.random() < 0.05 then
			-- max wind strength in each direction
			local WIND_MAX = 1.0
			-- maximum change per update
			local WIND_DELTA = 1

			world_wind_target.x = math.max(-WIND_MAX, math.min(WIND_MAX, world_wind_target.x + (math.random()*2 - 1) * WIND_DELTA))
			world_wind_target.z = math.max(-WIND_MAX, math.min(WIND_MAX, world_wind_target.z + (math.random()*2 - 1) * WIND_DELTA))

		end
	end

	update_world_wind()

	for parent_id, _ in pairs(cape_data) do
		local cape_obj = core.get_active_object(cape_data[parent_id].object_id)
		local parent_obj = core.get_active_object(parent_id)
		-- calculate cape rotation based on parent object movement
		local parent_velocity = {}
		if not parent_obj then
			cape_obj:remove()
			cape_data[parent_id] = nil
			return
		end
		if parent_obj:is_local_player() then
			parent_velocity = core.localplayer:get_velocity()
		else
			parent_velocity = parent_obj:get_velocity()
		end
		local parent_yaw = parent_obj:get_rotation().y
		
		local yaw_rad = math.rad(parent_yaw)

		local forward = vector.new(-math.sin(yaw_rad), 0, math.cos(yaw_rad))
		local right = vector.new(math.cos(yaw_rad), 0, math.sin(yaw_rad))

		local parent_forwardspeed = vector.dot(parent_velocity, forward)
		local parent_strafespeed  = vector.dot(parent_velocity, right)

		local wind_forward = vector.dot(world_wind, forward)
		local wind_strafe  = vector.dot(world_wind, right)

		local function clean(x, threshold)
			threshold = threshold or 0.01
			if math.abs(x) < threshold then
				return 0
			else
				return x
			end
		end

		parent_forwardspeed = clean(parent_forwardspeed)
		parent_strafespeed  = clean(parent_strafespeed)

		
		local target_rotation_x = parent_forwardspeed * 10 + wind_forward * 10
		local target_rotation_y = parent_strafespeed * 3 + wind_strafe * 10

		if target_rotation_x < -5 then
			target_rotation_x = -5
		elseif target_rotation_x > 85 then
			target_rotation_x = 85
		end

		if target_rotation_y < -90 then
			target_rotation_y = -90
		elseif target_rotation_y > 90 then
			target_rotation_y = 90
		end

		local current_rotation_x = cape_data[parent_id].current_rotation_x
		local current_rotation_y = cape_data[parent_id].current_rotation_y

		local difference_x = target_rotation_x - current_rotation_x
		local difference_y = target_rotation_y - current_rotation_y

		current_rotation_x = current_rotation_x + (difference_x * 0.05)
		current_rotation_y = current_rotation_y + (difference_y * 0.05)

		cape_data[parent_id].current_rotation_x = current_rotation_x
		cape_data[parent_id].current_rotation_y = current_rotation_y

		cape_obj:set_attachment(parent_id, "Body", {x=0, y=0.63, z=0.12}, {x=5+current_rotation_x, y=180, z=current_rotation_y}, false)
	end
end)
