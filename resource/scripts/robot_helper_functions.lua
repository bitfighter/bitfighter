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
-- Returning nil will grab the next bot name; however, this is a fallback and should be overridden by robots...
--
function getName()
    return nil
end

--
-- Wrap getFiringSolution with some code that helps C++ sort out what type of item
-- we're handing it
--
function getFiringSolution(item)
    return bot:getFiringSolution(item)
end


--
-- Convenience function: find closest item in a list of items
-- Will return nil if items has 0 elements
-- If teamIndx is specified, will only include items on team
--
function findClosest(items, teamIndx)

   local closest = nil
   local minDist = 999999999
   local loc = bot:getLoc()

   for indx, item in ipairs(items) do              -- Iterate over our list

--logprint(tostring(teamIndx)..","..item:getTeamIndx())
      if teamIndx == nil or item:getTeamIndx() == teamIndx then

         -- Use distSquared because it is less computationally expensive
         -- and works great for comparing distances 
         local d = vec.distSquared(loc, item:getLoc())  -- Dist btwn robot and TestItem

         if d < minDist then                         -- Is it the closest yet?
            closest = item
            minDist = d
         end
      end
   end

   return closest
end



--
-- Convenience function... let user use logprint directly, without referencing LuaUtil
--
function logprint(msg)
    LuaUtil:logprint("Robot", tostring(msg))
end

--
-- And two more
--
function subscribe(event)
   subscribe_bot(bot, event)
end   

function unsubscribe(event)
   unsubscribe_bot(bot, event)
end  

function globalMsg(message)
   bot:globalMsg(message)
end

--
-- Let the log know that this file was processed correctly
--
logprint("Loaded robot helper functions...")

