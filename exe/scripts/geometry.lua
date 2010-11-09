---------------------------------------
--
-- Bitfighter geometry library
-- These functions are available for bots and levelgens
-- Functions written by Joseph Ivie
--
---------------------------------------

local M = {}

function M.flipPoints(points, horizontal)
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

function M.translatePoints(points, px, py)
    local newPoints = {}
    for i=1,#points do
        newPoints[i] = Point(points[i]:x()+px, points[i]:y()+py)
    end
    return newPoints
end

function M.scalePoints(points, sx, sy)
    local newPoints = {}
    for i=1,#points do
        newPoints[i] = Point(points[i]:x()*sx, points[i]:y()*sy)
    end
    return newPoints
end

function M.rotatePoints(points, amount)
    local newPoints = {}
    local amountRadians = amount*math.pi/180
    local pointAngle = 0
    for i=1,#points do
        pointAngle = math.atan2(points[i]:y(),points[i]:x())
        newPoints[i] = Point(points[i]:len()*math.cos(amountRadians+pointAngle), points[i]:len()*math.sin(amountRadians+pointAngle))
    end
    return newPoints
end

function M.transformPoints(points, px, py, sx, sy, rotation)
    local newPoints = {}
    local amountRadians = rotation*math.pi/180
    local pointAngle = 0
    for i=1,#points do
        pointAngle = math.atan2(points[i]:y(),points[i]:x())
        newPoints[i] = Point((points[i]:len()*math.cos(amountRadians+pointAngle))*sx+px, (points[i]:len()*math.sin(amountRadians+pointAngle))*sy+py)
    end
    return newPoints
end

return M