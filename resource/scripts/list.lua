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