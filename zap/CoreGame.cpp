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
#include "UIEditorMenus.h"
#include "UI.h"
#include "gameObjectRender.h"
#endif

#include <cmath>

namespace Zap {


CoreGameType::CoreGameType() : GameType(0)  // Winning score hard-coded to 0
{
   // Do nothing
}

CoreGameType::~CoreGameType()
{
   // Do nothing
}


bool CoreGameType::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc > 0)
      setGameTime(F32(atof(argv[0]) * 60.0));      // Game time, stored in minutes in level file

   return true;
}


string CoreGameType::toString() const
{
   return string(getClassName()) + " " + ftos(F32(getTotalGameTime()) / 60 , 3);
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


S32 CoreGameType::getTeamCoreCount(S32 teamIndex)
{
   S32 count = 0;

   for(S32 i = 0; i < mCores.size(); i++)
   {
      CoreItem *coreItem = mCores[i];  // Core may have been destroyed
      if(coreItem && coreItem->getTeam() == teamIndex)
         count++;
   }

   return count;
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
   return Parent::getMenuItem(key);
}


bool CoreGameType::saveMenuItem(const MenuItem *menuItem, const char *key)
{
   return Parent::saveMenuItem(menuItem, key);
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
         case KillEnemy:
            return 0;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return 0;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
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
      switch(scoreEvent)
      {
         case KillEnemy:
            return 1;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return -1;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 1;
         case KillOwnTurret:
            return -1;
         case OwnCoreDestroyed:
            return -5 * data;
         case EnemyCoreDestroyed:
            return 5 * data;
         default:
            return naScore;
      }
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

// Statics:
#ifndef ZAP_DEDICATED
EditorAttributeMenuUI *CoreItem::mAttributeMenuUI = NULL;
#endif

// Ratio at which damage is reduced so that Core Health can fit between 0 and 1.0
// for easier bit transmission
const F32 CoreItem::DamageReductionRatio = 1000.0f;


// Constructor
CoreItem::CoreItem() : Parent(Point(0,0), F32(CoreStartWidth))
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = CoreTypeNumber;
   setStartingHealth(F32(CoreDefaultStartingHealth) / DamageReductionRatio);      // Hits to kill
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
      renderCore(pos, calcCoreWidth() / 2, getTeamColor(mTeam), getGame()->getCurrentTime());
}


void CoreItem::renderDock()
{
   renderCore(getVert(0), 5, &Colors::white, getGame()->getCurrentTime());
}


#ifndef ZAP_DEDICATED

EditorAttributeMenuUI *CoreItem::getAttributeMenu()
{
   // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
   if(!mAttributeMenuUI)
   {
      ClientGame *clientGame = static_cast<ClientGame *>(getGame());

      mAttributeMenuUI = new EditorAttributeMenuUI(clientGame);

      mAttributeMenuUI->addMenuItem(new CounterMenuItem("Hit points:", CoreDefaultStartingHealth,
            10, 1, S32(DamageReductionRatio), "", "", ""));

      // Add our standard save and exit option to the menu
      mAttributeMenuUI->addSaveAndQuitMenuItem();
   }

   return mAttributeMenuUI;
}


// Get the menu looking like what we want
void CoreItem::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   attributeMenu->getMenuItem(0)->setIntValue(S32(mStartingHealth * DamageReductionRatio));
}


// Retrieve the values we need from the menu
void CoreItem::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   setStartingHealth(F32(attributeMenu->getMenuItem(0)->getIntValue()) / F32(DamageReductionRatio));
}


// Render some attributes when item is selected but not being edited
string CoreItem::getAttributeString()
{
   return "Health: " + itos(S32(mStartingHealth * DamageReductionRatio));
}

#endif


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
   return (calcCoreWidth() / 2) * currentScale + 5;
}


bool CoreItem::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
//   center = getActualPos();
//   radius = calcCoreWidth() / 2;
   return false;
}


bool CoreItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   // This poly rotates with time to match what is rendered
   F32 coreRotateTime = F32(getGame()->getCurrentTime() & 16383) / 16384.f * FloatTau;
   Point pos = getActualPos();
   F32 radius = calcCoreWidth() / 2;

   for(F32 theta = 0; theta < FloatTau; theta += FloatTau / 10)  // 10 sides
   {
      Point p = Point(pos.x + cos(theta + coreRotateTime) * radius, pos.y + sin(theta + coreRotateTime) * radius);
      polyPoints.push_back(p);
   }

   return true;
}


void CoreItem::damageObject(DamageInfo *theInfo)
{
   if(hasExploded)
      return;

   mHealth -= theInfo->damageAmount / DamageReductionRatio;

   if(mHealth < 0)
      mHealth = 0;

   if(mHealth == 0)
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
      disableCollision();

      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(calcCoreWidth());
}


void CoreItem::setRadius(F32 radius)
{
   Parent::setRadius(radius * getGame()->getGridSize());
}


void CoreItem::setStartingHealth(F32 health)
{
   mStartingHealth = health;
   mHealth = health;
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

   // Now add to game
   CoreGameType *coreGameType = dynamic_cast<CoreGameType*>(gameType);
   if(coreGameType)
      coreGameType->addCore(this, getTeam());
}


U32 CoreItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
      stream->writeFloat(mHealth, 16);  // 16 bits -> 1/65536 increments

   stream->writeFlag(hasExploded);

   if(updateMask & InitialMask)
   {
      writeThisTeam(stream);
      stream->writeFloat(mStartingHealth, 16);
   }

   return retMask;
}


void CoreItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mHealth = stream->readFloat(16);
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
   {
      readThisTeam(stream);
      mStartingHealth = stream->readFloat(16);
   }
}


bool CoreItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 4)         // CoreItem <team> <health> <x> <y>
      return false;

   mTeam = atoi(argv[0]);
   setStartingHealth(F32(atoi(argv[1])) / DamageReductionRatio);

   if(!Parent::processArguments(argc-2, argv+2, game))
      return false;

   return true;
}


string CoreItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + itos(S32(mStartingHealth * DamageReductionRatio)) + " " + geomToString(gridSize);
}


F32 CoreItem::calcCoreWidth() const
{
   F32 ratio = mHealth / mStartingHealth;

   return (F32(CoreStartWidth - CoreMinWidth) * ratio) + CoreMinWidth;
}


bool CoreItem::collide(GameObject *otherObject)
{
   return true;
}


// Client only
void CoreItem::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXShipExplode, pos, Point());

   TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
   ClientGame *game = static_cast<ClientGame *>(getGame());

   Color teamColor = getTeamColor(mTeam);

   Color CoreExplosionColors[12] = {
      Colors::red,
      teamColor,
      Colors::white,
      teamColor,
      Colors::blue,
      teamColor,
      Colors::white,
      teamColor,
      Colors::yellow,
      teamColor,
      Colors::white,
      teamColor,
   };

   game->emitBlast(pos, 1200);
   game->emitExplosion(pos, 4.f, CoreExplosionColors, 12);
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
   method(CoreItem, getCurrentHitPoints),

   {0,0}    // End method list
};


S32 CoreItem::getClassID(lua_State *L)
{
   return returnInt(L, CoreTypeNumber);
}


S32 CoreItem::getCurrentHitPoints(lua_State *L)
{
   return returnFloat(L, mHealth);
}


void CoreItem::push(lua_State *L)
{
   Lunar<CoreItem>::push(L, this);
}


} /* namespace Zap */
