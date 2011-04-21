---------------------------------------
--
-- Bitfighter geometry library
-- These functions are available for bots and levelgens
-- Functions written by Joseph Ivie
--
---------------------------------------

Geom = {}

-- Flip points along the X or Y axis.
function Geom.flipPoints(points, horizontal)
    local newPoints = {}
    for i=1,#points do
        newPoints[i] = points[i]
        if (horizontal) then
            newPoints[i]:setx(-newPoints[i]:x())
        else
            newPoints[i]:sety(-newPoints[i]:y())
        end
    end
    return newPoints
end

-- Move points by px, py
function Geom.translatePoints(points, tx, ty)
    local newPoints = {}
    for i=1,#points do
        newPoints[i] = Point(points[i]:x()+tx, points[i]:y()+ty)
    end
    return newPoints
end


-- Scale points by sx, sy, in reference to the X and Y axes
function Geom.scalePoints(points, sx, sy)
    sy = sy or sx   -- Passing only one param will scale same amount in x & y directions
    local newPoints = {}
    for i=1,#points do
        newPoints[i] = Point(points[i]:x()*sx, points[i]:y()*sy)
    end
    return newPoints
end


-- Rotate points about the point (0,0)
function Geom.rotatePoints(points, angle)
    local newPoints = {}
    local angleRadians = angle*math.pi/180
    local pointAngle = 0
    for i=1,#points do
        pointAngle = math.atan2(points[i]:y(),points[i]:x())
        newPoints[i] = Point(points[i]:len()*math.cos(angleRadians+pointAngle), points[i]:len()*math.sin(angleRadians+pointAngle))
    end
    return newPoints
end


-- Apply a full transformation to the points, doing a combination of the above in a single operation.  Scales, rotates, then translates.
function Geom.transformPoints(points, tx, ty, sx, sy, angle)
    -- Check for single scaling factor, and adjust args
    if not angle then
      sx = sy
      sy = angle
    end
    
    local newPoints = {}
    local angleRadians = angle*math.pi/180
    local pointAngle = 0
    for i=1,#points do
        pointAngle = math.atan2(points[i]:y(),points[i]:x())
        newPoints[i] = Point((points[i]:len()*math.cos(angleRadians+pointAngle))*sx+tx, (points[i]:len()*math.sin(angleRadians+pointAngle))*sy+ty)
    end
    return newPoints
end