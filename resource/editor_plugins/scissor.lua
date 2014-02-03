-- Simple Scissors
-- Simple polygon difference
-- This work is released into the public domain
--
-- Authored by kaen

local sd = require('stardust')
local objects

function hasPolyGeom(object)
	return type(object.getGeom) == "function" and type(object:getGeom()) == "table"
end

function getArgsMenu()

	local menu = nil
	objects = sd.filter(plugin:getSelectedObjects(), hasPolyGeom)

	-- Return nil for menu arguments if the selection is invalid, causing main
	-- to execute immediately when activated and fail with a useful error
	-- message. Saves the user from filling out the dialog when his input is
	-- already provably invalid.
	if #objects > 2 then

		menu = 	{
			ToggleMenuItem.new("Subtract: ", { "First From Others", "Others From First" }, 1, false, "(B ... Z) - A or A - (B ... Z)"),
			YesNoMenuItem.new("Delete Subtracted: ", 1, false, "Delete B in A - B"),
		}

	end

	return "Scissors", "Subtract polygons", "Ctrl+Shift+-", menu
end

function main()

	local autoMode         = #objects == 2
	local firstIsSubject   = autoMode or (table.remove(arg, 1) == "Others From First")
	local deleteSubtracted = autoMode or (table.remove(arg, 1) == "Yes")
	local firstGeom        = nil
	local firstObject      = nil


	-- Make sure we have valid inputs
	if #objects < 2 then

		plugin:showMessage("You must select at least two polygon objects", false)
		return
	end

	-- Find first valid polygonal object
	firstObject = table.remove(objects, 1)
	firstGeom = firstObject:getGeom()

	if firstIsSubject then

		-- Get other polys
		local otherGeoms = sd.map(sd.copy(objects), function(x) return x:getGeom() end)

		-- Perform the operation
		local result = Geom.clipPolygons(ClipType.Difference, { firstGeom }, otherGeoms, true)

		-- Only proceed if the operation succeeded
		if type(result) == "table" then

			-- Create the output objects
			for _, poly in ipairs(result) do
				local new = sd.clone(firstObject)
				new:setGeom(poly)
				bf:addItem(new)
			end
		end

		if deleteSubtracted then
			for i,object in ipairs(objects) do
				object:removeFromGame()
			end
		end

		firstObject:removeFromGame()
	else

		-- Process the other objects
		for _, object in ipairs(objects) do
			local result = Geom.clipPolygons(ClipType.Difference, { object:getGeom() }, { firstGeom }, true)
			for _, poly in ipairs(result) do
				local new = sd.clone(object)
				new:setGeom(poly)
				bf:addItem(new)
			end

			object:removeFromGame()
		end

		if deleteSubtracted then
			firstObject:removeFromGame()
		end
	end
end

