
//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "gameItems.h"
#include "item.h"
#include "ship.h"
#include "gameObjectRender.h"

#include "SoundSystem.h"
#include "Colors.h"
#include "stringUtils.h"
#include "game.h"

#include "SDL/SDL_opengl.h"
#include "UI.h"

#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(RepairItem);

// Constructor
RepairItem::RepairItem(Point pos) : PickupItem(pos, (F32)REPAIR_ITEM_RADIUS, DEFAULT_RESPAWN_TIME * 1000) 
{ 
   mObjectTypeNumber = RepairItemTypeNumber;
}


RepairItem *RepairItem::clone() const
{
   return new RepairItem(*this);
}


// Runs on server, returns true if we're doing the pickup, false otherwise
bool RepairItem::pickup(Ship *theShip)
{
   if(theShip->getHealth() >= 1)
      return false;

   DamageInfo di;
   di.damageAmount = -0.5f;      // Negative damage = repair!
   di.damageType = DamageTypePoint;
   di.damagingObject = this;

   theShip->damageObject(&di);
   return true;
}


// Runs on client when item's unpack method signifies the item has been picked up
void RepairItem::onClientPickup()
{
   SoundSystem::playSoundEffect(SFXShipHeal, getRenderPos(), getRenderVel());
}


void RepairItem::renderItem(Point pos)
{
   if(!isVisible())
      return;

   renderRepairItem(pos);
}


void RepairItem::renderDock()
{
   renderRepairItem(getVert(0), true, 0, 1);
}


F32 RepairItem::getEditorRadius(F32 currentScale)
{
   return 22 * currentScale;
}


const char RepairItem::className[] = "RepairItem";      // Class name as it appears to Lua scripts

// Lua constructor
RepairItem::RepairItem(lua_State *L)
{
   mObjectTypeNumber = RepairItemTypeNumber;
}


// Define the methods we will expose to Lua
Lunar<RepairItem>::RegType RepairItem::methods[] =
{
   // Standard gameItem methods
   method(RepairItem, getClassID),
   method(RepairItem, getLoc),
   method(RepairItem, getRad),
   method(RepairItem, getVel),
   method(RepairItem, getTeamIndx),

   // Class specific methods
   method(RepairItem, isVis),
   {0,0}    // End method list
};

S32 RepairItem::isVis(lua_State *L) { return returnBool(L, isVisible()); }        // Is RepairItem visible? (returns boolean)

////////////////////////////////////////
////////////////////////////////////////


TNL_IMPLEMENT_NETOBJECT(EnergyItem);

// Constructor
EnergyItem::EnergyItem(Point p) : PickupItem(p, 20, DEFAULT_RESPAWN_TIME * 1000) 
{ 
   mObjectTypeNumber = EnergyItemTypeNumber;
};   


EnergyItem *EnergyItem::clone() const
{
   return new EnergyItem(*this);
}


// Runs on server, returns true if we're doing the pickup, false otherwise
bool EnergyItem::pickup(Ship *theShip)
{
   S32 energy = theShip->getEnergy();
   S32 maxEnergy = theShip->getMaxEnergy();

   if(energy >= maxEnergy)      // Don't need no stinkin' energy!!
      return false;

   theShip->changeEnergy(maxEnergy / 2);     // Bump up energy by 50%, changeEnergy() sets energy delta

   return true;
}


// Runs on client when item's unpack method signifies the item has been picked up
void EnergyItem::onClientPickup()
{
   SoundSystem::playSoundEffect(SFXShipHeal, getRenderPos(), getRenderVel());
}


void EnergyItem::renderItem(Point pos)
{
   if(!isVisible())
      return;

   renderEnergyItem(pos);
}


const char EnergyItem::className[] = "EnergyItem";      // Class name as it appears to Lua scripts

// Lua constructor
EnergyItem::EnergyItem(lua_State *L)
{
   mObjectTypeNumber = EnergyItemTypeNumber;
}


// Define the methods we will expose to Lua
Lunar<EnergyItem>::RegType EnergyItem::methods[] =
{
   // Standard gameItem methods
   method(EnergyItem, getClassID),
   method(EnergyItem, getLoc),
   method(EnergyItem, getRad),
   method(EnergyItem, getVel),
   method(EnergyItem, getTeamIndx),

   // Class specific methods
   method(EnergyItem, isVis),
   {0,0}    // End method list
};

