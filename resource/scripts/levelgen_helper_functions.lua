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
-- These functions will be included with every levelgen script automatically.
-- Do not tinker with these unless you are sure you know what you are doing!!
-- And even then, be careful!
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

-- LUA stack overflow, logprint recursively calls itself?
-- function logprint(msg)
--     logprint("Levelgen", tostring(msg))
-- end

--
-- And two more
--
function subscribe(event)
   levelgen:subscribe(event)
end   

function unsubscribe(event)
   levelgen:unsubscribe(event)
end  


function globalMsg(message)
   levelgen:globalMsg(message)
end   

--
-- Alias vec as Point for backwards compatibility
--
function Point(x, y)
    logprint("WARNING: Use of Point(x,y) is deprecated!  This function will be removed in a future version of Bitfighter!  Please change your scripts to use point.new(x,y)!")  -- started warning in 018
    return point.new(x, y)
end

--
-- Make sure this function exists for plugins.  Many plugins will overwrite this.
--
function getArgsMenu()
    return nil
end    

--
-- Let the log know that this file was processed correctly
--
-- logprint("Loaded levelgen helper functions")