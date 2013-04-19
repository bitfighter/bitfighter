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

#include "flagItem.h"
#include "goalZone.h"
#include "gameType.h"
#include "ServerGame.h"
#include "stringUtils.h"      // For itos
#include "gameConnection.h"
#include "gameObjectRender.h"
#include "ClientInfo.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(FlagItem);
/**
  *  @luaconst  FlagItem::FlagItem()
  *  @luaconst  FlagItem::FlagItem(point)
  *  @luaconst  FlagItem::FlagItem(point, team)
  */
// Combined Lua / C++ default constructor
FlagItem::FlagItem(lua_State *L) : Parent(Point(0,0), true, (F32)Ship::CollisionRadius) // radius was 20
{
   initialize();
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, END }, { PT, TEAM_INDX, END }}, 3 };
      
      S32 profile = checkArgList(L, constructorArgList, "FlagItem", "constructor");

      if(profile == 1)
         setPos(L, 1);

      else if(profile == 2)
      {
         setPos(L, 1);
         setTeam(L, 2);
      }
   }
}


// Alternate constructor, currently used by NexusFlag
FlagItem::FlagItem(const Point &pos, bool collidable, float radius, float mass) : Parent(pos, collidable, radius, mass)
{
   initialize();
}


void FlagItem::initialize()
{
   mIsAtHome = true;    // All flags start off at home!

   mNetFlags.set(Ghostable);
   mObjectTypeNumber = FlagTypeNumber;
   setZone(NULL);

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Alternate constructor, currently used by dropping flags in hunterGame
FlagItem::FlagItem(const Point &pos, const Point &vel, bool useDropDelay) : Parent(pos, true, (F32)Ship::CollisionRadius, 4)
{
   initialize();

   setActualVel(vel);
   if(useDropDelay)
      mDroppedTimer.reset();
}


// Destructor
FlagItem::~FlagItem()      
{
   LUAW_DESTRUCTOR_CLEANUP;
}


FlagItem *FlagItem::clone() const
{
   return new FlagItem(*this);
}


void FlagItem::onAddedToGame(Game *theGame)
{ 
   Parent::onAddedToGame(theGame);
   theGame->getGameType()->addFlag(this);    
}


void FlagItem::setZone(GoalZone *goalZone)
{
   if(goalZone)
      goalZone->setHasFlag(true);
   else     // We passed NULL
   {
      GoalZone *zone = getZone();
      if(zone)
         zone->setHasFlag(false);
   }

   // Now we can get around to setting the zone, which is what we came here to do
   mZone = goalZone;
   setMaskBits(ZoneMask);
}


GoalZone *FlagItem::getZone()
{
   return mZone;
}


bool FlagItem::isInZone()
{
   return mZone.isValid();
}


// Methods that really only apply to NexusFlagItems; having them here lets us get rid of a bunch of dynamic_casts
void FlagItem::changeFlagCount(U32 change) { TNLAssert(false, "Should never be called!"); }
U32 FlagItem::getFlagCount()               { return 1; }


bool FlagItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)         // FlagItem <team> <x> <y> {time}
      return false;

   setTeam(atoi(argv[0]));
   
   if(!Parent::processArguments(argc-1, argv+1, game))
      return false;

   S32 time = (argc >= 4) ? atoi(argv[4]) : 0;     // Flag spawn time is possible 4th argument.  Only important in Nexus games for now.

   mInitialPos = getActualPos();                   // Save the starting location of this flag

   // Create a spawn at the flag's location
   FlagSpawn *spawn = new FlagSpawn(mInitialPos, time, getTeam());
   spawn->addToGame(game, game->getGameObjDatabase());

   return true;
}


string FlagItem::toLevelCode(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode(gridSize);
}


U32 FlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ZoneMask))
   {
      if(mZone.isValid())
      {
         S32 index = connection->getGhostIndex(mZone);
         if(stream->writeFlag(index != -1))
            stream->writeInt(index, GhostConnection::GhostIdBitSize);
         else
            retMask |= ZoneMask;
      }
      else
         stream->writeFlag(false);
   }

   if(updateMask & InitialMask)
      writeThisTeam(stream);

   return retMask;
}


void FlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())     // ZoneMask
   {
      bool hasZone = stream->readFlag();
      if(hasZone)
         mZone = (GoalZone *) connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize));
      else
         mZone = NULL;
   }

   if(mInitial)
      readThisTeam(stream);
}


void FlagItem::idle(BfObject::IdleCallPath path)
{
   Parent::idle(path);
}


void FlagItem::mountToShip(Ship *ship)
{
   if(!ship)
      return;

   Parent::mountToShip(ship);

   if(mIsMounted)    // Will be true unless something went wrong in mountToShip
      mIsAtHome = false;
}


void FlagItem::sendHome()
{
   // Now that we have flag spawn points, we'll simply redefine "initial pos" as a random selection of the flag spawn points
   // Everything else should remain as it was

   // First, make a list of valid spawn points -- start with a list of all spawn points, then remove any occupied ones
   Vector<AbstractSpawn *> spawnPoints = getGame()->getGameType()->getSpawnPoints(FlagSpawnTypeNumber, getTeam());
   removeOccupiedSpawnPoints(spawnPoints);


   if(spawnPoints.size() == 0)      // Protect from crash if this happens, which it shouldn't, but has
   {
      TNLAssert(false, "No flag spawn points!");
      logprintf(LogConsumer::LogError, "LEVEL ERROR!! Level %s has no flag spawn points for team %d\n**Please submit this level to the devs!**", 
         ((ServerGame *)getGame())->getCurrentLevelFileName().getString(), getTeam());
   }
   else
   {
      S32 spawnIndex = TNL::Random::readI() % spawnPoints.size();
      mInitialPos = spawnPoints[spawnIndex]->getPos();
   }

   setPosVelAng(mInitialPos, Point(0,0), 0);

   mIsAtHome = true;
   setMaskBits(PositionMask);
   updateExtentInDatabase();
}