S32 EnergyItem::isVis(lua_State *L) { return returnBool(L, isVisible()); }        // Is EnergyItem visible? (returns boolean)


////////////////////////////////////////
////////////////////////////////////////

AbstractSpawn::AbstractSpawn(const Point &pos, S32 time)
{
   setVert(pos, 0);
   setRespawnTime(time);
};


void AbstractSpawn::setRespawnTime(S32 time)       // in seconds
{
   mSpawnTime = time;
   mTimer.setPeriod(time * 1000);
   mTimer.reset();
}


F32 AbstractSpawn::getEditorRadius(F32 currentScale)
{
   return 12;     // Constant size, regardless of zoom
}


bool AbstractSpawn::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc >= 5)
      return false;

   Point pos;
   pos.read(argv + 1);
   pos *= game->getGridSize();

   setVert(pos, 0);

   S32 time = (argc > 3) ? atoi(argv[3]) : getDefaultRespawnTime();

   setRespawnTime(time);

   return true;
}


string AbstractSpawn::toString(F32 gridSize) const
{
   // <<spawn class name>> <x> <y> <time>
   return string(getClassName()) + " " + geomToString(gridSize) + " " + itos(mSpawnTime);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
Spawn::Spawn(const Point &pos, S32 time) : AbstractSpawn(pos, time)
{
   mObjectTypeNumber = ShipSpawnTypeNumber;
}

Spawn::~Spawn()
{
}


Spawn *Spawn::clone() const
{
   return new Spawn(*this);
}


bool Spawn::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc >= 4)
      return false;

   S32 teamIndex = atoi(argv[0]);
   setTeam(teamIndex);

   Point pos;
   pos.read(argv + 1);
   pos *= game->getGridSize();

   setVert(pos, 0);

   return true;
}


string Spawn::toString(F32 gridSize) const
{
   // Spawn <team> <x> <y> 
   return string(getClassName()) + " " + itos(mTeam) + " " + geomToString(gridSize);
}


void Spawn::renderEditor(F32 currentScale)
{
   Point pos = getVert(0);

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(1/currentScale, 1/currentScale, 1);    // Make item draw at constant size, regardless of zoom
      renderSquareItem(Point(0,0), getGame()->getTeamColor(mTeam), 1, &Colors::white, 'S');
   glPopMatrix();   
}


void Spawn::renderDock()
{
   renderEditor(1);
}


////////////////////////////////////////
////////////////////////////////////////

ItemSpawn::ItemSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
AsteroidSpawn::AsteroidSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   mObjectTypeNumber = AsteroidSpawnTypeNumber;
}

AsteroidSpawn::~AsteroidSpawn()
{
}


AsteroidSpawn *AsteroidSpawn::clone() const
{
   return new AsteroidSpawn(*this);
}


void AsteroidSpawn::spawn(Game *game, const Point &pos)
{
   Asteroid *asteroid = dynamic_cast<Asteroid *>(TNL::Object::create("Asteroid"));   // Create a new asteroid

   F32 ang = TNL::Random::readF() * Float2Pi;

   asteroid->setPosAng(pos, ang);

   asteroid->addToGame(game, game->getGameObjDatabase());              // And add it to the list of game objects
}


static void renderAsteroidSpawn(const Point &pos)
{
   F32 scale = 0.8f;
   static const Point p(0,0);

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(scale, scale, 1);
      renderAsteroid(p, 2, .1f);

      glColor(Colors::white);
      drawCircle(p, 13);
   glPopMatrix();  
}


void AsteroidSpawn::renderEditor(F32 currentScale)
{
   Point pos = getVert(0);

   glPushMatrix();
      glTranslate(pos);
      glScale(1/currentScale);    // Make item draw at constant size, regardless of zoom
      renderAsteroidSpawn(Point(0,0));
   glPopMatrix();   
}


void AsteroidSpawn::renderDock()
{
   renderAsteroidSpawn(getVert(0));
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
CircleSpawn::CircleSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   mObjectTypeNumber = CircleSpawnTypeNumber;
}


CircleSpawn *CircleSpawn::clone() const
{
   return new CircleSpawn(*this);
}


