----------------------
--- item_select.lua
---
--- Author:  raptor
----------------------


items = {
	"Asteroid",
	"AsteroidSpawn",
	"Barrier",
	"Core",
	"Flag",
	"FlagSpawn",
	"ForceFieldProjector",
	"GoalZone",
	"LineItem",
	"LoadoutZone",
	"Mine",
	"Nexus",
	"PolyWall",
	"RepairItem",
	"ResourceItem",
	"ShipSpawn",
	"SoccerBallItem",
	"SpeedZone",
	"SpyBug",
	"Teleporter",
	"TestItem",
	"TextItem",
	"Turret",
	"Zone",
}

--
-- This builds the menu
--
function getArgsMenu()
	menuTable = {
		ToggleMenuItem.new("Action", { "Select", "Deselect" }, 1, true, "Action to perform"),
		ToggleMenuItem.new("Object", items, 1, true, "Object type"),
	}

	return "Select/deselect all items of the specified type", menuTable
end


--
-- This is run when you choose 'Run Plugin' from the plugin menu
--
function main()
	local action = arg[1]
	local object = arg[2]
	
	local select = true
	if action == "Deselect" then 
		select = false 
	end
	
	-- Handle special cases
	if object == "Barrier" then
		object = "WallItem"
	end
	
	-- Convert our object into the appropriate object type
	local objectType = ObjType[object]
  
	-- Grab all objects
	local allObjects = plugin:getAllObjects()
	
	-- Iterate through them and only adjust those of the specified type
	for k, currentObject in pairs(allObjects) do
		if currentObject:getClassId() == objectType then
			currentObject:setSelected(select)
		end
	end
end   
