-------------------------------------------------------------------------------
--
-- Bitfighter - A multiplayer vector graphics space game
-- Based on Zap demo released for Torque Network Library by GarageGames.com
--
-- Copyright (C) 2008-2009 Chris Eykamp
-- Other code copyright as noted
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful (and fun!),
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
--
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- These functions will be included with every robot and levelgen script automatically.
-- Do not tinker with these unless you are sure you know what you are doing!!
-- And even then, be careful!
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

-----------------------------------------------------------


-- Smarter implementation of dofile; finds script before loading it into current environment
function include(filename)
   fullName = findFile(filename)
   
   if(fullName) then
      f = loadfile(fullName)
      setfenv(f, getfenv())
      return f()
   end
end


-- Load some additional libraries
include("geometry")   -- Load geometry functions into Geom namespace; call with Geom.function
include("timer")
include("list")




arg = arg or { }  -- Make sure arg is defined before we ban globals


-- _fillTable = {}

function table.clear(tab)
   for k,v in pairs(tab) do tab[k]=nil end
end


--
-- This will be called every tick... updates timers
--
function _tickTimer(self, deltaT)
   Timer:_tick(deltaT)     -- Really should only be called once for all bots
end


--
-- strict.lua
-- Checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning nil will do) in a main chunk before being used
-- anywhere or assigned to inside a function.
--


local mt = getmetatable(getfenv())
if mt == nil then
  mt = {}
  setmetatable(getfenv(), mt)
end

__STRICT = true
mt.__declared = {}

mt.__newindex = function (t, n, v)
  if __STRICT and not mt.__declared[n] then
    local w = debug.getinfo(2, "S").what     -- See PiL ch 23
    if w == "C" then                         -- It's a C function!
      local name = debug.getinfo(2, "n").name
      if name ~= "main" then                    -- Allowed to declare globals in main function
         error("Attempted assign to undeclared variable '"..n.."' in function '"..(name or "<<unknown function>>").."'.\n" ..
               "All vars must be declared with 'local'; globals must be defined either in main() or outside a function declaration.", 2)
      end
    end
    mt.__declared[n] = true
  end
  rawset(t, n, v)
end

mt.__index = function (t, n)
  if not mt.__declared[n] and debug.getinfo(2, "S") and debug.getinfo(2, "S").what ~= "C" then
    error("Variable '"..n.."' cannot be used if it is not first declared.", 2)
  end
  return rawget(t, n)
end

function global(...)
   for _, v in ipairs{...} do mt.__declared[v] = true end
end


function _declared(fname)
   local mt = getmetatable(getfenv())

   if mt.__declared[fname] then
      return true
   end

   return false
end


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