void CircleSpawn::spawn(Game *game, const Point &pos)
{
   for(S32 i = 0; i < 10; i++)
   {
      Circle *circle = dynamic_cast<Circle *>(TNL::Object::create("Circle"));   // Create a new Circle
      F32 ang = TNL::Random::readF() * Float2Pi;

      circle->setPosAng(pos, ang);

      circle->addToGame(game, game->getGameObjDatabase());              // And add it to the list of game objects
   }
}


static void renderCircleSpawn(const Point &pos)
{
   F32 scale = 0.8f;
   static const Point p(0,0);

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(scale, scale, 1);
      drawCircle(p, 8);

      glColor(Colors::white);
      drawCircle(p, 13);
   glPopMatrix();  
}


void CircleSpawn::renderEditor(F32 currentScale)
{
   Point pos = getVert(0);

   glPushMatrix();
      glTranslate(pos);
      glScale(1/currentScale);    // Make item draw at constant size, regardless of zoom
      renderCircleSpawn(Point(0,0));
   glPopMatrix();   
}


void CircleSpawn::renderDock()
{
   renderCircleSpawn(getVert(0));
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
FlagSpawn::FlagSpawn(const Point &pos, S32 time) : AbstractSpawn(pos, time)
{
   mObjectTypeNumber = FlagSpawnTypeNumber;
}

FlagSpawn::~FlagSpawn()
{
}


FlagSpawn *FlagSpawn::clone() const
{
   return new FlagSpawn(*this);
}


void FlagSpawn::renderEditor(F32 currentScale)
{
   Point pos = getVert(0);

   glPushMatrix();
      glTranslatef(pos.x + 1, pos.y, 0);
      glScalef(0.4f/currentScale, 0.4f/currentScale, 1);
      Color color = getTeamColor(mTeam);  // To avoid taking address of temporary
      renderFlag(0, 0, &color);

      glColor(Colors::white);
      drawCircle(-4, 0, 26);
   glPopMatrix();
}


void FlagSpawn::renderDock()
{
   renderEditor(1);
}
 

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Asteroid);
class LuaAsteroid;

static F32 asteroidVel = 250;

static const F32 ASTEROID_MASS = 4;

// Constructor
Asteroid::Asteroid() : EditorItem(Point(0,0), true, (F32)ASTEROID_RADIUS, ASTEROID_MASS)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = AsteroidTypeNumber;
   mSizeIndex = 0;     // Higher = smaller
   hasExploded = false;
   mDesign = TNL::Random::readI(0, AsteroidDesigns - 1);

   // Give the asteroids some intial motion in a random direction
   F32 ang = TNL::Random::readF() * Float2Pi;
   F32 vel = asteroidVel;

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].vel.x = vel * cos(ang);
      mMoveState[i].vel.y = vel * sin(ang);
   }

   mKillString = "crashed into an asteroid";
}


Asteroid *Asteroid::clone() const
{
   return new Asteroid(*this);
}


void Asteroid::renderItem(Point pos)
{
   if(!hasExploded)
      renderAsteroid(pos, mDesign, asteroidRenderSize[mSizeIndex]);
}


void Asteroid::renderDock()
{
   renderAsteroid(getVert(0), 2, .1f);
}


F32 Asteroid::getEditorRadius(F32 currentScale)
{
   return 75 * currentScale;
}


bool Asteroid::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   center = mMoveState[state].pos;
   radius = F32(ASTEROID_RADIUS) * asteroidRenderSize[mSizeIndex];
   return true;
}


bool Asteroid::getCollisionPoly(Vector<Point> &polyPoints) const
{
   for(S32 i = 0; i < AsteroidPoints; i++)
   {
      Point p = Point(mMoveState[MoveObject::ActualState].pos.x + (F32) AsteroidCoords[mDesign][i][0] * asteroidRenderSize[mSizeIndex],
                      mMoveState[MoveObject::ActualState].pos.y + (F32) AsteroidCoords[mDesign][i][1] * asteroidRenderSize[mSizeIndex] );

      polyPoints.push_back(p);
   }

   return true;
}


#define ABS(x) (((x) > 0) ? (x) : -(x))


