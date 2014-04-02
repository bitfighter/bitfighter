require('geometry')

sd = { }

local VALID_TYPES = {
  "Asteroid",
  "AsteroidSpawn",
  "Core",
  "EnergyItem",
  "Flag",
  "FlagSpawn",
  "ForceFieldProjector",
  "GoalZone",
  "Line",
  "LoadoutZone",
  "Mine",
  "Nexus",
  "PolyWall",
  "RepairItem",
  "ResourceItem",
  "SoccerBallItem",
  "SlipZone",
  "ShipSpawn",
  "SpeedZone",
  "SpyBug",
  "Teleporter",
  "TestItem",
  "TextItem",
  "Turret",
  "WallItem",
  "Zone"
}

local function complain(str)
	print(str)
end

local IMPLICITLY_CLOSED_CLASS_IDS = {
	[ObjType.GoalZone] = true,
	[ObjType.LoadoutZone] = true,
	[ObjType.Nexus] = true,
	[ObjType.PolyWall] = true,
	[ObjType.SlipZone] = true,
	[ObjType.Zone] = true
}

local ZONE_CLASS_IDS = {
	[ObjType.GoalZone] = true,
	[ObjType.LoadoutZone] = true,
	[ObjType.Nexus] = true,
	[ObjType.SlipZone] = true,
	[ObjType.Zone] = true
}

local OBJTYPE_TO_CLASS = {
	-- [ObjType.Barrier]             = Barrier,
	[ObjType.Ship]                = Ship,
	[ObjType.Line]                = LineItem,
	[ObjType.ResourceItem]        = ResourceItem,
	[ObjType.TextItem]            = TextItem,
	[ObjType.LoadoutZone]         = LoadoutZone,
	[ObjType.TestItem]            = TestItem,
	[ObjType.Flag]                = FlagItem,
	-- [ObjType.Bullet]              = Bullet,
	[ObjType.Burst]               = Burst,
	[ObjType.Mine]                = Mine,
	[ObjType.Nexus]               = NexusZone,
	[ObjType.Robot]               = Robot,
	[ObjType.Teleporter]          = Teleporter,
	[ObjType.GoalZone]            = GoalZone,
	[ObjType.Asteroid]            = Asteroid,
	[ObjType.RepairItem]          = RepairItem,
	[ObjType.EnergyItem]          = EnergyItem,
	[ObjType.SoccerBallItem]      = SoccerBallItem,
	[ObjType.Turret]              = Turret,
	-- [ObjType.ForceField]          = ForceField,
	[ObjType.ForceFieldProjector] = ForceFieldProjector,
	[ObjType.SpeedZone]           = SpeedZone,
	[ObjType.PolyWall]            = PolyWall,
	[ObjType.ShipSpawn]           = Spawn,
	[ObjType.FlagSpawn]           = FlagSpawn,
	[ObjType.AsteroidSpawn]       = AsteroidSpawn,
	[ObjType.WallItem]            = WallItem,
	[ObjType.SlipZone]            = SlipZone,
	[ObjType.SpyBug]              = SpyBug,
	[ObjType.Core]                = CoreItem,
	[ObjType.Zone]                = Zone,
	[ObjType.Seeker]              = Seeker 
}

-- Return a shallow copy of t
local function copy(t)
	local u = { }
	for k,v in pairs(t) do
		u[k] = v
	end
	return u
end

