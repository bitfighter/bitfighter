--------------------------------------------------------------------------------
-- Copyright Chris Eykamp
-- See LICENSE.txt for full copyright information
--------------------------------------------------------------------------------
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
-- Convenience function: find closest item in a list of items
-- Will return nil if items has 0 elements
-- If teamIndex is specified, will only include items on team
--

--[[  TODO: These are docs I wrote but aren't included in the manual; need to figure out where to add these -CE 1/31/2021
            It's not exactly a robot method, but rather a globalish function available to all bots.
  /**
   * item findClosest(table items, int teamIndex)
   *
   * @brief Find closest item in a specfied list of items.
   *
   * @param items Items to search.  This should be a table with one or more Lua items (note that these are actual items, not item types).
   * @param teamIndex Limit search to items from this team.  If omitted, will return the closest item on any team.
   *
   * @descr The teamIndex parameter is optional; if it is omitted, items on any team will be evaluated.
   */
]]--
function findClosest(items, teamIndex)

   local closest = nil
   local minDist = 999999999
   local pos = bot:getPos()

   for indx, item in ipairs(items) do              -- Iterate over our list

      --logprint(tostring(teamIndex)..","..item:getTeamIndex())
      if teamIndex == nil or item:getTeamIndex() == teamIndex then

         -- Use distSquared because it is less computationally expensive
         -- and works great for comparing distances
         local d = point.distSquared(pos, item:getPos())  -- Dist btwn robot and TestItem

         if d < minDist then                         -- Is it the closest yet?
            closest = item
            minDist = d
         end
      end
   end

   return closest
end



--
-- The following functions are provided so users don't need to run them with the bot: prefix
--
-- Why?  why is the bot: prefix so horrible??
--

-- Documented in luaLevelGenerator.cpp
function subscribe(...)
   return bf:subscribe(...)
end

-- Documented in luaLevelGenerator.cpp
function unsubscribe(...)
   return bf:unsubscribe(...)
end

-- Documented in luaLevelGenerator.cpp
function globalMsg(...)
   return bot:globalMsg(...)
end

function findVisibleObjects(...)
  return bot:findVisibleObjects(...)
end

function findAllObjects(...)
  return bf:findAllObjects(...)
end

function getFiringSolution(...)
    return bot:getFiringSolution(...)
end


--
-- Add backwards compatibility for some API changes
--
--
-- Deprecation started in 019
--
function GameInfo()
    printDeprecationWarning("GameInfo()", "bf:getGameInfo()")
    return bf:getGameInfo()
end

function TeamInfo(index)
    printDeprecationWarning("TeamInfo(index)", "bf:getGameInfo():getTeam(index)")
    return bf:getGameInfo():getTeam(index)
end

function findObjects(...)
    printDeprecationWarning("findObjects(...)", "bot:findVisibleObjects(...)")
    return bot:findVisibleObjects(...)
end

function bot:findObjects(...)
    printDeprecationWarning("bot:findObjects(...)", "bot:findVisibleObjects(...)")
    return bot:findVisibleObjects(...)
end

function bot:findGlobalObjects(...)
    printDeprecationWarning("bot:findGlobalObjects(...)", "bf:findAllObjects(...)")
    return bf:findAllObjects(...)
end

function bot:setWeapon(index, weapon)
    printDeprecationWarning("bot:setWeapon(index, weapon)", "bot:fireWeapon(weapon) directly")
	logprint("This method now effectively does nothing.")
end

function bot:fire()
    printDeprecationWarning("bot:fire()", "bot:fireWeapon(weapon)")
	logprint("This method will now default to firing phaser if it is equipped")
	return bot:fireWeapon(Weapon.Phaser)
end

function bot:activateModule(module)
    printDeprecationWarning("bot:activateModule(module)", "bot:fireModule(module)")
	return bot:fireModule(module)
end