void Asteroid::damageObject(DamageInfo *theInfo)
{
   if(hasExploded)   // Avoid index out of range error
      return; 

   // Compute impulse direction
   mSizeIndex++;
   
   TNLAssert((U32)mSizeIndex <= asteroidRenderSizes, "Asteroid::damageObject mSizeIndex out of range");

   if(asteroidRenderSize[mSizeIndex] == -1)    // Kill small items
   {
      hasExploded = true;
      deleteObject(500);
      setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...
      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(F32(ASTEROID_RADIUS) * asteroidRenderSize[mSizeIndex]);

   F32 ang = TNL::Random::readF() * Float2Pi;      // Sync
   //F32 vel = asteroidVel;

   setPosAng(getActualPos(), ang);

   Asteroid *newItem = dynamic_cast<Asteroid *>(TNL::Object::create("Asteroid"));
   newItem->setRadius(F32(ASTEROID_RADIUS) * asteroidRenderSize[mSizeIndex]);

   F32 ang2;
   do
      ang2 = TNL::Random::readF() * Float2Pi;      // Sync
   while(ABS(ang2 - ang) < .0436 );    // That's 20 degrees in radians, folks!

   newItem->setPosAng(getActualPos(), ang2);

   newItem->mSizeIndex = mSizeIndex;
   newItem->addToGame(gServerGame, gServerGame->getGameObjDatabase());    // And add it to the list of game objects
}


void Asteroid::setPosAng(Point pos, F32 ang)
{
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = ang;
      mMoveState[i].vel.x = asteroidVel * cos(ang);
      mMoveState[i].vel.y = asteroidVel * sin(ang);
   }
}


U32 Asteroid::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
   {
      stream->writeEnum(mSizeIndex, mSizeIndexLength);
      stream->writeEnum(mDesign, AsteroidDesigns);
   }

   stream->writeFlag(hasExploded);

   return retMask;
}


void Asteroid::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mSizeIndex = stream->readEnum(mSizeIndexLength);
      setRadius(F32(ASTEROID_RADIUS) * asteroidRenderSize[mSizeIndex]);
      mDesign = stream->readEnum(AsteroidDesigns);

      if(!mInitial)
         SoundSystem::playSoundEffect(SFXAsteroidExplode, mMoveState[RenderState].pos, Point());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(mMoveState[RenderState].pos);
   }
}


bool Asteroid::collide(GameObject *otherObject)
{
   if(isGhost())   //client only, to try to prevent asteroids desync...
   {
      Ship *ship = dynamic_cast<Ship *>(otherObject);
      if(ship)
      {
         // Client does not know if we actually get destroyed from asteroids
         // prevents bouncing off asteroids, then LAG puts back to position.
         if(! ship->isModuleActive(ModuleShield)) return false;
      }
   }

   // Asteroids don't collide with one another!
   return dynamic_cast<Asteroid *>(otherObject) ? false : true;
}


// Client only
void Asteroid::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos, Point());
   // FXManager::emitBurst(pos, Point(.1, .1), Colors::white, Colors::white, 10);
}


const char Asteroid::className[] = "Asteroid";      // Class name as it appears to Lua scripts

// Lua constructor
Asteroid::Asteroid(lua_State *L)
{
   // Do we want to construct these from Lua?  If so, do that here!
}


// Define the methods we will expose to Lua
Lunar<Asteroid>::RegType Asteroid::methods[] =
{
   // Standard gameItem methods
   method(Asteroid, getClassID),
   method(Asteroid, getLoc),
   method(Asteroid, getRad),
   method(Asteroid, getVel),
   method(Asteroid, getTeamIndx),

   // Class specific methods
   method(Asteroid, getSize),
   method(Asteroid, getSizeCount),

   {0,0}    // End method list
};


S32 Asteroid::getSize(lua_State *L) { return returnInt(L, getSizeIndex()); }         // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
S32 Asteroid::getSizeCount(lua_State *L) { return returnInt(L, getSizeCount()); }    // Number of indexes of size we can have (returns int)


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Circle);
class LuaCircle;

static F32 CIRCLE_VEL = 250;

static const F32 CIRCLE_MASS = 4;

// Constructor
Circle::Circle() : EditorItem(Point(0,0), true, (F32)CIRCLE_RADIUS, CIRCLE_MASS)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = CircleTypeNumber;
   hasExploded = false;

   // Give the asteroids some intial motion in a random direction
   F32 ang = TNL::Random::readF() * Float2Pi;
   F32 vel = CIRCLE_VEL;

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].vel.x = vel * cos(ang);
      mMoveState[i].vel.y = vel * sin(ang);
   }

   mKillString = "crashed into an circle";
}


Circle *Circle::clone() const
{
   return new Circle(*this);
}


