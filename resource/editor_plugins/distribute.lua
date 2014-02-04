-- Distribute
-- This work is released into the public domain
-- Authored by kaen

-- A map of edge names to functions which retrieve that edge from the objects
-- extents

local sd = require('stardust')

function getArgsMenu()
	local choices = { }
	for k, _ in pairs(sd.EDGE) do
		table.insert(choices, k)
	end

	menu = 	{
		ToggleMenuItem.new("Distribute By", choices, 1, "The relative position of the objects to distribute by")
	}
	return "Distribute", "Evenly distribute objects", "Ctrl+Shift+'", menu
end

function main()
	local edge = table.remove(arg, 1)
	local objects = plugin:getSelectedObjects()

	sd.distribute(objects, sd.EDGE[edge])
end   

