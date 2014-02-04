-- Polygon subdivision
-- Subdivide (a.k.a round) polygons using multi-pass fixed midpoint vertex
-- insertion and gaussian first-order vertex smoothing
-- This work is released into the public domain
--
-- Authored by kaen

local sd = require('stardust')

function getArgsMenu()

	menu = 	{
		CounterMenuItem.new("Size Threshold: ",  32, 1,       1,    0xFFFF, "", "", "Minimum length of segments before subdivision occurs"),
		CounterMenuItem.new("Smoothing: ",  0, 1,       0,    100, "%", "No Smoothing", "Amount of smoothing to perform"),
		YesNoMenuItem.new("Subdivide Completely: ",  2, "Divide until all segments are below size threshold")
	}

	return "Subdivide", "Subdivide and smooth polygons", "Ctrl+]", menu
end

function main()
	local maxDistance = table.remove(arg, 1) + 0
	local smoothing   = table.remove(arg, 1) / 100
	local completely  = table.remove(arg, 1)
	local gridSize    = plugin:getGridSize()
	local objects     = plugin:getSelectedObjects()

	for k, v in pairs(objects) do

		local geom = v:getGeom()

		if type(geom) == "table" then

			-- Add a virtual vertex to the end of implicitly closed types
			if sd.implicitlyClosed(v) then
				table.insert(geom, geom[1])
			end

			geom = sd.subdividePolyline(geom, maxDistance, smoothing, completely == "Yes")

			-- Remove duplicate vertexes from implicitly closed types
			while sd.implicitlyClosed(v) and point.distanceTo(geom[1], geom[#geom]) < 1 do
				table.remove(geom, #geom)
			end

			v:setGeom(geom)
		end
	end
end   