void Circle::idle(GameObject::IdleCallPath path)
{
   //if(path == GameObject::ServerIdleMainLoop)
   {
      // Find nearest ship
      fillVector.clear();
      findObjects((TestFunc)isShipType, fillVector, Rect(getActualPos(), 1200));

      F32 dist = F32_MAX;
      Ship *closest = NULL;

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         Ship *ship = dynamic_cast<Ship *>(fillVector[i]);
         F32 d = getActualPos().distSquared(ship->getActualPos());
         if(d < dist)
         {
            closest = ship;
            dist = d;
         }
      }

      if(!closest)
         return;

      F32 ang = getActualPos().angleTo(closest->getActualPos());

      Point v = getActualVel();
      v += closest->getActualPos() - getActualPos();

      v.normalize(CIRCLE_VEL);

      setActualVel(v);
   }

   Parent::idle(path);
}


void Circle::renderItem(Point pos)
{
   if(!hasExploded)
   {
      glColor(Colors::red);
      drawCircle(pos, CIRCLE_RADIUS);
   }
}


void Circle::renderDock()
{
   drawCircle(getVert(0), 2);
}


F32 Circle::getEditorRadius(F32 currentScale)
{
   return CIRCLE_RADIUS * currentScale;
}


bool Circle::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   return Parent::getCollisionCircle(state, center, radius);
}


bool Circle::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}


void Circle::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   hasExploded = true;
   deleteObject(500);
   setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...
   return;
}


void Circle::setPosAng(Point pos, F32 ang)
{
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = ang;
      mMoveState[i].vel.x = asteroidVel * cos(ang);
      mMoveState[i].vel.y = asteroidVel * sin(ang);
   }
}


U32 Circle::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   stream->writeFlag(hasExploded);

   return retMask;
}


void Circle::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(mMoveState[RenderState].pos);
   }
}


bool Circle::collide(GameObject *otherObject)
{
   return true;
}


// Client only
void Circle::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos, Point());
}


const char Circle::className[] = "Circle";      // Class name as it appears to Lua scripts

// Lua constructor
Circle::Circle(lua_State *L)
{
   // Do we want to construct these from Lua?  If so, do that here!
}


// Define the methods we will expose to Lua
Lunar<Circle>::RegType Circle::methods[] =
{
   // Standard gameItem methods
   method(Circle, getClassID),
   method(Circle, getLoc),
   method(Circle, getRad),
   method(Circle, getVel),
   method(Circle, getTeamIndx),

   {0,0}    // End method list
};


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Worm);

Worm::Worm() : Item(Point(0,0), true, (F32)WORM_RADIUS, 1)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = WormTypeNumber;
   hasExploded = false;

   // Give the worm some intial motion in a random direction
   F32 ang = TNL::Random::readF() * Float2Pi;
   mNextAng = TNL::Random::readF() * Float2Pi;
   F32 vel = asteroidVel;

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].vel.x = vel * cos(ang);
      mMoveState[i].vel.y = vel * sin(ang);
   }

   mDirTimer.reset(1000);

   mKillString = "killed by a worm";
}

void Worm::renderItem(Point pos)
{
   if(!hasExploded)
      renderWorm(pos);
}


bool Worm::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   center = mMoveState[state].pos;
   radius = F32(WORM_RADIUS);
   return true;
}


bool Worm::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}

bool Worm::collide(GameObject *otherObject)
{
   // Worms don't collide with one another!
   return /*dynamic_cast<Worm *>(otherObject) ? false : */true;
}


static const S32 wormVel = 250;
void Worm::setPosAng(Point pos, F32 ang)
{
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = ang;
      mMoveState[i].vel.x = wormVel * cos(ang);
      mMoveState[i].vel.y = wormVel * sin(ang);
   }
}

void Worm::damageObject(DamageInfo *theInfo)
{
   hasExploded = true;
   deleteObject(500);
}


void Worm::idle(GameObject::IdleCallPath path)
{
   if(!isInDatabase())
      return;

   if(mDirTimer.update(mCurrentMove.time))
   {
      //mMoveState[ActualState].angle = TNL::Random::readF() * Float2Pi;
      F32 ang = mMoveState[ActualState].vel.ATAN2();
      setPosAng(mMoveState[ActualState].pos, ang + (TNL::Random::readI(0,2) - 1) * FloatPi / 4);
      mDirTimer.reset(1000);
      setMaskBits(InitialMask);     // WRONG!!
   }

   Parent::idle(path);
}