-- Reverse the order of t's ipairs in place
local function reverse(t)
	for i=1,#t/2 do
		local temp = t[i]
		t[i] = t[#t - i + 1]
		t[#t - i + 1] = temp
	end

	return t
end

-- Swap the keys and values of all pairs in t
local function invert(t)
	local u = copy(t)
	for k,v in pairs(u) do
		t[k] = nil
		t[v] = k
	end

	return t
end

-- Apply f to each value in t
local function each(t, f)
	for k,v in pairs(t) do
		f(v)
	end
end

-- Change all values `v` in t to the result of f(v)
local function map(t, f)
	for k,v in pairs(t) do
		t[k] = f(v)
	end
	return t
end

-- Save some work here
local CLASS_TO_OBJTYPE = invert(copy(OBJTYPE_TO_CLASS))

-- Returns true if all of the given objects are of one of the specified types
local function is(...)
	local objTypes = { }
	local objects = { }

	for _, v in pairs(arg) do
		if type(v) == "userdata" then
			table.insert(objects, v)
		elseif type(v) == "number" then
			table.insert(objTypes, v)
		elseif CLASS_TO_OBJTYPE[v] then
			table.insert(objTypes, CLASS_TO_OBJTYPE[v])
		end
	end

	if #objects == 0 then
		return false
	end

	for j,object in ipairs(objects) do
		local objectResult = false

		for i,objtype in ipairs(objTypes) do
			if objtype == object:getObjType() then
				objectResult = true
				break
			end
		end

		if objectResult == false then
			return false
		end
	end

	return true
end

-- adds every element of t2 on to t1:
-- 
-- append({1, 2}, {3, 4})
-- > {1, 2, 3, 4}
local function append(t1, t2)
	for _, v in pairs(t2) do
		table.insert(t1, v)
	end
end

local function prepend(t1, t2)
	for i = #t2,1,-1 do
		table.insert(t1, t2[i], 1)
	end
end

-- removes all t[k] where predicate(t[k]) == false
local function filter(t, predicate)
	local j = 1

	for i = 1,#t do
		local v = t[i]
		if predicate(v) then
			t[j] = v
			j = j + 1
		end
	end

	while t[j] ~= nil do
		t[j] = nil
		j = j + 1
	end

	return t
end

-- Keep all objects in `t` which are of one of the Classes or ObjTypes given
local function keep(t, ...)
	filter(t, function(x) return is(x, unpack(arg)) end)
	return t
end

-- return the extents of the given object
local function extents(object)
	local minx = math.huge
	local miny = math.huge
	local maxx = -math.huge
	local maxy = -math.huge
	local geom = object:getGeom()

	if type(geom) == "table" then
		for i, p in ipairs(geom) do
			minx = math.min(minx, p.x)
			miny = math.min(miny, p.y)
			maxx = math.max(maxx, p.x)
			maxy = math.max(maxy, p.y)
		end
	elseif type(geom) == "point" then
	    local r = 0

	    -- Try to get the radius, but don't worry if you can't
	    pcall(function()
	      r = object:getRad()
	    end)

		minx = geom.x - r
		maxx = geom.x + r
		miny = geom.y - r
		maxy = geom.y + r
	end

	return { minx = minx, miny = miny, maxx = maxx, maxy = maxy }
end

local function halfSize(object)
	local geom = object:getGeom()
	local result = point.new(0, 0)
	if type(geom) == "table" then
		local ext = extents(object)
		result = point.new((ext.maxx - ext.minx) / 2, (ext.maxy - ext.miny) / 2)
	elseif type(geom) == "point" then
		local r
		-- check if the object has getRad
		if object.getRad then
			r = object:getRad()
		else
			r = 0
		end
		result = point.new(r, r)
	end
	return result
end

local function midPoint(p1, p2)
	return point.new((p1.x + p2.x) / 2, (p1.y + p2.y) / 2)
end

-- returns a point equal to the average of the supplied points
-- accepts any number of arguments as long as they are points
-- or first-order tables of points
local function average(...)
	local points = {}

	for _, v in pairs(...) do
		if type(v) == 'point' then
			table.insert(points, v)
		elseif type(v) == 'table' then
			append(points, v)
		end
	end

	local sum = point.new(0,0)
	for i, v in ipairs(points) do
		sum = sum + v
	end
	return sum / #points
end

-- returns true if the object has polygon geometry i.e. it is
-- always rendered as a closed polygon (Zones and polywalls)
local function implicitlyClosed(obj)
	return not not IMPLICITLY_CLOSED_CLASS_IDS[obj:getObjType()]
end

-- returns `true` if arg is an object which is a zone, or an ObjType
-- corresponding to a zone type. `false` otherwise.
local function isZone(arg)
	local result = false

	if type(arg) == "number" then
		result = not not ZONE_CLASS_IDS[arg]
	elseif type(arg) == "userdata" and type(arg.getObjType) == "function" then
		result = not not ZONE_CLASS_IDS[arg:getObjType()]
	end

	return result
end

-- get the object's center point
local function center(obj)
	local ext = extents(obj)
	return point.new(ext.minx + (ext.maxx - ext.minx) / 2, ext.miny + (ext.maxy - ext.miny) / 2)
end

-- set the object's center point
local function centerOn(object, pos)
	local geom = object:getGeom()
	if type(geom) == "table" then
		local half = halfSize(object)
		local translation = pos - center(object)

		local newGeom = { }
		for i, p in ipairs(geom) do
			table.insert(newGeom, p + translation)
		end
		object:setGeom(newGeom)
	else
		object:setPos(pos)
	end
end

local function mergeExtents(objects)
	local minx = math.huge
	local miny = math.huge
	local maxx = -math.huge
	local maxy = -math.huge

	for k, obj in ipairs(objects) do
		local ext = extents(obj)
		minx = math.min(minx, ext.minx)
		miny = math.min(miny, ext.miny)
		maxx = math.max(maxx, ext.maxx)
		maxy = math.max(maxy, ext.maxy)
	end

	return { minx = minx, miny = miny, maxx = maxx, maxy = maxy }
end

local function align(objects, alignment)
	local ext = mergeExtents(objects)
	for k, obj in pairs(objects) do
		local c = center(obj)
		local h = halfSize(obj)
		if alignment == "Left" then
			centerOn(obj, point.new(ext.minx + h.x, c.y))
		elseif alignment == "Right" then
			centerOn(obj, point.new(ext.maxx - h.x, c.y))
		elseif alignment == "Center" then
			centerOn(obj, point.new(ext.minx + (ext.maxx - ext.minx) / 2, c.y))
		elseif alignment == "Top" then
			centerOn(obj, point.new(c.x, ext.miny + h.y))
		elseif alignment == "Bottom" then
			centerOn(obj, point.new(c.x, ext.maxy - h.y))
		elseif alignment == "Middle" then
			centerOn(obj, point.new(c.x, ext.miny + (ext.maxy - ext.miny) / 2))
		else
			error('Unknown alignment ' .. alignment)
		end
	end
end

-- returns point evaluating a cubic bezier at time t
local function evaluateCubicBezier(points, t, power)
  local meana = ((points[2] - points[1]) + (points[3] - points[2])) / 2
  local meanb = ((points[4] - points[3]) + (points[3] - points[2])) / 2

  local pa = points[2]
  local pb = points[2] + point.normalize(meana) * point.distanceTo(points[2], points[3]) * power
  local pc = points[3] - point.normalize(meanb) * point.distanceTo(points[2], points[3]) * power
  local pd = points[3]

  local a = 1 - t
  local b = t
  local x = pa.x*a*a*a + 3*pb.x*a*a*b + 3*pc.x*a*b*b + pd.x*b*b*b
  local y = pa.y*a*a*a + 3*pb.y*a*a*b + 3*pc.y*a*b*b + pd.y*b*b*b

  return point.new(x, y)
end

-- return point i from poly, handling bounds crossing appropriately depending
-- on whether the polygon is closed or not
local function getPoint(poly, i)
  local result
  local closed = false

  if poly[1] == poly[#poly] then
    closed = true
  end

  if i < 1 then
    if closed then
      result = poly[#poly + i - 1]
    else
      result = poly[#poly + i]
    end
  elseif i > #poly then
    if closed then
      result = poly[i - #poly + 1]
    else
      result = poly[i - #poly]
    end
  else
    result = poly[i]
  end
  return result
end

local function getPoints(poly, start, n)
  local points = { }
  for i = start, start+n do
    table.insert(points, getPoint(poly, i))
  end
  return points
end

local function lengthOf(poly)
  local result = 0
  for k, v in ipairs(poly) do
    if k > 1 then
      result = result + point.length(poly[k] - poly[k-1])
    end
  end
  return result
end

-- Find the segment of `poly` at distance `d` along the polyline
--
-- Traverses `poly` until it gets to the segment which contains the 
-- point at distance `d` along the line, then returns the one-based
-- index of the segment. `d` is a number between 0.0 and 1.0 inclusive
--
-- returns segment, segmentStart, segmentEnd
-- where segment is the index, and segmentStart/End are the distance along
-- the line where this segment starts and ends
local function segmentAt(poly, d)
	local totalLength = lengthOf(poly)
    local segment, segmentStart, segmentEnd
    local traversedLength = 0
    for k = 2, #poly do
      segmentStart = traversedLength / totalLength
      traversedLength = traversedLength + point.length(poly[k] - poly[k - 1])
      segmentEnd = traversedLength / totalLength
      if (traversedLength / totalLength) >= d then
        segment = k - 1
        break
      end
    end

    return segment, segmentStart, segmentEnd
end

local EDGE = {
	LEFT = 0,
	CENTER = 1,
	RIGHT = 2,
	BOTTOM = 3,
	MIDDLE = 4,
	TOP = 5
}

-- Find the position of obj's given edge
local function edgePos(obj, edge)
  local ext = extents(obj)

  if     edge == EDGE.LEFT   then return ext.minx
  elseif edge == EDGE.CENTER then return (ext.minx + ext.maxx) / 2
  elseif edge == EDGE.RIGHT  then return ext.maxx
  elseif edge == EDGE.BOTTOM then return ext.miny
  elseif edge == EDGE.MIDDLE then return (ext.miny + ext.maxy) / 2
  elseif edge == EDGE.TOP    then return ext.maxy
  else
  	complain('Unknown edge ' .. edge)
  end
end

-- Returns the axis ('x' or 'y') which this edge is measured with
local function axisOf(edge)
  if     edge == EDGE.LEFT   then
  	return "x"
  elseif edge == EDGE.CENTER then
  	return "x"
  elseif edge == EDGE.RIGHT  then
  	return "x"
  elseif edge == EDGE.BOTTOM then
  	return "y"
  elseif edge == EDGE.MIDDLE then
  	return "y"
  elseif edge == EDGE.TOP    then
  	return "y"
  else
  	error('Unknown edge ' .. edge)
  end
end


-- Sorts a structure such as:
--   {
--		{ foo = 3, bip = 0 }
--		{ foo = 1, bar = 2 },
--	 }
-- ordered by the specified property (such as 'foo')
local function sortTableListByProperty(list, property)
	local result = { list[1] }
	for item, t in ipairs(list) do
		if item ~= 1 then
			local inserted = false
			for i, u in ipairs(result) do
				if u[property] > t[property] then
					table.insert(result, i, t)
					inserted = true
					break
				end
			end
			if not inserted then
				table.insert(result, t)
			end
		end
	end
	return result
end

-- returns the minimum and maximum position of the given edge for all objects
local function edgeExtents(objects, edge)
	local min = math.huge
	local max = -math.huge

	for k, obj in ipairs(objects) do
		local pos = edgePos(obj, edge)
		min = math.min(min, pos)
		max = math.max(max, pos)
	end

	return { min = min, max = max }
end

-- Move `obj` so that the given `edge` lies at `pos`
local function alignTo(obj, pos, edge)
	local axis = axisOf(edge)
	local offset = pos - edgePos(obj, edge)
	local geom = obj:getGeom()

	if axis == 'x' then
		geom = Geom.translate(geom, offset, 0)
	elseif axis == 'y' then
		geom = Geom.translate(geom, 0, offset)
	else
		error('No axis for edge ' .. edge)
	end

	obj:setGeom(geom)
end

-- distribute objects using the specified edge
local function distribute(objects, edge)
	local ext = edgeExtents(objects, edge)
	local step = (ext.max - ext.min) / (#objects - 1)

	local midpointsTable = { }
	for _, obj in ipairs(objects) do
		local mid = center(obj)
		table.insert(midpointsTable, { x = mid.x, y = mid.y, obj = obj })
	end

	local axis = axisOf(edge)
	local sortedTable = sortTableListByProperty(midpointsTable, axis)

	for i = 2, #sortedTable - 1 do
		local obj = sortedTable[i].obj
		local pos = ext.min + (i - 1) * step
		alignTo(obj, pos, edge)
	end
end

-- get a point representing the object's x and y size
local function size(obj)
  local ext = extents(obj)
  return point.new(ext.maxx - ext.minx, ext.maxy - ext.miny)
end

-- return an ordered table of unique values in t
local function uniqueValues(t)
  local values = { }
  local result = { }

  for k, v in pairs(t) do
    values[v] = true
  end

  for k, _ in pairs(values) do
    table.insert(result, k)
  end

  return result
end

-- returns the average slope for vertex i
local function findSlope(poly, i)
	local points = getPoints(poly, i - 1, 3)
	return ((points[2] - points[1]) + (points[3] - points[2])) / 2
end

local function slice(values,i1,i2)
	local res = {}
	local n = #values
	-- default values for range
	i1 = i1 or 1
	i2 = i2 or n
	if i2 < 0 then
		i2 = n + i2 + 1
	elseif i2 > n then
		i2 = n
	end
	if i1 < 1 or i1 > n then
		return {}
	end
	local k = 1
	for i = i1,i2 do
		res[k] = values[i]
		k = k + 1
	end
	return res
end

-- returns the minimum distance from the line containing the first two elements
-- of `line` to the point `p`
local function minimumDistance(line, p)
	local a, n
	-- beginning of the line segment
	a = line[1]
	-- unit vector from 1 to 2
	n = point.normalize(line[2] - line[1])

	return point.length((a - p) - point.dot(a - p, n) * n)
end

-- return the indices of the polygon representing the RDP simplification of
-- `poly` using `epsilon`
local function rdp_simplify(poly, epsilon)
	if #poly == 2 then
		return poly
	end

	local worstDistance = 0
	local worstIndex
	for i = 2,#poly-1 do
		local d = minimumDistance({poly[1], poly[#poly]}, poly[i])
		if d > worstDistance then
			worstDistance = d
			worstIndex = i
		end
	end

	if worstDistance < epsilon then
		return {poly[1], poly[#poly]}
	end

	local first, second
	first = rdp_simplify(slice(poly, 1, worstIndex), epsilon)
	second = rdp_simplify(slice(poly, worstIndex, #poly), epsilon)
	local result = { }
	for i = 1, #first do
		table.insert(result, first[i])
	end
	for i = 2, #second do
		table.insert(result, second[i])
	end
	return result
end

local function simplify(poly)
	if not poly then
		return
	end

	-- true if the polygon's start and end are equal
	local inputClosed = false
	if poly[1] == poly[#poly] then
		inputClosed = true
	end

	local newPoly = { }

	if not inputClosed then
		table.insert(newPoly, poly[1])
	end

	for i = 1, #poly - 1 do
		local points = getPoints(poly, i, 2)
		table.insert(newPoly, midPoint(points[1], points[2]))
	end

	if not inputClosed then
		table.insert(newPoly, poly[#poly])
	end

	-- if the input poly was closed, then close the new poly
	if inputClosed then
		table.insert(newPoly, midPoint(poly[1], poly[2]))
	end

	return newPoly
end

-- Returns a polyline which is a subdivision of the edges of the given poly
-- using the following algorithm:
-- 	- For each segment of the poly line:
--		- Output a vertex equal to the weighted average of the first
--		  vertex of this segment with the two adjacent midpoints
--		  controlled by `smoothing`
--		- If this segment's old length was greater than maxDistance:
--			- Output a vertex at the old midpoint of the segment
--	- If do_completely is true, and any subdivisions occured this pass:
--		- Set the output geometry as input geometry and repeat
local function subdividePolyline(poly, maxDistance, smoothing, do_completely)
	if not poly then
		return
	end

	-- true if the polygon's start and end are equal
	local inputClosed = false
	if poly[1] == poly[#poly] then
		inputClosed = true
	end

	local done = false
	local newPoly
	while not done do
		newPoly = { }
		done = true
		for i = 1, #poly do
			-- output the weighted average of the current point and
			-- the two adjacent midpoints
			local points = getPoints(poly, i - 1, 3)
			if not (inputClosed and i == 0) then
				local smoothedPoint = sd.average({sd.midPoint(points[1],points[2]),points[2],sd.midPoint(points[2],points[3])}) * smoothing + points[2] * (1 - smoothing)
				table.insert(newPoly, smoothedPoint)
			end

			-- split this segment if needed
			if (i ~= #poly) and point.distanceTo(points[2], points[3]) > maxDistance then
				table.insert(newPoly, sd.midPoint(points[2], points[3]))
				if do_completely then
					done = false
				end
			end
		end
		poly = newPoly
	end

	return newPoly
end

--[[
@func table spread(points, power)
@brief
Spread a table of points away from each other.

@desc
Spreads the given points using either inverse exponential displacement (that is,
`D = k / d^2` where D is the displacement vector applied to p1, k is `power`,
and d is the distance between points p1 and p2) or a user-supplied function. The
user-supplied function may be either a callable function or a string to be
converted into a callable function using makeEquation.

Each point is tested against each point other than itself. The two points are
fed into the power function, (as arguments `p1` and `p2`) and the return value
of the function is added to the point's displacement vector. The sumed
displacement vectors received by each point are then added to the position of
that point to find its new position.

@param table A table of points, which is modified in place.
@param power Either a number to use as k in `D = k / d^2`, a function which
	takes two arguments and returns a displacement vector, or a string to be
	converted via makeEquation with p1 and p2 set in its context.

@return The result of the spreading, which is the same table supplied as
    `points` with its values modified
]]
local function spread(points, power)
	local fn
	if type(power) == 'number' then
		fn = function(p1, p2) return point.normalize(p1 - p2) * power / point.distSquared(p1, p2) end
	elseif type(power) == 'string' then
		fn = makeEquation(power)
	elseif type(power) == 'function' then
		fn = power
	else
		complain('Expected number, string, or function for "power"')
		return
	end

	local forces = { }
	for i=1,#points do
		local force = point.zero
		local p1 = points[i]
		for j=1,#points do
			if i ~= j then
				local p2 = points[j]
				force = force + fn(p1, p2)
			end
		end
		forces[i] = force
	end

	for i=1,#points do
		points[i] = points[i] + forces[i]
	end

	return points
end

--[[
Returns a string of the form "1 thing" or "2 things"
@param n The number of things
@param singular The singular version of the word
@param [plural] An explicit puralized version of the word
@param [includeCount] Set to false to disable adding the count before the word
]]
local function plural(n, singular, plural, includeCount)
	local result = ""
	if includeCount == nil or includeCount then
		result = n .. " "
	end

	if n == 1 then
		result = result .. singular
	elseif type(plural) == "string" then
		result = result .. plural
	else
		result = result .. singular .. "s"
	end
	return result
end

--[[
@func table polarClamp(points, center, radius)

@brief
Clamp `points` to within `radius` units of `center`.

@desc
Clamp `points` to within `radius` units of `center`, modifying `points` in
place. Points further than `radius` units away from `center` will be moved
directly towards `center` until they are exactly `radius` away.

@param points The points to clamp.
@param center The center of circle to clamp within.
@param radius The distance from center to clamp all the points within.

]]
local function polarClamp(points, center, radius)
	local rSquared = radius^2
	for i,p in ipairs(points) do
		local distSquared = point.distSquared(p, center)
		if distSquared > rSquared then
			points[i] = point.normalize(p - center) * radius
		end
	end

	return points
end

--[[
@func table sortOn(t, k)
@brief
Sort an array of tables by a shared property `k`.

@desc
Sorts a structure such as:
@code
  {
	{ foo = 321, bam = 432 },
	{ foo = 123, bar = 234 }
  }
@endcode

By a specified property such as `foo`.

@param t The array of tables to sort.
@param k The value to use in the expression t[k] when sorting.

@return The sorted array of tables.
]]
local function sortOn(t, k)
	table.sort(t, function(a, b) return a[k] < b[k] end)
	return t
end

--[[
@func table makeCircle(center, r, div)

@brief
Make a circle at `center` with radius `r` using `div` number of divisions. More
divisions mean a more round-looking circle.

@param center The center of the circle.
@param r The radius of the circle.
@param [div] The number of subdivisions to use. Defaults to 32.

@return Table of points suitable for use as a polygon geometry.
]]
local function makeCircle(center, r, div)
	div = div or 32
	local result = { }
	for theta = 0, math.tau, math.tau/div do
		table.insert(result, point.new(math.sin(theta) * r, math.cos(theta) * r))
	end
	return result
end


--[[
@brief Return a copy of the given object including editor properties
]]
local function clone(obj)
	local result = OBJTYPE_TO_CLASS[obj:getObjType()].new()
	pcall(function() result:setGeom(obj:getGeom()) end)
	pcall(function() result:setWidth(obj:getWidth()) end)
	pcall(function() result:setTeam(obj:getTeamIndex()) end)
	pcall(function() result:setText(obj:getText()) end)
	pcall(function() result:setRegenTime(obj:getRegenTime()) end)
	pcall(function() result:setHealRate(obj:getHealRate()) end)
	pcall(function() result:setSpeed(obj:getSpeed()) end)
	pcall(function() result:setSlipFactor(obj:getSlipFactor()) end)
	pcall(function() result:setSnapping(obj:getSnapping()) end)
	return result
end


local function hasPolyGeom(object)
	return type(object.getGeom) == "function" and type(object:getGeom()) == "table"
end

sd = {
	align                       = align,
	alignTo                     = alignTo,
	append                      = append,
	average                     = average,
	axisOf                      = axisOf,
	center                      = center,
	centerOn                    = centerOn,
	clone                       = clone,
	complain                    = complain,
	copy                        = copy,
	distribute                  = distribute,
	each                        = each,
	edgeExtents                 = edgeExtents,
	edgePos                     = edgePos,
	evaluateCubicBezier         = evaluateCubicBezier,
	extents                     = extents,
	filter                      = filter,
	findSlope                   = findSlope,
	getPoint                    = getPoint,
	getPoints                   = getPoints,
	halfSize                    = halfSize,
	hasPolyGeom                 = hasPolyGeom,
	implicitlyClosed            = implicitlyClosed,
	invert                      = invert,
	is                          = is,
	isZone                      = isZone,
	keep                        = keep,
	lengthOf                    = lengthOf,
	makeCircle                  = makeCircle,
	map                         = map,
	mergeExtents                = mergeExtents,
	midPoint                    = midPoint,
	minimumDistance             = minimumDistance,
	prepend                     = prepend,
	plural                      = plural,
	polarClamp                  = polarClamp,
	rdp_simplify                = rdp_simplify,
	reverse                     = reverse,
	segmentAt                   = segmentAt,
	simplify                    = simplify,
	size                        = size,
	slice                       = slice,
	sortOn                      = sortOn,
	sortTableListByProperty     = sortTableListByProperty,
	spread                      = spread,
	subdividePolyline           = subdividePolyline,
	uniqueValues                = uniqueValues,
	
	CLASS_TO_OBJTYPE            = CLASS_TO_OBJTYPE,
	EDGE                        = EDGE,
	IMPLICITLY_CLOSED_CLASS_IDS = IMPLICITLY_CLOSED_CLASS_IDS,
	OBJTYPE_TO_CLASS            = OBJTYPE_TO_CLASS,
	VALID_TYPES                 = VALID_TYPES,
	ZONE_CLASS_IDS              = ZONE_CLASS_IDS,
}

return sd
