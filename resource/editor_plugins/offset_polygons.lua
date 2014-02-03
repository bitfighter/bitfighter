-- Offset Polygons
-- Offsets selected polygonal objects by a given amount, either 
-- positively (grow) or negatively (shrink)
--
-- This work is released into the public domain
-- Authored by raptor

local sd = require('stardust')


function getArgsMenu()
	menu = 	{
		CounterMenuItem.new("Offset Amount",  0, 1, -0xFFFF, 0xFFFF, "grid units", "", "The amount to offset"),
		YesNoMenuItem.new("Process Together", 2, "Merging may result. Only works on a single type")
	}

	return "Offset Polygons", "Shrink or grow polygon object types", "Ctrl+Shift+O", menu
end


function main()
	local offsetAmount = table.remove(arg, 1)
	local processTogether  = table.remove(arg, 1)

	local objects = plugin:getSelectedObjects()
	
	
	if #objects == 0 then
		plugin:showMessage("Operation failed.  No objects are selected", false)
		return
	end
	
	
	-- Analyze if all are a single type
	for _, obj in pairs(objects) do
		if not sd.implicitlyClosed(obj) then
			plugin:showMessage("Operation failed.  Non-polygon objects are selected", false)
			return
		end
	end
	
	
	-- Process all polygons together as a set
	if processTogether == "Yes" then
		local typeId = nil
		
		-- Analyze if all are a single type
		for _, obj in pairs(objects) do
			if typeId == nil then
				typeId = obj:getObjType()
			end
			
			if typeId ~= obj:getObjType() then
				plugin:showMessage("Operation failed.  Objects are not the same type", false)
				return
			end
		end
		
		local polySet = {}
		-- Now build a list of polygons
		for _, obj in pairs(objects) do
			table.insert(polySet, obj:getGeom())
		end
		
		-- Offset!
		local result = Geom.offsetPolygons(offsetAmount, polySet)
		
		if result == nil then
			plugin:showMessage("Operation failed.  No output polygons", false)
			return
		end
		
		-- Delete old polys
		for _, obj in pairs(objects) do
			obj:setSelected(false)
			obj:removeFromGame()
		end
		
		-- Add new polys
		for _, poly in pairs(result) do
			local newObj = sd.OBJTYPE_TO_CLASS[typeId].new()
			
			newObj:setGeom(poly)
			newObj:setSelected(true)
			bf:addItem(newObj)
		end
		
	-- Process each polygon individually
	else
		-- First build a list of polygons
		for _, obj in pairs(objects) do
			local typeId = obj:getObjType()
			local geom = obj:getGeom()
			local team = obj:getTeamIndex()
			
			local result = Geom.offsetPolygons(offsetAmount, {geom})
			
			if result ~= nil then
				-- Delete old polys
				obj:setSelected(false)
				obj:removeFromGame()
				
				-- Add new poly(s)
				for _, poly in pairs(result) do
					local newObj = sd.OBJTYPE_TO_CLASS[typeId].new()
					
					newObj:setGeom(poly)
					newObj:setTeam(team)
					newObj:setSelected(true)
					bf:addItem(newObj)
				end
			end
		end
	end
end   

