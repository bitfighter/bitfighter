-- AutoBorder
-- Draw lines around polygons
-- This work is released into the public domain
--
-- Authored by kaen

local sd = require('stardust')

function getArgsMenu()
	return "Polywallify", "Convert Barriers to Polywalls (for real)", "Ctrl+Shift+P"
end

function main()

	local barriers = sd.keep(plugin:getSelectedObjects(), WallItem)
	if #barriers < 1 then
		plugin:showMessage('Please select at least one barrier', false)
		return
	end

	-- deselect old objects
	sd.each(plugin:getSelectedObjects(), function(x) x:setSelected(false) end)

	-- keep all selected WallItems (barriers), ignore the rest, and for each barrier:
	sd.each(barriers, function(barrier)

		if barrier == nil then
			return
		end

		-- get the barrier's skeleton
		local width = barrier:getWidth() / 2
		local geom = constructBarrierEndPoints(barrier:getGeom(), width)
		local segments = { }

		for i = 1,#geom-1,2 do
			table.insert(segments, extrudeSegment(geom[i], geom[i+1], width, i == 1, i == #geom-1))
		end

		results = Geom.clipPolygons(ClipType.Union, { }, segments)
		for k,geom in ipairs(results) do
			local polyWall = PolyWall.new(geom)
			bf:addItem(polyWall)
			polyWall:setSelected(true)
		end

		barrier:removeFromGame()

		end)
end

-- Gets the extrusion of the segment in the order { p1r, p2r, p2l, p1l }
function extrudeSegment(p1, p2, width, isFirst, isLast)

	-- a vector point along the segment with length one
	local unit = normalize(p2 - p1)

	-- a unit vector perpendicular to the segment (pointing right)
	local normal = point.new(-unit.y, unit.x)

	-- factors used to account for segments at the start and end of a barrier
	-- not getting padded past the geom's endpoint
	local headFactor, tailFactor = 1.0, 1.0

	if isFirst then
		tailFactor = 0
	end

	if isLast then
		headFactor = 0
	end

	return {
		p1 + width * normal,
		p2 + width * normal,
		p2 - width * normal,
		p1 - width * normal
	}

end

function normalize(p)
	local length = point.length(p)
	return point.new(p.x/length, p.y/length)
end

function constructBarrierEndPoints(points, width)
	local barrierEnds = { }

	if #points <= 1 then
		return
	end

	local loop = (points[1] == points[#points])      -- Does our barrier form a closed loop?

	local edgeVector = { }
	for i = 1, #points-1 do
		table.insert(edgeVector, normalize(points[i+1] - points[i]))
	end

	local lastEdge = edgeVector[#edgeVector];
	local extend = { }
	for i=1,#edgeVector do
		local curEdge = edgeVector[i]
		local cosTheta = point.dot(curEdge, lastEdge)

		if cosTheta > 1.0 then
			cosTheta = 1.0
		elseif cosTheta < -1.0 then
			cosTheta = -1.0
		end
		cosTheta = math.abs(cosTheta)

		local extendAmt = width * 0.5 * math.tan(math.acos(cosTheta) / 2 )
		if(extendAmt > 0.01) then
			extendAmt = extendAmt - 0.01
		end
		table.insert(extend, extendAmt)

		lastEdge = curEdge;
	end

	local first = extend[0]
	table.insert(extend, first)

	for i=1,#edgeVector do
		local extendBack = extend[i]
		local extendForward = extend[i+1]
		if(i == 1 and not loop) then
			extendBack = 0;
		elseif(i == #edgeVector and not loop) then
			extendForward = 0
		end

		local start = points[i]   - edgeVector[i] * extendBack;
		local stop  = points[i+1] + edgeVector[i] * extendForward;
		table.insert(barrierEnds, start);
		table.insert(barrierEnds, stop);
	end

	return barrierEnds
end
