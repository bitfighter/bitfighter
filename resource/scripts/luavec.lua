--[[

@luaclass point

@brief A simple object representing a coordinate pair.

@descr Points are used to represent locations and positions in the game.
       Note that for historical reasons, "point" is lower case.
       Points can be added and subtracted from one another, as well as multiplied together.
       Most point methods are static.
@code
    function getAngleDiff(bullet, botPos)     -- BfObject, point
        local o = point.zero                  -- Origin; using builtin constant rather than creating new object
        local bulletPos = bullet:getPos()
        print(bulletPos.x, bulletPos.y)       -- Access coordinates with dot notation
        local bulletVel = bullet:getVel()
        return math.abs(angleDifference(point.angleTo(o, bulletVel), point.angleTo(bulletPos, botPos)))
    end
@endcode
--]]


point = {}

-- For saving before sandbox wipes these out
local tms = setmetatable
local tmg = getmetatable

local mt = {}

--[[
@luafunc point.point(num x, num y)
 --]]
function point.new(x,y)
  local x=x or 0
  local y=y or 0
  local v={x=x,y=y}
  tms(v,mt)
  return v
end

--[[
@luafunc static point point.dot(point p1, point p2)
@brief Compute the dot product of two points.
@descr Computes and returns a %point representing the dot product of two passed points.
 --]]
function point.dot(v1,v2)
  return v1.x*v2.x+v1.y*v2.y
end

--[[
@luafunc static point point.cross(point p1, point p2)
@brief Compute the cross product of two points.
@descr Computes and returns a %point representing the cross product of two passed points.
 --]]
 function point.cross(v1,v2)
  return point.new(v1.x*v2.y-v1.y*v2.x,0)
end

--[[
@luafunc static point point.normalize(point p)
@brief Normalizes the length of a vector represented by a point to have a length of 1.
@descr If the input vector is (3, 0), the output will be (1, 0).
 --]]
function point.normalize(v)
  local l = point.length(v)
  if l > 0 then
    local inv = 1.0/l
    v.x=v.x*inv
    v.y=v.y*inv
  end

  return v
end

--[[
@luafunc static num point.length(point p)
@brief Compute the length of the vector contained in the passed %point.
@descr Computes the length of the vector contained in the passed %point by using
       the Pythagorean equation; that is, it returns sqrt(p.x^2 + p.y^2).
 --]]
function point.length(v)
  return math.sqrt(v.x*v.x+v.y*v.y)
end

--[[
@luafunc static num point.lengthSquared(point p)
@brief Compute the square of the length of the vector contained in the passed %point.
@descr Computes the square of the length of the vector contained in the passed %point by using
       the Pythagorean equation; that is, it returns (p.x^2 + p.y^2).  This is useful if you
       are comparing lengths to one another, and don't need the actual length.  Not computing
       the sqrt will make your scripts run more efficiently.
 --]]
function point.lengthSquared(v)
  return v.x*v.x+v.y*v.y
end

--[[
@luafunc static num point.angleTo(point p1, point p2)
@brief Compute the angle between vectors represented in the two passed points.
@descr Computes the angle from the vector passed in p1 and the vector passed in p2 using the
       atan2 function.
 --]]
 function point.angleTo(v1,v2)
  return math.atan2(v2.y-v1.y,v2.x-v1.x)
end

--[[
@luafunc static num point.distanceTo(point p1, point p2)
@brief Compute the distance between two points.
@descr Computes the distance between the two passed points.
 --]]
 function point.distanceTo(v1,v2)
  return math.sqrt((v2.x-v1.x)*(v2.x-v1.x)+(v2.y-v1.y)*(v2.y-v1.y))
end

--[[
@luafunc static num point.distSquared(point p1, point p2)
@brief Compute the distance squared between two points.
@descr Computes the distance squared between the two passed points.  This is useful if you
are comparing distances to one another, and don't need the actual distance.  Not computing
the sqrt will make your scripts run more efficiently.
--]]
function point.distSquared(v1,v2)
  return (v2.x-v1.x)*(v2.x-v1.x)+(v2.y-v1.y)*(v2.y-v1.y)
end

-- Some possibly useful constants
--[[
@luaconst point.zero
@brief Constant representing the point (0, 0).

Using this constant is marginally more efficient than defining it yourself.
--]]
point.zero = point.new(0,0)

--[[
@luaconst point.one
@brief Constant representing the point (1, 1).

Using this constant is marginally more efficient than defining it yourself.
--]]
point.one = point.new(1,1)


-- Metamethods for a 'point'

-- Pretty printing of a point
mt.__tostring = function(p) return "point ("..tostring(p.x)..","..tostring(p.y)..")"  end

-- Math operators
mt.__add = function(v1,v2) return point.new(v1.x+v2.x,v1.y+v2.y) end
mt.__sub = function(v1,v2) return point.new(v1.x-v2.x,v1.y-v2.y) end
mt.__mul = function(v1,v2)
  local s = tonumber(v2)
  if s then
    -- vector * scalar
    return point.new(v1.x*s,v1.y*s)
  else
    local s = tonumber(v1)
    if s then
      -- scalar * vector
      return point.new(v2.x*s,v2.y*s)
    else
      -- vector * vector
      return point.new(v1.x*v2.x,v1.y*v2.y)
    end
  end
end
mt.__div = function(v1,s) return point.new(v1.x/s,v1.y/s) end
mt.__unm = function(v) return point.new(-v.x,-v.y) end

-- Patch 'type' function
local t = type
type = function(o)
  -- Old type
  local ot = t(o)
  if ot == "table" and tmg(o) == mt then return "point" end
  return ot
end