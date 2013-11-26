-------
--- This is our sandboxed Lua environment that gets set applied to the global environment
--- for any user script that is run.
---
--- This contains a blacklist of Lua functions known to be unsafe, taken from here:
---    http://lua-users.org/wiki/SandBoxes
---
--- Also see the following sandboxes for some good ideas:
---    https://github.com/kikito/sandbox.lua/blob/master/sandbox.lua
---    http://www.videolan.org/developers/vlc/share/lua/modules/sandbox.lua
---
---
--- NOTE/IMPORTANT:
---
--- Bitfighter will have already modified the global table quite a bit, e.g. providing
--- an alternate method for math.random, adding table.copy, etc.
---
--- *** SO BE CAREFUL ***
---

---
--- HERE BE DRAGONS...
---

print("loading sandbox")
-- print(a.a.a)  -- To trigger an error


---
--- Save some functions for use in this script.  Cleaned-up later
---
local smt = setmetatable

---
--- Unsafe functions will be nil'd out
---
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

---
--- Unsafe functions mixed with safe functions of some packages
---
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

-- All debug stuff is evil.  
-- Let's still keep 'traceback', 'getinfo', 'getlocal' for stacktracer.  Should be harmless enough.
debug.getupvalue = nil
debug.debug = nil
debug.sethook = nil
debug.getmetatable = nil
debug.gethook = nil
debug.setmetatable = nil
debug.setlocal = nil
debug.setfenv = nil
debug.setupvalue = nil
debug.getregistry = nil
debug.getfenv = nil

-- Lastly, kill _G - need to set the new _G somehow
-- _G = _G


---
--- Module protection
---
local protected_modules = "coroutine math os string table"

local function protect_module(module, module_name)
  return smt({}, {
    __index = module,
    __newindex = function(_, attr_name, _)
      error('Can not modify ' .. module_name .. '.' .. attr_name .. '. Protected by the sandbox.')
    end
  })
end

protected_modules:gsub('%S+', function(module_name)
  _G[module_name] = protect_module(_G[module_name], module_name)
end)

---
--- Clean-up
---
smt = nil