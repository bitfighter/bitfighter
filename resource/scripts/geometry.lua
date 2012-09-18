---------------------------------------
--
-- Bitfighter geometry library
-- These functions are available for bots, levelgens, and plugins
-- Functions written by Joseph Ivie
--
---------------------------------------

--[[
@luaclass Geom
@brief    Library of various geometric transforms.
@descr    The %Geom class provides a collection of useful geometric operations.

In all cases below, \em geom refers to a table of points.

--]]

Geom = {}

--[[ 
@luafunc Geom.flip(geom, horizontal)
@brief   Flip points along the x- or y-axis.
@param   geom - The geometry to modify.
@param   horizontal - Pass true to flip along the x-axis, false to flip along the y-axis.
 --]]
function Geom.flip(geom, horizontal)
    local newPoints = {}
    for i = 1, #geom do
        newPoints[i] = geom[i]
        if (horizontal) then
            newPoints[i]:setx(-newPoints[i]:x())
        else
            newPoints[i]:sety(-newPoints[i]:y())
        end
    end
    return newPoints
end


--[[ 
@luafunc Geom.translate(geom, tx, ty)
@brief   Translate (offset) the specified geometry by tx, ty.
@param   geom - The geometry to modify.
@param   tx - The amount to add to the x-coord of each point in \em geom.
@param   ty - The amount to add to the y-coord of each point in \em geom.
 --]]
function Geom.translate(geom, tx, ty)
    local newPoints = {}
    for i = 1, #geom do
        newPoints[i] = Point(geom[i]:x() + tx, geom[i]:y() + ty)
    end
    return newPoints
end


--[[ 
@luafunc Geom.scale(geom, sx, sy)
@brief   Scale geom by sx, sy, in reference to the x- and y-axes.
@param   geom - The geometry to modify.
@descr   If sy is omitted, geom will be scaled evenly horizontally and vertically without distortion.
@param   sx - The amount to scale each point in \em geom horizontally.
@param   sy - (Optional) The amount to scale each point in \em geom vertically.  Defaults to \em sx.
--]]
function Geom.scale(geom, sx, sy)
    sy = sy or sx   -- Passing only one param will scale same amount in x & y directions
    local newPoints = {}
    for i = 1, #geom do
        newPoints[i] = Point(geom[i]:x() * sx, geom[i]:y() * sy)
    end
    return newPoints
end


--[[ 
@luafunc Geom.rotate(geom, angle)
@brief   Rotate geom about the point (0,0).
@param   geom - The geometry to modify.
@param   angle - The angle (in degrees) to rotate \em geom.
 --]]
function Geom.rotate(geom, angle)
    local newPoints = {}
    local angleRadians = angle * math.pi / 180
    local pointAngle = 0
    for i = 1, #geom do
        pointAngle = math.atan2(geom[i]:y(), geom[i]:x())
        newPoints[i] = Point(geom[i]:len() * math.cos(angleRadians + pointAngle), geom[i]:len() * math.sin(angleRadians + pointAngle))
    end
    return newPoints
end


--[[ 
@luafunc Geom.transform(geom, tx, ty, sx, sy, angle)
@brief   Transform geom by scaling, rotating, and transforming.
@desc    Apply a full transformation to the points in \em geom, doing a combination of the above in a single operation.  
         Scales, rotates, then translates.  Performing these operations together is more effient than applying them
         individually.

Note that \em sy can be omitted for uniform scaling horizontally and vertically.

@param   geom - The geometry to modify.
@param   tx - The amount to add to the x-coord of each point in \em geom.
@param   ty - The amount to add to the y-coord of each point in \em geom.
@param   sx - The amount to scale each point in \em geom horizontally.
@param   sy - (Optional) The amount to scale each point in \em geom vertically.  Defaults to \em sx.
@param   angle - The angle (in degrees) to rotate \em geom.
 --]]
function Geom.transform(geom, tx, ty, sx, sy, angle)
    -- Check for single scaling factor, and adjust args
    if not angle then
      sx = sy
      sy = angle
    end
    
    local newPoints = {}
    local angleRadians = angle * math.pi / 180
    local pointAngle = 0
    for i = 1, #geom do
        pointAngle = math.atan2(geom[i]:y(), geom[i]:x())
        newPoints[i] = Point((geom[i]:len() * math.cos(angleRadians + pointAngle)) * sx + tx, 
                             (geom[i]:len() * math.sin(angleRadians + pointAngle)) * sy + ty )
    end
    return newPoints
end