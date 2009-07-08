-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- This is called by the robot's idle routine each tick.  
-- This function must be present for the robot to work!
function getMove()
		
		--zone = r:getZone(r:getPosXY())
		
		local x,y = r:getPosXY()
		
		local angle = r:getAimAngle()
		--r:setAngle(angle)
		
		
		-- We've arrived!  Advance to next destination
		if inDestZone() then
			destX, destY = getNextDestXY()
		end
		
		-- Travel towards destX,destY at full speed
		r:setThrustXY(1, destX, destY)
		
		-- Fire at nearby ships
		ship = r:findObjects(ShipType)
		if(ship ~= nil) then
			r:logprint("Found ship at " ..ship.x..","..ship.y);
			r:setAngleXY(ship.x, ship.y)
			--r:fire()
		end
			
		
		ctr = ctr + 1
		if ctr >= 100 or destX == nil or destY == nil then
			destX, destY = getCurDestXY()
			r:setWeapon(weap)
			--r:logprint("Bang!" .. weap)
			--r:fire()
			r:globalMsg("I'm at" .. x.. "," ..y.."(z-> "..r:getCurrentZone()..")")

			ctr = 0
			weap = weap + 1
		end
end

-- Get next destination from our list of destinations
function getNextDestXY()

	lastDest = curDest
	-- Advance to next zone	
	curDest = curDest + 1
	if curDest > #zones then
		curDest = 1	-- First element of Lua array is 1
	end		
	
	return getCurDestXY()
end

-- Reaffirm current destination
function getCurDestXY()

	local x,y
	
	-- Initially, head towards the center of our first zone...
	if(initialDest) then
		return r:getZoneCenterXY(zones[curDest])
	end
	
	-- Otherwise, head towards the appropriate gateway
	x,y = r:getGatewayToXY(zones[lastDest], zones[curDest])
	
	-- If the gateway looks bad, head towards the center of the zone
	if(x == nil or y == nil) then 
		r:logprint("============================================NIL=="..zones[curDest].." "..zones[lastDest])
		x,y = r:getZoneCenterXY(zones[curDest])
	end
	
	return x,y
end

-- Are we in zone zones[curDest]
function inDestZone()
	local z = r:getCurrentZone()
	return (z ~= nil and z == zones[curDest])
end

-- Are we at (or near) point dx, dy?
function atDest()
	delta = 50
	local x, y = r:getPosXY()
	return (dist(x, y, destX, destY) < delta)
end


-- Return the distance between points x1,y1 and x2,y2
function dist(x1, y1, x2, y2)
	return( math.sqrt((x2 - x1)^2 + (y2 - y1)^2) )
end

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- This function is called once and should return the robot's name
function getName()
	return("Mr. Roboto")
end

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- Setup code here: this is run once when the robot is initialized, and
-- any variables set here will persist throughout the robot's life.  These
-- variables can be accessed from various functions defined in this file.

-- Start up a new LuaGameObject wrapper class and pass the global gameobject
-- C++ lightuserdata pointer into it
r = LuaGameObject(Robot) -- This is a reference to our robot
ctr = 0
weap = 0
currZone = -1		-- Current zone we're in
r:globalMsg("Hello, I'm "..getName() )
r:logprint("Hello, I'm "..getName() )
zones = {0,1,2,3,4}
curDest = 0	-- Will be incremented to 1 at first call of getNextDestXY()
initialDest = true
destX, destY = getNextDestXY()
r:globalMsg("Heading for "..destX..","..destY )
initialDest = false
lastDest = #zones
r:setWeapon(3)