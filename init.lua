--[[
this file produces the actual module for lua-fb, combining the
C functionallity with the lua functionallity. You can use the C module
directly by requiring lua-fb.lua_fb directly.
--]]

-- load the c module
local lfb = require("lua-fb.lua_fb")

-- also add the lfb module to lua-db table
local ldb = require("lua-db")
ldb.lfb = lfb


-- return module table
return lfb
