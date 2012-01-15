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
#include "projectile.h"
#include "game.h"
#include "stringUtils.h"

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
         Team *team = dynamic_cast<Team *>(getGame()->getTeam(i));
         if(team)
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
         case OwnCoreDestroyed:   // Scores are adjusted the same for all Core-destroyed events
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


void CoreGameType::score(Ship *destroyer, S32 coreOwningTeam, S32 score)
{
   Vector<StringTableEntry> e;

   if(destroyer)
   {
      e.push_back(destroyer->getClientInfo()->getName());
      e.push_back(getGame()->getTeamName(coreOwningTeam));

      // If someone destroyed enemy core
      if(destroyer->getTeam() != coreOwningTeam)
      {
         static StringTableEntry capString("%e0 destroyed a %e1 Core!");
         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

         updateScore(NULL, coreOwningTeam, EnemyCoreDestroyed, score);
      }
      else
      {
         static StringTableEntry capString("%e0 destroyed own %e1 Core!");
         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

         updateScore(NULL, coreOwningTeam, OwnCoreDestroyed, score);
      }
   }
   else
   {
      e.push_back(getGame()->getTeamName(coreOwningTeam));

      static StringTableEntry capString("Something destroyed a %e0 Core!");
      broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

      updateScore(NULL, coreOwningTeam, EnemyCoreDestroyed, score);
   }
}


GameTypes CoreGameType::getGameType() const
{
   return CoreGame;
}


const char *CoreGameType::getShortName() const
{
   return "Core";
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

U32 CoreItem::mCurrentExplosionNumber = 0;  // zero indexed

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

   mHeartbeatTimer.reset(CoreHeartbeatStartInterval);

   mKillString = "crashed into a core";    // TODO: Really needed?
}


CoreItem *CoreItem::clone() const
{
   return new CoreItem(*this);
}


void CoreItem::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   if(!hasExploded)
      renderCore(pos, calcCoreWidth() / 2, getTeamColor(mTeam), getGame()->getCurrentTime());
#endif
}


void CoreItem::renderDock()
{
#ifndef ZAP_DEDICATED
   renderCore(getVert(0), 5, &Colors::white, getGame()->getCurrentTime());
#endif
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
            1, 1, S32(DamageReductionRatio), "", "", ""));

      // Add our standard save and exit option to the menu
      mAttributeMenuUI->addSaveAndQuitMenuItem();
   }

   return mAttributeMenuUI;
}


// Get the menu looking like what we want
void CoreItem::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   attributeMenu->getMenuItem(0)->setIntValue(S32(mStartingHealth * DamageReductionRatio + 0.5));
}


// Retrieve the values we need from the menu
void CoreItem::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   setStartingHealth(F32(attributeMenu->getMenuItem(0)->getIntValue()) / F32(DamageReductionRatio));
}


// Render some attributes when item is selected but not being edited
string CoreItem::getAttributeString()
{
   return "Health: " + itos(S32(mStartingHealth * DamageReductionRatio + 0.5));
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
         Projectile *p = dynamic_cast<Projectile *>(theInfo->damagingObject);  // What about GrenadeProjectile and Mines?
         Ship *destroyer = p ? dynamic_cast<Ship *>(p->mShooter.getPointer()) : NULL;

         CoreGameType *coreGameType = dynamic_cast<CoreGameType*>(gameType);
         if(coreGameType)
            coreGameType->score(destroyer, getTeam(), CoreGameType::DestroyedCoreScore);
      }

      hasExploded = true;
      deleteObject(ExplosionCount * ExplosionInterval);   // Must wait for triggered explosions
      setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...
      disableCollision();

      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(calcCoreWidth());
}


void CoreItem::doExplosion(const Point &pos)
{
#ifndef ZAP_DEDICATED
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

   bool isStart = mCurrentExplosionNumber == 0;

   S32 xNeg = TNL::Random::readB() ? 1 : -1;
   S32 yNeg = TNL::Random::readB() ? 1 : -1;

   F32 x = TNL::Random::readF() * xNeg * .71  * F32(CoreStartWidth) / 2;  // rougly sin(45)
   F32 y = TNL::Random::readF() * yNeg * .71  * F32(CoreStartWidth) / 2;

   // First explosion is at the center
   Point blastPoint = isStart ? pos : pos + Point(x, y);

   // Also add in secondary sound at start
//   if(isStart)
//      SoundSystem::playSoundEffect(SFXCoreExplodeSecondary, blastPoint, Point());

   SoundSystem::playSoundEffect(SFXCoreExplode, blastPoint, Point(), 1.0 - 0.25 * F32(mCurrentExplosionNumber));

   game->emitBlast(blastPoint, 600 - 100 * F32(mCurrentExplosionNumber));
   game->emitExplosion(blastPoint, 4.f - 1 * F32(mCurrentExplosionNumber), CoreExplosionColors, 12);

#endif
   mCurrentExplosionNumber++;
}


void CoreItem::idle(GameObject::IdleCallPath path)
{
   // Only run the following on the client
   if(path != GameObject::ClientIdleMainRemote)
      return;

   // Update Explosion Timer
   if(hasExploded)
   {
      if(mExplosionTimer.getCurrent() != 0)
         mExplosionTimer.update(mCurrentMove.time);
      else
         if(mCurrentExplosionNumber < ExplosionCount)
         {
            doExplosion(getActualPos());
            mExplosionTimer.reset(ExplosionInterval);
         }
   }

   if(mHeartbeatTimer.getCurrent() != 0)
      mHeartbeatTimer.update(mCurrentMove.time);
   else
   {
      // Thump thump
      SoundSystem::playSoundEffect(SFXCoreHeartbeat, getActualPos(), Point());

      // Now reset the timer as a function of health
      // Exponential
      F32 ratio = mHealth / mStartingHealth;
      U32 soundInterval = CoreHeartbeatMinInterval + F32(CoreHeartbeatStartInterval - CoreHeartbeatMinInterval) * pow(ratio, 2.f);

      mHeartbeatTimer.reset(soundInterval);
   }
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
   setStartingHealth(atof(argv[1]) / DamageReductionRatio);

   if(!Parent::processArguments(argc-2, argv+2, game))
      return false;

   return true;
}


string CoreItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + ftos(mStartingHealth * DamageReductionRatio) + " " + geomToString(gridSize);
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
   mCurrentExplosionNumber = 0;
   mExplosionTimer.reset(ExplosionInterval);

   // Start with an explosion at the center.  See idle() for other called explosions
   doExplosion(pos);
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
