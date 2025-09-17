-- Eclipse - Init.lua
-- Copyright (C) 2025 ProunceDev
-- MIT License

local scriptpath = core.get_builtin_path()
local eclipsepath = scriptpath.."eclipse"..DIR_DELIM

dofile(eclipsepath .. "util.lua")
dofile(eclipsepath .. "networking.lua")
dofile(eclipsepath .. "cosmetics.lua")

assert(core.get_http_api == nil) -- Ensure http API is disabled for security