// Removes occupied spawns from spawnPoints list
void FlagItem::removeOccupiedSpawnPoints(Vector<AbstractSpawn *> &spawnPoints) // Modifies spawnPoints
{
   bool isTeamGame = getGame()->getGameType()->isTeamGame();

   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   // Now remove the occupied spots from our list of potential spawns
   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));
      if(flag->isAtHome() && (flag->getTeam() <= TEAM_NEUTRAL || flag->getTeam() == getTeam() || !isTeamGame))
      {
         // Need to remove this flag's spawnpoint from the list of potential spawns... it's occupied, after all...
         // Note that if two spawnpoints are on top of one another, this will remove the first, leaving the other
         // on the unoccupied list, unless a second flag at this location removes it from the list on a subsequent pass.
         for(S32 j = 0; j < spawnPoints.size(); j++)
            if(spawnPoints[j]->getPos() == flag->mInitialPos)
            {
               spawnPoints.erase_fast(j);
               break;
            }
      }
   }
}


void FlagItem::renderItem(const Point &pos)
{
   Point offset;

   if(mIsMounted)
      offset.set(15, -15);

   renderFlag(pos + offset, getColor());
}


void FlagItem::renderItemAlpha(const Point &pos, F32 alpha)
{
   // No cloaking for normal flags!
   renderItem(pos);
}


void FlagItem::renderDock()
{
#ifndef ZAP_DEDICATED
   renderFlag(getActualPos(), 0.6, getColor());
#endif
}


F32 FlagItem::getEditorRadius(F32 currentScale)
{
   return 18 * currentScale;
}


const char *FlagItem::getOnScreenName()     { return "Flag";  }
const char *FlagItem::getOnDockName()       { return "Flag";  }
const char *FlagItem::getPrettyNamePlural() { return "Flags"; }
const char *FlagItem::getEditorHelpString() { return "Flag item, used by a variety of game types."; }


bool FlagItem::hasTeam()      { return true; }
bool FlagItem::canBeHostile() { return true; }
bool FlagItem::canBeNeutral() { return true; }


// Runs on both client and server
bool FlagItem::collide(BfObject *hitObject)
{
   // Flag never collides if it is mounted or is set to be not collideable for some reason
   if(mIsMounted || !mIsCollideable)
      return false;

   // Flag always collides with walls and forcefields
   if(isFlagCollideableType(hitObject->getObjectTypeNumber()))
      return true;

   // No other collision detection happens on the client -- From here on out, it's server only!
   if(isClient())
      return false;

   bool isShip = isShipType(hitObject->getObjectTypeNumber());

   // The only things we'll collide with (aside from walls and forcefields above) is ships and robots
   if(!isShip)
      return false;

   Ship *ship = static_cast<Ship*>(hitObject);

   // Ignore collisions that occur to recently dropped flags.  Make sure flag is ready to be picked up! 
   if(mDroppedTimer.getCurrent())    
      return false;

   // We've hit a ship or robot  (remember, robot is a subtype of ship, so this will work for both)
   // We'll need to make sure the ship is a valid entity and that it hasn't exploded
   if(ship->hasExploded)
      return false;

   GameType *gt = getGame()->getGameType();

   if(!gt)     // Something is wrong...
      return false;     
   
   // Finally!
   gt->shipTouchFlag(ship, this);

   return false;
}


void FlagItem::dismount(DismountMode dismountMode)
{
   Ship *ship = mMount;    // mMount will be set to NULL in Parent::dismount() -- grab it while we can
   Parent::dismount(dismountMode);

   // Should getting shot up count as a flag drop event for statistics purposes?
   if(ship && ship->getClientInfo())
      ship->getClientInfo()->getStatistics()->mFlagDrop++;
}


TestFunc FlagItem::collideTypes()
{
   return (TestFunc)isFlagOrShipCollideableType;
}


bool FlagItem::isAtHome()
{
   return mIsAtHome;
}


/////
// Lua interface

/**
  *  @luaclass FlagItem
  *  @brief    Flags are used in many games, such as Nexus and Capture The Flag (CTF).
  *  @geom     The geometry of a %FlagItem is a point.
  */
//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, isInInitLoc,  ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(FlagItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(FlagItem, LUA_METHODS);

#undef LUA_METHODS


const char *FlagItem::luaClassName = "FlagItem";
REGISTER_LUA_SUBCLASS(FlagItem, MountableItem);


/**
  *  @luafunc FlagItem::isInInitLoc()
  *  @brief   Returns true if the flag is in its starting position, false if it has been moved.
  *  @return  \e bool - True if flag is in its starting position, false if it has been moved.
  */
S32 FlagItem::lua_isInInitLoc(lua_State *L)
{ 
   return returnBool(L, isAtHome()); 
}


/**
 *  @luafunc FlagItem::getCaptureZone()
 *  @brief   Returns capture zone holding the %FlagItem, or nil if it is not in a capture zone.
 *  @return  \e GoalZone - The capture zone where the %FlagItem is, or nil if it is not in a capture zone.
 */

// Override parent method
S32 FlagItem::lua_getCaptureZone(lua_State *L)
{
   if(mZone.isValid())
   {
      mZone->push(L);
      return 1;
   }
   else
      return returnNil(L);
}


// Override parent method
S32 FlagItem::lua_isInCaptureZone(lua_State *L) { return returnBool(L, mZone.isValid()); }

};
