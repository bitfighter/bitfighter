-- Ugly script that should now be uber-powerful
-- mostly stolen from _k - thanks!
-- hacked into inogural plugin, probably making it uglier yet

-- Defaults
ItemType = "BarrierMaker"
BarrierWidth = 50
CenterX = 0
CenterY = 0
radius = 1
degrees = 90
StartPoint = 0
precision = 30


function getArgsMenu()  -- Returns a table of strings representing the cmd line args
   
   return "Create Arc",	     -- Title shown on menu
      {
         CounterMenuItem:new("Angle",          90, 1,     0,  360, "deg.",       "", "Sweep of arc"),    
         CounterMenuItem:new("Points",      64, 1,     4,  512, "points",     "", "Number of points to complete a full circle"),
         CounterMenuItem:new("Radius of arc", 100, 1,     1,  500, "grid units", "", "Radius of the arc"),
         CounterMenuItem:new("Start of arc",   90, 1,     0,  360, "degrees",    "", "Start angle of arc from the positive x axis"),
         ToggleMenuItem:new ("Type", { "BarrierMaker", "LoadoutZone", "GoalZone" }, 1, true, "Type of item to insert"),
         CounterMenuItem:new("Barrier Width",  50, 1,     1,   50,  "grid units", "", "With of wall if BarrierMaker is selected above"),
         CounterMenuItem:new("Center X",        0, 10, -10000, 10000,  "",           "", "X coordinate of center of arc"),
         CounterMenuItem:new("Center Y",        0, 10, -10000, 10000,  "",           "", "Y coordinate of center of arc")
      }

end


-- Do the logic and build the level line

-- Quick helper to convert from degrees to radians
function toRads(deg)
   return (deg * 2 * math.pi)/360
end


function main()
   -- arg table will include values from menu items above, in order
  
   local gridsize = levelgen:getGridSize()
   
   scriptName = arg[0]
   degrees = arg[1]
   precision = arg[2]
   radius = arg[3] / gridsize
   StartPoint = arg[4]
   ItemType = arg[5]
   BarrierWidth = arg[6]
   CenterX = arg[7] / gridsize
   CenterY = arg[8] / gridsize
   
   
   local Curve

   if(ItemType == "BarrierMaker") then
       Curve = ItemType .. " " .. BarrierWidth .. " "
   else
       Curve = ItemType .. " 0 "
   end

   for x = toRads(StartPoint), toRads(degrees) + toRads(StartPoint),  2 * math.pi / precision do
      Curve = Curve .. radius * math.cos(x) + CenterX .. " " .. radius * math.sin(x) - CenterY .. "  "
   end

   if(degrees == 360) then
      Curve = Curve .. radius * math.cos(toRads(StartPoint)) + CenterX .. " " .. radius * math.sin(toRads(StartPoint)) - CenterY .. "  "
   end

   print(Curve)  -- prints to console

   levelgen:addLevelLine(Curve)
end   

