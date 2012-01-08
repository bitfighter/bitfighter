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

#include "CoreGame.h"
#include "item.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#include "UIMenuItems.h"
#include "gameObjectRender.h"
#endif


namespace Zap {


CoreGameType::CoreGameType()
{
   setWinningScore(0);   // Winning score hard-coded to 0
}

CoreGameType::~CoreGameType()
{
   // Do nothing
}


bool CoreGameType::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc > 0)
   {
      setGameTime(F32(atof(argv[0]) * 60.0));      // Game time, stored in minutes in level file
      if(argc > 1)
         mCoreItemHitPoints = U32(atoi(argv[1]));  // Hit points of Cores in game
   }

   return true;
}


string CoreGameType::toString() const
{
   return string(getClassName()) + " " + ftos(F32(getTotalGameTime()) / 60 , 3) + " " + itos(mCoreItemHitPoints);
}


// Runs on client
void CoreGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
#ifndef ZAP_DEDICATED
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *ship = dynamic_cast<Ship *>(dynamic_cast<ClientGame *>(getGame())->getConnectionToServer()->getControlObject());

   if(!ship)
      return;

   for(S32 i = 0; i < mCores.size(); i++)
   {
      CoreItem *coreItem = mCores[i];  // Core may have been destroyed
      if(coreItem)
         if(coreItem->getTeam() != ship->getTeam())
            renderObjectiveArrow(coreItem, getTeamColor(coreItem->getTeam()));
   }
#endif
}


#ifndef ZAP_DEDICATED
const char **CoreGameType::getGameParameterMenuKeys()
{
    static const char *items[] = {
      "Level Name",
      "Level Descr",
      "Level Credits",
      "Levelgen Script",
      "Game Time",
      "Core Hit Points",      // <=== defined here
//      "Score to Win",       // There is no score to win in this game mode - it is hardcoded as 0
      "Grid Size",
      "Min Players",
      "Max Players",
      "Allow Engr",
      "Allow Robots",
      "" };

      return items;
}


boost::shared_ptr<MenuItem> CoreGameType::getMenuItem(const char *key)
{
   if(!strcmp(key, "Core Hit Points"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Core Hit Points:", mCoreItemHitPoints, 1, 1, 1000, "", "",
                                                                        "Hit points that each Core has in-game"));
   else return Parent::getMenuItem(key);

}


bool CoreGameType::saveMenuItem(const MenuItem *menuItem, const char *key)
{
   if(!strcmp(key, "Core Hit Points"))
      mCoreItemHitPoints = menuItem->getIntValue();
   else
      return Parent::saveMenuItem(menuItem, key);

   return true;
}
#endif


void CoreGameType::addCore(CoreItem *core, S32 team)
{
   mCores.push_back(core);

   for(S32 i = 0; i < getGame()->getTeamCount(); i++)
   {
      // Make every team that doesn't own this core require another point to win
      if(i != team)
      {
         Team *team = (Team *)getGame()->getTeam(i);
         team->addScore(-DestroyedCoreScore);
      }
   }
}


S32 CoreGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   if(scoreGroup == TeamScore)
   {
      switch(scoreEvent)
      {
         case OwnCoreDestroyed:
            return -data;
         case EnemyCoreDestroyed:
            return data;
         default:
            return naScore;
      }
   }
   else  // scoreGroup == IndividualScore
   {
      //switch(scoreEvent)
      //{
      //   default:
            return naScore;
      //}
   }
}


void CoreGameType::score(Ship *destroyer, S32 team, S32 score)
{
   // If someone destroyed enemy core
   if(destroyer && destroyer->getTeam() != team)
      updateScore(NULL, team, EnemyCoreDestroyed, score);
   else
      updateScore(NULL, team, OwnCoreDestroyed, score);
}


U32 CoreGameType::getCoreItemHitPoints()
{
   return mCoreItemHitPoints;
}


void CoreGameType::setCoreItemHitPoints(U32 hitPoints)
{
   mCoreItemHitPoints = hitPoints;
}


GameTypes CoreGameType::getGameType() const
{
   return CoreGame;
}


const char *CoreGameType::getShortName() const
{
   return "C";
}


const char *CoreGameType::getInstructionString()
{
   return "Destroy all of the opposing team's Cores";
}


bool CoreGameType::canBeTeamGame() const
{
   return true;
}


bool CoreGameType::canBeIndividualGame() const
{
   return false;
}


TNL_IMPLEMENT_NETOBJECT(CoreGameType);


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(CoreItem);
class LuaCore;

// Constructor
CoreItem::CoreItem() : Parent(Point(0,0), F32(CoreStartWidth))
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = CoreTypeNumber;
   mStartingHitPoints = CoreDefaultHitPoints;      // Hits to kill
   mHitPoints = mStartingHitPoints;
   hasExploded = false;

   mKillString = "crashed into a core";    // TODO: Really needed?
}


