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


-----------------------------------------------------------
-----------------------------------------------------------
--[[
Based on code:
   Copyright 2009 Haywood Slap
   Modified by Chris Eykamp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

]]

--[[

A timer object that can be used to schedule arbitrary events to be executed
at some time in the future.  All times are given in milliseconds.


USAGE

There are four user functions:

1. Timer:scheduleOnce(event, delay)             -- Executes the event once in 'delay' ms
2. Timer:scheduleRepeating(event, delay)        -- Executes the event every 'delay' ms
3. Timer:scheduleRepeatWhileTrue(event, delay)  -- Executes the event every 'delay' ms while event returns true
4. Timer:clear()                                -- Removes all pending events from the timer's queue


EVENTS

The 'event' used can either be the name a function, a table that contains
a function named 'run', or a bit of arbitrary Lua code.

Any Lua function can be called by the Timer.


EXAMPLES

function onLevelStart()
   Timer:scheduleOnce("logprint(\"Once!\")", 5000)   -- compiles and runs string in five seconds
   Timer:scheduleRepeating(always, 30000)            -- runs always function every 30 seconds
   Timer:scheduleRepeatWhileTrue(maybe, 2000)        -- runs "run" method of maybe every two seconds
end                                                  -- until method returns false

-- Standard function
function always()
   logprint("Timer called always")
end

-- Table: run method will be called
maybe = {
   count = 3
   run = function(self)
      logprint("Count down " .. self.count)
      self.count = self.count - 1
      return self.count > 0
   end
}

When using a table as an 'event' the first parameter to the run
function should always be a parameter named "self" (or "this"
if you prefer).  The "self" parameter can be used to access the
other fields in the table, that is, the "self" parameter will refer
to the table that contains the run function.  If you do not need to
refer to any of the other fields in the table then you do not need
to include the self parameter.

]]


-- Initialize a new timer object, called once when object is instantiated
function Timer:_initialize()
   self.queue = {}   -- List of events (i.e. functions) waiting to be called
   self.time = 0     -- Accumulated time (ms)
end


-- Schedule an event to be executed only once in 'time' ms
function Timer:scheduleOnce(event, deltaT)
   local record = {event = event, time = self.time + deltaT }
   self:_insert(record)
end


-- Schedule an event to be repeated every 'time' ms
function Timer:scheduleRepeating(event, deltaT)
   local record = {event = event, time = self.time + deltaT, repeating = deltaT}
   self:_insert(record)
end

-- Schedule an event to be executed every 'time' seconds. The event will be repeated
-- only if the call to 'event' returns true.
function Timer:scheduleRepeatWhileTrue(event, deltaT)
   local record = {event = event, time = self.time + deltaT, repeatIf = deltaT}
   self:_insert(record)
end


-- Removes all pending events from the timer queue
function Timer:clear()
   self.queue = {}
end


-- Checks the current time and fires any scheduled events. This is called by the C++ code.
function Timer:_tick(timeDelta)
   self.time = self.time + timeDelta

   -- Return if there are no events
   if #self.queue == 0 then return end

   -- Check the front of the queue for events that need to be fired.  Remember that
   -- events are sorted by time so as soon as we find an event that doesn't need to
   -- be fired, we're done.
   local now = self.time
   local record = self.queue[1]

   while record and now > record.time do
      -- This event needs to be fired
      local reschedule = false

      if type(record.event) == "string" then
         -- Compile the string and execute it
         record.event = loadstring(record.event)
         if record.event == nil then
            logprint("Timer: Error compiling string: " .. record.event)
         end
         reschedule = record.event()         -- Execute the compiled code

      elseif type(record.event) == "table" then
         reschedule = record.event:run()     -- Execute the run method on the table

      else
         -- Assume record.event is a function
         reschedule = record.event()         -- Execute it and see what happens!
      end

      -- Remove the item at the front of the queue and reschedule the event if it repeats
      table.remove(self.queue, 1)
      if record.repeating then
         self:scheduleRepeating(record.event, record.repeating)
      elseif record.repeatIf and reschedule then
         self:scheduleRepeatWhileTrue(record.event, record.repeatIf)
      end

      -- Check the next item, now at the front of the queue
      record = self.queue[1]
   end
end


-- Inserts 'record' into the proper position in the timer queue.
-- The queue is ordered so that events that need to fire first
-- are at the front of the queue.
function Timer:_insert(record)
   -- Start at the end of the queue and work towards the front
   local n = #self.queue + 1

   -- Find the position in the queue to insert the event
   while n > 1 do
      if self.queue[n - 1].time > record.time then
         n = n - 1
      else
         break  -- The record goes here, break out of the loop
      end
   end
   -- We have either found the correct location or hit the
   -- front of the queue.
   table.insert(self.queue, n, record)
end


Timer:_initialize()     -- Actually intialize the timer