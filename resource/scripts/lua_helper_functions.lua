--------------------------------------------------------------------------------
-- Copyright Chris Eykamp
-- See LICENSE.txt for full copyright information
--------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- These functions will be included with every script automatically.
-- Do not tinker with these unless you are sure you know what you are doing!!
-- And even then, be careful!
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

-----------------------------------------------------------


-- Load some additional libraries
require("geometry")   -- Load geometry functions into Geom namespace; call with Geom.function
require("timer")
require("debugger")

-- Hookup our supercharged stacktrace util
_stackTracer = require("stack_trace_plus").stacktrace


arg = arg or { }  -- Make sure arg is defined before we ban globals


-- _fillTable = {}

function table.clear(tab)
   for k,v in pairs(tab) do tab[k]=nil end
end


--
-- This will be called every tick... updates timers
--
function _tickTimer(self, deltaT)
   Timer:_tick(deltaT)     -- Really should only be called once for all bots/levelgens
end


--
-- strict.lua
-- Checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning nil will do) in a main chunk before being used
-- anywhere or assigned to inside a function.
--


-- local mt = getmetatable(getfenv())
-- if mt == nil then
--   mt = {}
--   setmetatable(getfenv(), mt)
-- end
-- 
-- __STRICT = true
-- mt.__declared = {}
-- 
-- mt.__newindex = function (t, n, v)
--   if __STRICT and not mt.__declared[n] then
--     local w = debug.getinfo(2, "S").what     -- See PiL ch 23
--     if w == "C" then                         -- It's a C function!
--       local name = debug.getinfo(2, "n").name
--       if name ~= "main" then                    -- Allowed to declare globals in main function
--          error("Attempted assign to undeclared variable '"..n.."' in function '"..(name or "<<unknown function>>").."'.\n" ..
--                "All vars must be declared with 'local'; globals must be defined either in main() or outside a function declaration.", 2)
--       end
--     end
--     mt.__declared[n] = true
--   end
--   rawset(t, n, v)
-- end
-- 
-- mt.__index = function (t, n)
--   if not mt.__declared[n] and debug.getinfo(2, "S") and debug.getinfo(2, "S").what ~= "C" then
--     error("Variable '"..n.."' cannot be used if it is not first declared.", 2)
--   end
--   return rawget(t, n)
-- end
-- 
-- function global(...)
--    for _, v in ipairs{...} do mt.__declared[v] = true end
-- end
-- 
-- 
-- function _declared(fname)
--    local mt = getmetatable(getfenv())
-- 
--    if mt.__declared[fname] then
--       return true
--    end
-- 
--    return false
-- end


--
-- Convenience function, use in place of ipairs, from PiL book sec 19.3
--     e.g.  for item in values(items) do...
-- Hopefully, this will make life easier for beginners.
--
function values(t)
    local i = 0
    local n = table.getn(t)
    return function()
        i = i + 1
        if i <= n then return t[i] end
    end
end


-- Wrapper for printing our standard deprecation warning
function printDeprecationWarning(oldFunction, newFunction)
    logprint("WARNING: '" .. oldFunction .. "' is deprecated and will be removed in a future version of Bitfighter.  Please change your scripts to use '" .. newFunction .. "'")
end