CoreItem *CoreItem::clone() const
{
   return new CoreItem(*this);
}


void CoreItem::renderItem(const Point &pos)
{
   if(!hasExploded)
      renderCore(pos, calcCoreWidth() / 2, getTeamColor(mTeam));
}


void CoreItem::renderDock()
{
   renderCore(getVert(0), 5, &Colors::white);
}


const char *CoreItem::getEditorHelpString()
{
   return "Core.  Destroy to score.";
}


const char *CoreItem::getPrettyNamePlural()
{
   return "Cores";
}


const char *CoreItem::getOnDockName()
{
   return "Core";
}


const char *CoreItem::getOnScreenName()
{
   return "Core";
}


F32 CoreItem::getEditorRadius(F32 currentScale)
{
   return getRadius() * currentScale;
}


bool CoreItem::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   return false;
}


bool CoreItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   Rect rect = Rect(getActualPos(), calcCoreWidth());
   rect.toPoly(polyPoints);
   return true;
}


void CoreItem::damageObject(DamageInfo *theInfo)
{
   if(hasExploded)
      return;

   mHitPoints--;
   if(mHitPoints == 0)    // Kill small items
   {
      // We've scored!
      GameType *gameType = getGame()->getGameType();
      if(gameType)
      {
         Ship *destroyer = dynamic_cast<Ship *>(theInfo->damagingObject);
         CoreGameType *coreGameType = dynamic_cast<CoreGameType*>(gameType);
         if(coreGameType)
            coreGameType->score(destroyer, getTeam(), CoreGameType::DestroyedCoreScore);
      }

      hasExploded = true;
      deleteObject(500);
      setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...

      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(calcCoreWidth());
}


void CoreItem::setRadius(F32 radius)
{
   Parent::setRadius(radius * getGame()->getGridSize());
}


U32 CoreItem::getStartingHitPoints()
{
   return mStartingHitPoints;
}


void CoreItem::setStartingHitPoints(U32 hitPoints)
{
   mStartingHitPoints = hitPoints;
   mHitPoints = hitPoints;
}


void CoreItem::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   // Make cores always visible
   if(!isGhost())
      setScopeAlways();

   GameType *gameType = theGame->getGameType();

   if(!gameType)                 // Sam has observed this under extreme network packet loss
      return;

   // Adjust hit points to match that set in CoreGameType
   CoreGameType *coreGameType = dynamic_cast<CoreGameType*>(gameType);
   if(coreGameType)
   {
      setStartingHitPoints(coreGameType->getCoreItemHitPoints());

      // Now add to game
      coreGameType->addCore(this, getTeam());
   }
}


U32 CoreItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
      stream->writeInt(mHitPoints, 8);

   stream->writeFlag(hasExploded);

   if(updateMask & InitialMask)
      writeThisTeam(stream);

   return retMask;
}


void CoreItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mHitPoints = stream->readInt(8);
      setRadius(calcCoreWidth());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(getActualPos());
   }

   if(mInitial)
      readThisTeam(stream);
}


bool CoreItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)         // CoreItem <team> <x> <y>
      return false;

   mTeam = atoi(argv[0]);

   if(!Parent::processArguments(argc-1, argv+1, game))
      return false;

   return true;
}


string CoreItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + geomToString(gridSize);
}


F32 CoreItem::calcCoreWidth() const
{
   F32 ratio = F32(mHitPoints) / F32(mStartingHitPoints);

   return
         F32(CoreStartWidth - CoreMinWidth) * ratio / F32(CoreDefaultHitPoints) + CoreMinWidth;
}


bool CoreItem::collide(GameObject *otherObject)
{
   return true;
}


// Client only
void CoreItem::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos, Point());
   // FXManager::emitBurst(pos, Point(.1, .1), Colors::white, Colors::white, 10);
}


const char CoreItem::className[] = "CoreItem";      // Class name as it appears to Lua scripts

// Lua constructor
CoreItem::CoreItem(lua_State *L)
{
   // Do we want to construct these from Lua?  If so, do that here!
}


// Define the methods we will expose to Lua
Lunar<CoreItem>::RegType CoreItem::methods[] =
{
   // Standard gameItem methods
   method(CoreItem, getClassID),
   method(CoreItem, getLoc),
   method(CoreItem, getRad),
   method(CoreItem, getVel),
   method(CoreItem, getTeamIndx),

   // Class specific methods
   method(CoreItem, getHitPoints),

   {0,0}    // End method list
};


S32 CoreItem::getClassID(lua_State *L)
{
   return returnInt(L, CoreTypeNumber);
}


S32 CoreItem::getHitPoints(lua_State *L)
{
   return returnInt(L, mHitPoints);
}


void CoreItem::push(lua_State *L)
{
   Lunar<CoreItem>::push(L, this);
}


} /* namespace Zap */
