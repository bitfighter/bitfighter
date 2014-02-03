-- Polygon simplification
-- Simplify (a.k.a smooth) polylines using the Ramer-Douglas-Peucker algorithm
-- This work is released into the public domain
-- Authored by kaen

local sd = require('stardust')

function getArgsMenu()

	menu = 	{
		TextEntryMenuItem.new("Epsilon: ", "2.0", "2.0", "Maximum variation to allow"),
	}

	return "Simplify II", "Simplify by removing variations greater than a threshold", "Ctrl+=", menu
end

function main()

	local epsilon = table.remove(arg, 1) + 0
	local objects = plugin:getSelectedObjects()

	for k, v in pairs(objects) do
		if type(v:getGeom()) == "table" then
			local geom = v:getGeom()
			local closed = false
			if geom[1] == geom[#geom] then
				table.remove(geom, #geom)
				closed = true
			end
			newGeom = sd.rdp_simplify(geom, epsilon)
			if closed then
				newGeom[#newGeom] = newGeom[1]
			end
			v:setGeom(newGeom)
		end
	end
end   

