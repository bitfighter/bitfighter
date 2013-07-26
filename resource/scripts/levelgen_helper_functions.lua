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
-- These functions will be included with every levelgen script automatically.
-- Do not tinker with these unless you are sure you know what you are doing!!
-- And even then, be careful!
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

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
-- Add backwards compatibility for some API changes
--
--
-- Deprecation started in 019
--
function levelgen:findObjectById(id)
    return bf:findObjectById(id)
end

function levelgen:addItem(object)
    return bf:addItem(object)
end

function GameInfo()
    printDeprecationWarning("GameInfo()", "bf:getGameInfo()")
    return bf:getGameInfo()
end

function TeamInfo(index)
    printDeprecationWarning("TeamInfo(index)", "bf:getGameInfo():getTeam(index)")
    return bf:getGameInfo():getTeam(index)
end

--
-- Deprecation started in 018
--
function Point(x, y)
    printDeprecationWarning("Point(x,y)", "point.new(x,y)")
    return point.new(x, y)
end

--
-- Let the log know that this file was processed correctly
--
-- logprint("Loaded levelgen helper functions")