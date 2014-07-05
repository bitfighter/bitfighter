//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "item.h"
#include "game.h"
#include "gameConnection.h"
#include "Level.h"

#include "gameObjectRender.h"
#include "Colors.h"
#include "stringUtils.h"

namespace Zap
{

bool Item::mInitial;

// Constructor
Item::Item(F32 radius) : Parent(radius)
{
   mRadius = radius;
   setPos(Point(0,0));

   static U16 itemId = 1;
   mItemId = itemId++;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Item::~Item()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


bool Item::getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
{
   point = getPos();
   radius = mRadius;
   return true;
}


// Server only  --> Assumes first two params are x and y location; subclasses may read additional params
bool Item::processArguments(S32 argc, const char **argv, Level *level)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= level->getLegacyGridSize();

   setPos(pos);      // Needed by game

   return true;
}


string Item::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + geomToLevelCode();
}


U16 Item::getItemId()
{
   return mItemId;
}


// Client only
void Item::setItemId(U16 id)
{
   mItemId = id;
}


U32 Item::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //U32 retMask = Parent::packUpdate(connection, updateMask, stream);  // Goes to empty function NetObject::packUpdate

   if(stream->writeFlag(updateMask & InitialMask))
      stream->writeRangedU32(mItemId, 0, U16_MAX);    // Send id in inital packet

   if(stream->writeFlag(updateMask & (InitialMask | GeomMask)))
      ((GameConnection *) connection)->writeCompressedPoint(getPos(), stream);

   return 0; //retMask;
}


void Item::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //Parent::unpackUpdate(connection, stream);  // Goes to empty function NetObject::unpackUpdate

   mInitial = stream->readFlag();      // InitialMask
   if(mInitial)     
      mItemId = stream->readRangedU32(0, U16_MAX);

   if(stream->readFlag())              // GeomMask
   {
      Point pos;
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);

      setPos(pos);      // Also sets object extent
   }
}


void Item::setPos(const Point &pos)
{
   Parent::setPos(pos);

   setOutline();
   setMaskBits(GeomMask);
}


// Generates a rectangle around the item; override for a more interesting outline
// Outlines only needed on client... could probably get away with doing nothing on the server
void Item::setOutline()
{
   F32 x = getPos().x;
   F32 y = getPos().y;

   S32 rad = (S32)mRadius; 

   mOutlinePoints.clear();
   mOutlinePoints.reserve(4);

   // Need to rebuild this every time because the object might be moving
   mOutlinePoints.push_back(Point(x - rad, y - rad));
   mOutlinePoints.push_back(Point(x + rad, y - rad));
   mOutlinePoints.push_back(Point(x + rad, y + rad));
   mOutlinePoints.push_back(Point(x - rad, y + rad));
}


const Vector<Point> *Item::getOutline() const
{
   return &mOutlinePoints;
}


// This has no practical effect, but we need a signature to help the compiler figure out the signatures
void Item::setPos(lua_State *L, S32 stackIndex)
{
   Parent::setPos(L, stackIndex);
}


F32 Item::getRadius() const
{
   return mRadius;
}


void Item::setRadius(F32 radius)
{
   mRadius = radius;
}


// Provide generic item rendering; will be overridden
void Item::renderItem(const Point &pos) const
{
#ifndef ZAP_DEDICATED
   drawFilledSquare(pos, 10, Colors::cyan);
#endif
}


void Item::render() const
{
   renderItem(getPos());
}


void Item::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
   renderItem(getPos());                    
}


F32 Item::getEditorRadius(F32 currentScale) const
{
   return (getRadius() + 2) * currentScale;
}


Rect Item::calcExtents()
{
   return Rect(getPos(), mRadius);
}


/////
// Lua interface
/**
 * @luaclass Item
 * 
 * @brief Parent class for most common game items
 */
// Standard methods available to all Items:
//               Fn name           Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getRad,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getShip,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isInCaptureZone,  ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getCaptureZone,   ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(Item, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Item, LUA_METHODS);

#undef LUA_METHODS


const char *Item::luaClassName = "Item";
REGISTER_LUA_SUBCLASS(Item, BfObject);


/**
 * @luafunc num Item::getRad()
 * 
 * @return The radius of the item. For a Teleporter, this is the radius of the
 * entrance. For a ForceFieldProjector, this is the radius of the base.
 */
S32 Item::lua_getRad(lua_State *L) { return returnFloat(L, getRadius()); }


/**
 * @luafunc Zone Item::getCaptureZone()
 * 
 * @brief Returns capture zone holding the item.
 * 
 * @descr Many games do not feature capture zones. For those games, this
 * function will always return nil. Currently only \link FlagItem FlagItems
 * \endlink can be captured.
 * 
 * @return Zone where the item has been captured. Returns nil if the item is not
 * in a capture zone.
 */
S32 Item::lua_getCaptureZone (lua_State *L) { return returnNil(L); }


/**
 * @luafunc Ship Item::getShip()
 * 
 * @brief Returns the ship where the item is mounted.
 * 
 * @descr Most objects cannot be mounted. For those, this function will always
 * return nil.
 * 
 * @return Ship where the item is mounted. Returns nil if the item is not
 * mounted.
 */
S32 Item::lua_getShip (lua_State *L) { return returnNil(L); }


/**
 * @luafunc bool Item::isInCaptureZone()
 *
 * @return `true` if item is held in a capture zone, `false` otherwise.
 */
S32 Item::lua_isInCaptureZone(lua_State *L) { return returnBool(L, false); }


};


