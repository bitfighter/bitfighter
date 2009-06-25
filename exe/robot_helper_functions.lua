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
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- These functions will be included with every robot automatically.
-- Do not tinker with these unless you are sure you know what you are doing!!
-- And even then, be careful!
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

--
-- Create a reference to our bot
--
bot = LuaRobot(Robot)

--
-- Blot out some functions that seem particularly insecure
--
--[[  -- Not sure about these...
debug.debug = nil   
debug.getfenv = getfenv
debug.getregistry = nil
--]]
dofile = nil
loadfile = nil

--
-- strict.lua
-- Checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning nil will do) in a main chunk before being used
-- anywhere or assigned to inside a function.
--
local mt = getmetatable(_G)
if mt == nil then
  mt = {}
  setmetatable(_G, mt)
end

__STRICT = true
mt.__declared = {}

mt.__newindex = function (t, n, v)
  if __STRICT and not mt.__declared[n] then
    local w = debug.getinfo(2, "S").what
    if w ~= "main" and w ~= "C" then
      error("Attempted assign to undeclared variable '"..n.."'.  All vars must be declared 'local' or 'global'.", 2)
    end
    mt.__declared[n] = true
  end
  rawset(t, n, v)
end

mt.__index = function (t, n)
  if not mt.__declared[n] and debug.getinfo(2, "S").what ~= "C" then
    error("Variable '"..n.."' cannot be used if it is not first declared.", 2)
  end
  return rawget(t, n)
end

function global(...)
   for _, v in ipairs{...} do mt.__declared[v] = true end
end


--
-- Default robot name, can and should be overwritten by user robots, but we need to have something...
--
function getName()
    return( "FancyNancy")
end

--
-- Wrap getFiringSolution with some code that helps C++ sort out what type of item
-- we're handing it
--
function getFiringSolution(item)
    if(item == nil) then
        return nil
    end

    type = item:getClassID()
    if(type == nil) then
        return nil
    end

    return bot:getFiringSolution(type, item)
end

--
-- Convenience function... let user use logprint directly, without referencing the bot
--
function logprint(msg)
    bot:logprint(tostring(msg))
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

--
-- Convenience function: find closest item in a list of items
-- Will return nil if items has 0 elements
--
function findClosest(items)

    local closest = nil
    local minDist = 999999999
    local loc = bot:getLoc()

    for indx, item in ipairs(items) do              -- Iterate over our list
        -- Use distSquared because it is less computationally expensive
        -- and works great for comparing distances
        local d = loc:distSquared( item:getLoc() )  -- Dist btwn robot and TestItem

        if( d < minDist ) then                      -- Is it the closest yet?
           closest = item
           minDist = d
        end
    end

    return closest
end

--
--
--
