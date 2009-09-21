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
-- These functions will be included with every robot and levelgen script automatically.
-- Do not tinker with these unless you are sure you know what you are doing!!
-- And even then, be careful!
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

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


--------------------------------------------------------------

-- $Id: lists.lua,v 1.6 2001/01/13 22:04:18 doug Exp $
-- http://www.bagley.org/~doug/shootout/
-- implemented by: Roberto Ierusalimschy

--------------------------------------------------------------
-- List module
-- defines a prototipe for lists
--------------------------------------------------------------

List = {first = 0, last = -1}

function List:new ()
  local n = {}
  for k,v in pairs(self) do n[k] = v end
  return n
end

function List:length ()
  return self.last - self.first + 1
end

function List:pushleft (value)
  local first = self.first - 1
  self.first = first
  self[first] = value
end

function List:pushright (value)
  local last = self.last + 1
  self.last = last
  self[last] = value
end

function List:popleft ()
  local first = self.first
  if first > self.last then error"list is empty" end
  local value = self[first]
  self[first] = nil  -- to allow collection
  self.first = first+1
  return value
end

function List:popright ()
  local last = self.last
  if self.first > last then error"list is empty" end
  local value = self[last]
  self[last] = nil  -- to allow collection
  self.last = last-1
  return value
end

function List:reverse ()
  local i, j = self.first, self.last
  while i<j do
    self[i], self[j] = self[j], self[i]
    i = i+1
    j = j-1
  end
end

function List:equal (otherlist)
  if self:length() ~= otherlist:length() then return false end
  local diff = otherlist.first - self.first
  for i1=self.first,self.last do
    if self[i1] ~= otherlist[i1+diff] then return false end
  end
  return true
end

-----------------------------------------------------------
-----------------------------------------------------------
-- Our utility object
luaUtil = LuaUtil()

-- Ensure we have a good stream of random numbers
math.randomseed( luaUtil:getMachineTime() )

