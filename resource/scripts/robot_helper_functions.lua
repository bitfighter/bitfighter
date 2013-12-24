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
function subscribe(...)
   return bf:subscribe(...)
end   

function unsubscribe(...)
   return bf:unsubscribe(...)
end  

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
    return bot:findAllObjects(...)
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