U32 Worm::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   stream->writeFlag(hasExploded);

   return retMask;
}


void Worm::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      //onItemExploded(mMoveState[RenderState].pos);
   }
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(TestItem);

static const F32 TEST_ITEM_MASS = 4;

// Constructor
TestItem::TestItem() : EditorItem(Point(0,0), true, (F32)TEST_ITEM_RADIUS, TEST_ITEM_MASS)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = TestItemTypeNumber;
}


TestItem *TestItem::clone() const
{
   return new TestItem(*this);
}


void TestItem::renderItem(Point pos)
{
   renderTestItem(pos);
}


void TestItem::renderDock()
{
   renderTestItem(getVert(0), 8);
}


F32 TestItem::getEditorRadius(F32 currentScale)
{
   return getRadius() * currentScale;
}


// Appears to be server only??
void TestItem::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3f;
}


bool TestItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   //for(S32 i = 0; i < 8; i++)    // 8 so that first point gets repeated!  Needed?  Maybe not
   //{
   //   Point p = Point(60 * cos(i * Float2Pi / 7 + FloatHalfPi) + mMoveState[ActualState].pos.x, 60 * sin(i * Float2Pi / 7 + FloatHalfPi) + mMoveState[ActualState].pos.y);
   //   polyPoints.push_back(p);
   //}

   return false;
}


const char TestItem::className[] = "TestItem";      // Class name as it appears to Lua scripts

// Lua constructor
TestItem::TestItem(lua_State *L)
{
   // Do nothing, for now...  should take params from stack and create testItem object
}


// Define the methods we will expose to Lua
Lunar<TestItem>::RegType TestItem::methods[] =
{
   // Standard gameItem methods
   method(TestItem, getClassID),
   method(TestItem, getLoc),
   method(TestItem, getRad),
   method(TestItem, getVel),

   {0,0}    // End method list
};

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(ResourceItem);

static const F32 RESOURCE_ITEM_MASS = 1;

   // Constructor
ResourceItem::ResourceItem() : EditorItem(Point(0,0), true, (F32)RESOURCE_ITEM_RADIUS, RESOURCE_ITEM_MASS)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = ResourceItemTypeNumber;
}


ResourceItem *ResourceItem::clone() const
{
   return new ResourceItem(*this);
}



void ResourceItem::renderItem(Point pos)
{
   renderResourceItem(pos);
}


void ResourceItem::renderDock()
{
   renderResourceItem(getVert(0), .4f, 0, 1);
}


bool ResourceItem::collide(GameObject *hitObject)
{
   if(mIsMounted)
      return false;

   if( ! (isShipType(hitObject->getObjectTypeNumber())) )
      return true;

   // Ignore collisions that occur to recently dropped items.  Make sure item is ready to be picked up! 
   if(mDroppedTimer.getCurrent())    
      return false;

   Ship *ship = dynamic_cast<Ship *>(hitObject);
   if(!ship || ship->hasExploded)
      return false;

   if(ship->hasModule(ModuleEngineer) && !ship->isCarryingItem(ResourceItemTypeNumber))
   {
      if(!isGhost())
         mountToShip(ship);
      return false;
   }
   return true;
}


void ResourceItem::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3f;
}


void ResourceItem::onItemDropped()
{
   if(mMount.isValid())
   {
      this->setActualPos(mMount->getActualPos()); 
      this->setActualVel(mMount->getActualVel() * 1.5);
   }   
   
   Parent::onItemDropped();
}


const char ResourceItem::className[] = "ResourceItem";      // Class name as it appears to Lua scripts

// Lua constructor
ResourceItem::ResourceItem(lua_State *L)
{
   // Do nothing, for now...  should take params from stack and create testItem object
}


// Define the methods we will expose to Lua
Lunar<ResourceItem>::RegType ResourceItem::methods[] =
{
   // Standard gameItem methods
   method(ResourceItem, getClassID),
   method(ResourceItem, getLoc),
   method(ResourceItem, getRad),
   method(ResourceItem, getVel),
   method(ResourceItem, getTeamIndx),

   // item methods
   method(ResourceItem, isInCaptureZone),
   method(ResourceItem, getCaptureZone),
   method(ResourceItem, isOnShip),
   method(ResourceItem, getShip),

   {0,0}    // End method list
};

};


