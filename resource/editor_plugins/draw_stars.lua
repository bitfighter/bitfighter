-- Coded by Phill as part of the 2013 GCI

function getArgsMenu()
   return "Create Polygon/Star",                            -- Title shown on menu
          "Generate a polygon or star with n-sides/points", -- Description shown when hovered
          "Ctrl+G",                                         -- Shortcut to open the plugin
      {
         -- Toggle args:     Name        Choices                                                        Start Loop    Description
         ToggleMenuItem.new("Type",   {"Polygon", "Star"},                                                1,  true, "Type of shape"),
         ToggleMenuItem.new("Object", {"Wall", "Polywall", "Loadout Zone", "Goal Zone", "Nexus", "Zone"}, 1,  true, "Type of object"),

         -- Counter args:      Name           Value  Step    Min    Max   Units        Msg Description   
         CounterMenuItem.new("Points",          4,    1,      3,   1000, "points",     "", "Number of outside points"),
         CounterMenuItem.new("Outside Radius",  100,  1,      2,  10000, "grid units", "", "Radius of outside points"),
         CounterMenuItem.new("Inside Radius",   50,   1,      1,  10000, "grid units", "", "Radius of inside points for Stars (ignored for Polygons)"), 
         CounterMenuItem.new("Barrier Width",   50,   1,      1,    500, "grid units", "", "Wall width (only used for Wall objects)"),
         CounterMenuItem.new("Center X",        0,   10, -10000,  10000, "",           "", "X coordinate of center of shape"),
         CounterMenuItem.new("Center Y",        0,   10, -10000,  10000, "",           "", "Y coordinate of center of shape"),
         CounterMenuItem.new("Rotation",        0,    1,      0,    360, "degrees",    "", "Rotation offset")
      }
end

function main()
   -- arg table will include values from menu items above, in order
   local scriptName   = arg[0]
   local shape        = arg[1]
   local objectType   = arg[2]
   local points       = arg[3]
   local outerRadius  = arg[4]
   local innerRadius  = arg[5]
   local barrierWidth = arg[6]
   local centerX      = arg[7]
   local centerY      = arg[8]
   local offset       = arg[9]

   local object = nil

   if objectType == "Wall" then
      object = WallItem.new()
      object:setWidth(barrierWidth)
      points = points + 1     -- A wall needs to reconnect back to the beginning
   elseif objectType == "Polywall" then
      object = PolyWall.new()
   elseif objectType == "Loadout Zone" then
      object = LoadoutZone.new()
   elseif objectType == "Goal Zone" then
      object = GoalZone.new()
   elseif objectType == "Nexus" then
      object = NexusZone.new()
   elseif objectType == "Zone" then
      object = Zone.new()      
   end

   
   offset = (offset * math.pi) / 180      -- Convert to radians 
   
   local step = (2 * math.pi) / points    -- Each turn is these radians apart
   local geom = {}      -- Create a place to store the points we will create
   local angle          -- Declare variable outside loop for memory purposes

   for i = 1, points do
      -- Find the current target angle from the center
      angle = i * step + offset
      -- Insert the new point into the table with trig
      local pt = point.new(outerRadius * math.cos(angle) + centerX, outerRadius * math.sin(angle) - centerY)
      table.insert(geom, pt)

      if shape == "Star" then
         -- Insert to inside point if it's a star
         local pt = point.new(innerRadius * math.cos(angle + step/2) + centerX, innerRadius * math.sin(angle + step/2) - centerY)
         table.insert(geom, pt)
      end
   end
   
   object:setGeom(geom)
   bf:addItem(object)
end   