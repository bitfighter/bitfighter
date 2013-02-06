-- Sample Bitfighter editor plugin script
-- Create rudimentary curved walls or zones
-- Based on work by _k, adapted by raptor and watusimoto

--
-- If this function exists, it should return a list of menu items that can be used to build an options menu
-- If this function is not implemented, or returns nil, the menu will not be displayed, and main() will be
-- run without args
--
-- For a full list of the menu widgets available, see the Bitfighter wiki on bitfighter.org
--
function getArgsMenu()
   
   return "Create Arc",         -- Title shown on menu
      {
         CounterMenuItem.new("Angle",          90, 1,       0,   360, "deg.",       "", "Sweep of arc"),    
         CounterMenuItem.new("Precision",      16, 1,       4,   62,  "divisions",  "", "Number of sections per arc"),
         CounterMenuItem.new("Radius of arc", 100, 1,       1,   500, "grid units", "", "Radius of the arc"),
         CounterMenuItem.new("Start of arc",   90, 1,       0,   360, "degrees",    "", "Start angle of arc from the positive x axis"),

         ToggleMenuItem.new ("Type", { "BarrierMaker", "LoadoutZone", "GoalZone" }, 1, true, "Type of item to insert"),

         CounterMenuItem.new("Barrier Width",  50, 1,       1,   500, "grid units", "", "Width of wall if BarrierMaker is selected above"),
         CounterMenuItem.new("Center X",        0, 10, -10000, 10000, "",           "", "X coordinate of center of arc"),
         CounterMenuItem.new("Center Y",        0, 10, -10000, 10000, "",           "", "Y coordinate of center of arc")
      }
end


--
-- Helper to convert degrees to radians
--
function toRads(deg)
   return (deg * math.pi) / 180
end


--
-- The main body of the code gets put in main()
--
function main()
   -- arg table will include values from menu items above, in order
 
   local gridsize = plugin:getGridSize()
   
   local scriptName = arg[0]
   local degrees = arg[1]
   local divisions = arg[2]
   local radius = arg[3] / gridsize
   local startPoint = arg[4]
   local itemType = arg[5]
   local barrierWidth = arg[6]
   local centerX = arg[7] / gridsize
   local centerY = arg[8] / gridsize
   
   
   local levelLine

   -- First part of the level line
   if(itemType == "BarrierMaker") then
       levelLine = itemType .. " " .. barrierWidth .. " "
   else
       levelLine = itemType .. " 0 "
   end
   
   -- Each division is this many radians
   local step = toRads(degrees / divisions)
   
   -- Now add some coordinates
   for x = 0, divisions do
      levelLine = levelLine .. radius * math.cos(x * step) + centerX .. " " .. radius * math.sin(x * step) - centerY .. "  "
   end

   if(degrees == 360) then
      levelLine = levelLine .. radius * math.cos(toRads(startPoint)) + centerX .. " " .. radius * math.sin(toRads(startPoint)) - centerY .. "  "
   end

   -- Now add item to the level
   plugin:addLevelLine(levelLine)
end   

