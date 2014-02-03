-- Filter Selection
--
-- Reduce selection to objects of a certain type
--
-- This work is released into the public domain
-- Authored by kaen

local sd = require('stardust')

function getArgsMenu()

  -- Limit options to types of items currently in the level
  local selectedTypes = {}

  for _, obj in pairs(plugin:getAllObjects()) do
    for _, typeName in ipairs(sd.VALID_TYPES) do
      if ObjType[typeName] == obj:getObjType() then
        table.insert(selectedTypes, typeName)
      end
    end
  end
  
  -- Default to global scope, unless there are some selected objects
  scopeIndex = 1
  if #plugin:getSelectedObjects() > 0 then
    scopeIndex = 2
  end

  local options = sd.uniqueValues(selectedTypes)
  table.sort(options)

  local menu = { }
  if #options > 0 then
    menu[1] = ToggleMenuItem.new("Filter by Type:", options, 1, "The desired object type")
  end
  
  actions = {"Select/Keep", "Deselect"}
  menu[2] = ToggleMenuItem.new("Action:", actions, 1, "The action to perform")
  
  scope = {"All Objects", "Selection Only"}
  menu[3] = ToggleMenuItem.new("Scope:", scope, scopeIndex, "The object scope to alter")

  return "Filter Selection", "Keep or deselect the specified object type", "Ctrl+Shift+F", menu
end

function main()

  local objectType = table.remove(arg, 1)
  local action = table.remove(arg, 1)
  local scope = table.remove(arg, 1)
  
  -- Determine action
  local select = true
  if action == "Deselect" then 
    select = false 
  end
  
  -- Grab objects by scope
  local objects = {}
  local global = false
  if scope == "Selection Only" then
    objects = plugin:getSelectedObjects()
  elseif scope == "All Objects" then
    objects = plugin:getAllObjects()
    global = true
  end
  
  if #objects == 0 then
    plugin:showMessage('No known objects selected', false)
    return
  end
  
  -- Change selection of the objects
  for _, obj in pairs(objects) do
    if obj:getObjType() == ObjType[objectType] then
      obj:setSelected(select)
    -- Let's be smart and only set the inverse selection on objects
    -- we have selected
    elseif(not global) then
      obj:setSelected(not select)
    end
  end
end   

