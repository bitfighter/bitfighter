--------------------------------------------------------------------------------
-- Copyright Chris Eykamp
-- See LICENSE.txt for full copyright information
--------------------------------------------------------------------------------

-------
---
--- STOP! This file is essential to the safe operation of bitfighter.
---
--- DO NOT MODIFY THIS FILE (especially under someone else's directions) unless
--- you understand exactly what the implications may be.
---
-------

--- This is our sandbox that is applied to the global environment.
---
--- This contains a blacklist of unsafe Lua functions, taken from here:
---    http://lua-users.org/wiki/SandBoxes
---
--- NOTE/IMPORTANT:
---
--- Bitfighter will have already modified the global table quite a bit, e.g.
--- providing an alternate method for math.random, adding table.copy, etc.
---
--- *** SO BE CAREFUL ***
---

---
--- HERE BE DRAGONS...
---

-- Local reference needed after blacklisting occurs
local smt = setmetatable

-- Unsafe functions will be nil'd out
collectgarbage = nil
dofile = nil
getfenv = nil
getmetatable = nil
load = nil
loadfile = nil
loadstring = nil
rawequal = nil
rawget = nil
rawset = nil
setfenv = nil
setmetatable = nil
module = nil
package = nil
io = nil
debug = nil

-- Unsafe functions mixed with safe functions of some packages
string.dump = nil
math.randomseed = nil

-- Most 'os' stuff is unsafe - we only keep 'clock', 'difftime', 'time'
os.date = nil
os.execute = nil
os.exit = nil
os.getenv = nil
os.remove = nil
os.rename = nil
os.setlocale = nil
os.tmpname = nil

local function protect_module(module, module_name)
	return smt({}, {
		__index = module,
		__newindex = function(_, attr_name, _)
		error('Can not modify ' .. module_name .. '.' .. attr_name .. '. Protected by the sandbox')
	end
	})
end

local protected_modules = "coroutine math os string table"

protected_modules:gsub('%S+', function(module_name)
  _G[module_name] = protect_module(_G[module_name], module_name)
end)
