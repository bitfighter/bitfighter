-- From https://github.com/kikito/inspect.lua
-- include the following line at the top of your source file:
-- local inspect = require 'inspect'
-- and call with print(inspect(<<var>>))

-----------------------------------------------------------------------------------------------------------------------
-- inspect.lua - v1.1.1 (2011-01)
-- Enrique García Cota - enrique.garcia.cota [AT] gmail [DOT] com
-- human-readable representations of tables.
-- inspired by http://lua-users.org/wiki/TableSerialization
-- see license info at bottom of file
-----------------------------------------------------------------------------------------------------------------------

-- Apostrophizes the string if it has quotes, but not aphostrophes
-- Otherwise, it returns a regular quoted string
local function smartQuote(str)
  if string.match( string.gsub(str,"[^'\"]",""), '^"+$' ) then
    return "'" .. str .. "'"
  end
  return string.format("%q", str )
end

local controlCharsTranslation = {
  ["\a"] = "\\a",  ["\b"] = "\\b", ["\f"] = "\\f",  ["\n"] = "\\n",
  ["\r"] = "\\r",  ["\t"] = "\\t", ["\v"] = "\\v",  ["\\"] = "\\\\"
}

local function unescapeChar(c) return controlCharsTranslation[c] end

local function unescape(str)
  local result, _ = string.gsub( str, "(%c)", unescapeChar )
  return result
end

local function isIdentifier(str)
  return string.match( str, "^[_%a][_%a%d]*$" )
end

local function isArrayKey(k, length)
  return type(k)=='number' and 1 <= k and k <= length
end

local function isDictionaryKey(k, length)
  return not isArrayKey(k, length)
end

local sortOrdersByType = {
  ['number']   = 1, ['boolean']  = 2, ['string'] = 3, ['table'] = 4,
  ['function'] = 5, ['userdata'] = 6, ['thread'] = 7
}

local function sortKeys(a,b)
  local ta, tb = type(a), type(b)
  if ta ~= tb then return sortOrdersByType[ta] < sortOrdersByType[tb] end
  if ta == 'string' or ta == 'number' then return a < b end
  return false
end

local function getDictionaryKeys(t)
  local length = #t
  local keys = {}
  for k,_ in pairs(t) do
    if isDictionaryKey(k, length) then table.insert(keys,k) end
  end
  table.sort(keys, sortKeys)
  return keys
end

local function getToStringResultSafely(t, mt)
  local __tostring = type(mt) == 'table' and mt.__tostring
  local string, status
  if type(__tostring) == 'function' then
    status, string = pcall(__tostring, t)
    string = status and string or 'error: ' .. tostring(string)
  end
  return string
end

local Inspector = {}

function Inspector:new(v, depth)
  local inspector = {
    buffer = {},
    depth = depth,
    level = 0,
    counters = {
      ['function'] = 0,
      ['userdata'] = 0,
      ['thread'] = 0,
      ['table'] = 0
    },
    pools = {
      ['function'] = setmetatable({}, {__mode = "kv"}),
      ['userdata'] = setmetatable({}, {__mode = "kv"}),
      ['thread'] = setmetatable({}, {__mode = "kv"}),
      ['table'] = setmetatable({}, {__mode = "kv"})
    }
  }

  setmetatable( inspector, { 
    __index = Inspector,
    __tostring = function(instance) return table.concat(instance.buffer) end
  } )
  return inspector:putValue(v)
end

function Inspector:puts(...)
  local args = {...}
  for i=1, #args do
    table.insert(self.buffer, tostring(args[i]))
  end
  return self
end

function Inspector:tabify()
  self:puts("\n", string.rep("  ", self.level))
  return self
end

function Inspector:up()
  self.level = self.level - 1
end

function Inspector:down()
  self.level = self.level + 1
end

function Inspector:putComma(comma)
  if comma then self:puts(',') end
  return true
end

function Inspector:putTable(t)
  if self:alreadySeen(t) then
    self:puts('<table ', self:getOrCreateCounter(t), '>')
  elseif self.level >= self.depth then
    self:puts('{...}')
  else
    self:puts('<',self:getOrCreateCounter(t),'>{')
    self:down()

      local length = #t
      local mt = getmetatable(t)

      local string = getToStringResultSafely(t, mt)
      if type(string) == 'string' and #string > 0 then
        self:puts(' -- ', unescape(string))
        if length >= 1 then self:tabify() end -- tabify the array values
      end

      local comma = false
      for i=1, length do
        comma = self:putComma(comma)
        self:puts(' '):putValue(t[i])
      end

      local dictKeys = getDictionaryKeys(t)

      for _,k in ipairs(dictKeys) do
        comma = self:putComma(comma)
        self:tabify():putKey(k):puts(' = '):putValue(t[k])
      end

      if mt then
        comma = self:putComma(comma)
        self:tabify():puts('<metatable> = '):putValue(mt)
      end
    self:up()
    
    if #dictKeys > 0 or mt then -- dictionary table. Justify closing }
      self:tabify()
    elseif length > 0 then -- array tables have one extra space before closing }
      self:puts(' ')
    end
    self:puts('}')
  end
  return self
end

function Inspector:alreadySeen(v)
  local tv = type(v)
  return self.pools[tv][v] ~= nil
end

function Inspector:getOrCreateCounter(v)
  local tv = type(v)
  local current = self.pools[tv][v]
  if not current then
    current = self.counters[tv] + 1
    self.counters[tv] = current
    self.pools[tv][v] = current
  end
  return current
end

function Inspector:putValue(v)
  local tv = type(v)

  if tv == 'string' then
    self:puts(smartQuote(unescape(v)))
  elseif tv == 'number' or tv == 'boolean' or tv == 'nil' then
    self:puts(tostring(v))
  elseif tv == 'table' then
    self:putTable(v)
  else
    self:puts('<',tv,' ',self:getOrCreateCounter(v),'>')
  end
  return self
end

function Inspector:putKey(k)
  if type(k) == "string" and isIdentifier(k) then
    return self:puts(k)
  end
  return self:puts( "[" ):putValue(k):puts("]")
end

local function inspect(t, depth)
  depth = depth or 4
  return tostring(Inspector:new(t, depth))
end

return inspect



--
-- Copyright (c) 2011, Enrique García Cota
-- All rights reserved.
-- 
-- Redistribution and use in source and binary forms, with or without modification, 
-- are permitted provided that the following conditions are met:
-- 
--   1. Redistributions of source code must retain the above copyright notice, 
--      this list of conditions and the following disclaimer.
--   2. Redistributions in binary form must reproduce the above copyright notice, 
--      this list of conditions and the following disclaimer in the documentation 
--      and/or other materials provided with the distribution.
--   3. Neither the name of inspect.lua nor the names of its contributors 
--      may be used to endorse or promote products derived from this software 
--      without specific prior written permission.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
-- WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
-- IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
-- INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
-- BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
-- DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
-- LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
-- OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
-- OF THE POSSIBILITY OF SUCH DAMAGE.
--