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
-- Main function run when robot starts, before getName().  By default does nothing.  Override in bot scripts.
--
function _main()
   if(main) then main() end
end

--
-- Default robot name, can and should be overridden by user robots, but we need to have something...
--
function getName()
    return("FancyNancy")
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
        local d = loc:distSquared(item:getLoc())    -- Dist btwn robot and TestItem

        if(d < minDist) then                        -- Is it the closest yet?
           closest = item
           minDist = d
        end
    end

    return closest
end


--
-- Convenience function... let user use logprint directly, without referencing luaUtil
--
function logprint(msg)
    luaUtil:logprint("Robot", tostring(msg))
end


--
-- This will be called every tick... update timer, then call robot's onTick() method if it exists
--
function _onTick(deltaT)
   Timer:_tick(deltaT)
   if onTick then onTick(deltaT) end
   --if getMove then getMove() end      -- TODO: Here for compatibility with older bots.  Remove this in a later release
end


--
-- Let the log know that this file was processed correctly
--
logprint("Loaded robot helper functions...")
