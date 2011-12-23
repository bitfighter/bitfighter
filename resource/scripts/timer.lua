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
-----------------------------------------------------------
-----------------------------------------------------------

--[[

A timer object that can be used to schedule arbitrary events to be executed
at some time in the future.  All times are given in milliseconds.


USAGE

There are four user functions:

1. Timer:scheduleOnce(event, delay)             -- Executes the event once in 'delay' ms
2. Timer:scheduleRepeating(event, delay)        -- Executes the event repeatedly every 'delay' ms
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

-- Declare our global timer object
Timer = {}

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

