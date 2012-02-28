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
   ClientGame *clientGame = dynamic_cast<ClientGame *>(getGame());
   Ship *ship = clientGame && clientGame->getConnectionToServer() ? dynamic_cast<Ship *>(clientGame->getConnectionToServer()->getControlObject()) : NULL;

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


bool CoreGameType::isTeamCoreBeingAttacked(S32 teamIndex)
{
   for(S32 i = 0; i < mCores.size(); i++)
   {
      CoreItem *coreItem = mCores[i];  // Core may have been destroyed
      if(coreItem && coreItem->getTeam() == teamIndex)
         if(coreItem->isBeingAttacked())
            return true;
   }

   return false;
}


#ifndef ZAP_DEDICATED
Vector<string> CoreGameType::getGameParameterMenuKeys()
{
   Vector<string> items = Parent::getGameParameterMenuKeys();

   // Remove "Win Score" as that's not needed here -- win score is determined by the number of cores
   for(S32 i = 0; i < items.size(); i++)
      if(items[i] == "Win Score")
      {
         items.erase(i);
         break;
      }
 
   return items;
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


void CoreGameType::score(ClientInfo *destroyer, S32 coreOwningTeam, S32 score)
{
   Vector<StringTableEntry> e;

   if(destroyer)
   {
      e.push_back(destroyer->getName());
      e.push_back(getGame()->getTeamName(coreOwningTeam));

      // If someone destroyed enemy core
      if(destroyer->getTeamIndex() != coreOwningTeam)
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


GameTypeId CoreGameType::getGameTypeId() const
{
   return CoreGame;
}


const char *CoreGameType::getShortName() const
{
   return "Core";
}


const char *CoreGameType::getInstructionString() const
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

   mHasExploded = false;

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
   if(!mHasExploded)
      renderCore(pos, calcCoreWidth() / 2, getTeamColor(mTeam),
            getGame()->getGameType()->getRemainingGameTimeInMs(), mPanelHealth, mStartingPanelHealth);
#endif
}


void CoreItem::renderDock()
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();
   renderCoreSimple(pos, &Colors::white, 10);
#endif
}


void CoreItem::renderEditor(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();
   renderCoreSimple(pos, getTeamColor(mTeam), CoreStartWidth);
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
   setStartingHealth(F32(attributeMenu->getMenuItem(0)->getIntValue()) / DamageReductionRatio);
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
   center = getPos();
   radius = calcCoreWidth() / 2;
   return true;
}


bool CoreItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   // This poly rotates with time to match what is rendered
   // problem with shooting through moving polygon, and client and server may have different getCurrentTime()
   //F32 coreRotateTime = F32(getGame()->getCurrentTime() & 16383) / 16384.f * FloatTau;
   //Point pos = getActualPos();
   //F32 radius = calcCoreWidth() / 2;

   //for(F32 theta = 0; theta < FloatTau; theta += FloatTau / 10)  // 10 sides
   //{
   //   Point p = Point(pos.x + cos(theta + coreRotateTime) * radius, pos.y + sin(theta + coreRotateTime) * radius);
   //   polyPoints.push_back(p);
   //}

   return false;
}


void CoreItem::damageObject(DamageInfo *theInfo)
{
   if(mHasExploded)
      return;

   if(theInfo->damageAmount == 0)
      return;

   // Special logic for handling the repairing of Core panels
   if(theInfo->damageAmount < 0)
   {
      S32 repairablePanelCount = 0;

      // First determine how many panels have damage and are not destroyed
      for(S32 i = 0; i < CORE_PANELS; i++)
         if(mPanelHealth[i] < mStartingPanelHealth && mPanelHealth[i] > 0)
            repairablePanelCount++;

      // None are repairable, return
      if(repairablePanelCount == 0)
         return;

      // Now divide up the healing to the panels that aren't at full health
      for(S32 i = 0; i < CORE_PANELS; i++)
         if(mPanelHealth[i] < mStartingPanelHealth && mPanelHealth[i] > 0)
         {
            mPanelHealth[i] -= (theInfo->damageAmount / DamageReductionRatio) / repairablePanelCount;

            // Don't overflow
            if(mPanelHealth[i] > mStartingPanelHealth)
               mPanelHealth[i] = mStartingPanelHealth;

            setMaskBits(PanelDamagedMask << i);
         }

      // We're done if we're repairing
      return;
   }

   // Check for friendly fire
   if(theInfo->damagingObject->getTeam() == this->getTeam())
      return;

   //mHealth -= theInfo->damageAmount / DamageReductionRatio / 10;

   //if(mHealth < 0)
   //   mHealth = 0;


   // Which panel was hit?  Look at shot position, compare it to core position
   Point p = getPos();
   Point cp = theInfo->collisionPoint;

   F32 angle = p.angleTo(cp);

   F32 coreAngle = F32(getGame()->getGameType()->getRemainingGameTimeInMs() & 16383) / 16384.f * FloatTau;

   F32 combinedAngle = (angle - coreAngle);

   // Make sure combinedAngle is between 0 and Tau -- sometimes angleTo returns odd values
   while(combinedAngle < 0)
      combinedAngle += FloatTau;
   while(combinedAngle >= FloatTau)
      combinedAngle -= FloatTau;

   static const F32 PANEL_ANGLE = FloatTau / F32(CORE_PANELS);
   S32 hit = (S32) (combinedAngle / PANEL_ANGLE);

   if(mPanelHealth[hit] > 0)
   {
      mPanelHealth[hit] -= theInfo->damageAmount / DamageReductionRatio;

      if(mPanelHealth[hit] < 0)
         mPanelHealth[hit] = 0;

      setMaskBits(PanelDamagedMask << hit);
   }

   // Determine if Core is destroyed by checking all the panel healths
   bool coreDestroyed = false;

   if(mPanelHealth[hit] == 0)
   {
      coreDestroyed = true;
      for(S32 i = 0; i < CORE_PANELS; i++)
         if(mPanelHealth[i] > 0)
         {
            coreDestroyed = false;
            break;
         }
   }

   if(coreDestroyed)
   {
      // We've scored!
      GameType *gameType = getGame()->getGameType();
      if(gameType)
      {
         ClientInfo *destroyer = theInfo->damagingObject->getOwner();

         CoreGameType *coreGameType = dynamic_cast<CoreGameType*>(gameType);
         if(coreGameType)
            coreGameType->score(destroyer, getTeam(), CoreGameType::DestroyedCoreScore);
      }

      mHasExploded = true;
      deleteObject(ExplosionCount * ExplosionInterval);  // Must wait for triggered explosions
      setMaskBits(ExplodedMask);                         
      disableCollision();

      return;
   }

   // Reset the attacked warning timer if we're not healing
   if(theInfo->damageAmount > 0)
      mAttackedWarningTimer.reset(CoreAttackedWarningDuration);

   //setMaskBits(ItemChangedMask);    // So our clients will get new size
   //setRadius(calcCoreWidth());
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

   F32 x = TNL::Random::readF() * xNeg * .71f  * F32(CoreStartWidth) / 2;  // rougly sin(45)
   F32 y = TNL::Random::readF() * yNeg * .71f  * F32(CoreStartWidth) / 2;

   // First explosion is at the center
   Point blastPoint = isStart ? pos : pos + Point(x, y);

   // Also add in secondary sound at start
//   if(isStart)
//      SoundSystem::playSoundEffect(SFXCoreExplodeSecondary, blastPoint);

   SoundSystem::playSoundEffect(SFXCoreExplode, blastPoint, Point(), 1 - 0.25f * F32(mCurrentExplosionNumber));

   game->emitBlast(blastPoint, 600 - 100 * mCurrentExplosionNumber);
   game->emitExplosion(blastPoint, 4.f - F32(mCurrentExplosionNumber), CoreExplosionColors, 12);

#endif
   mCurrentExplosionNumber++;
}


void CoreItem::idle(GameObject::IdleCallPath path)
{
   // Update attack timer on the server
   if(path == GameObject::ServerIdleMainLoop)
   {
      // If timer runs out, then set this Core as having a changed state so the client
      // knows it isn't being attacked anymore
      if(mAttackedWarningTimer.update(mCurrentMove.time))
         setMaskBits(ItemChangedMask);
   }

   // Only run the following on the client
   if(path != GameObject::ClientIdleMainRemote)
      return;

   // Update Explosion Timer
   if(mHasExploded)
   {
      if(mExplosionTimer.getCurrent() != 0)
         mExplosionTimer.update(mCurrentMove.time);
      else
         if(mCurrentExplosionNumber < ExplosionCount)
         {
            doExplosion(getPos());
            mExplosionTimer.reset(ExplosionInterval);
         }
   }

   if(mHeartbeatTimer.getCurrent() != 0)
      mHeartbeatTimer.update(mCurrentMove.time);
   else
   {
      // Thump thump
      SoundSystem::playSoundEffect(SFXCoreHeartbeat, getPos());

      // Now reset the timer as a function of health
      // Exponential
      F32 health = getHealth();
      U32 soundInterval = CoreHeartbeatMinInterval + U32(F32(CoreHeartbeatStartInterval - CoreHeartbeatMinInterval) * health * health);

      mHeartbeatTimer.reset(soundInterval);
   }
}


void CoreItem::setStartingHealth(F32 health)
{
   mStartingHealth = health;

   // Now that starting health has been set, divide it amongst the panels
   mStartingPanelHealth = mStartingHealth / CORE_PANELS;
   
   // Core's total health is divided evenly amongst its panels
   for(S32 i = 0; i < 10; i++)
      mPanelHealth[i] = mStartingPanelHealth;
}


F32 CoreItem::getTotalHealth()
{
   F32 total = 0;

   for(S32 i = 0; i < CORE_PANELS; i++)
      total += mPanelHealth[i];

   return total;
}


F32 CoreItem::getHealth()
{
   // health is from 0 to 1.0
   return getTotalHealth() / mStartingHealth;
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


// compatible with readFloat at the same number of bits
static void writeFloatZeroOrNonZero(BitStream &s, F32 &val, U8 bitCount)
{
   TNLAssert(val >= 0 && val <= 1, "writeFloat Must be between 0.0 and 1.0");
   if(val == 0)
      s.writeInt(0, bitCount);  // always writes zero
   else
      s.writeInt(U32(val * ((1 << bitCount) - 2) + 0.5f) + 1, bitCount);  // never writes zero
}


U32 CoreItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(updateMask & InitialMask)
   {
      writeThisTeam(stream);
   }

   stream->writeFlag(mHasExploded);

   if(!mHasExploded)
   {
      // Don't bother with health report if we've exploded
      for(S32 i = 0; i < CORE_PANELS; i++)
      {
         if(stream->writeFlag(updateMask & (PanelDamagedMask << i))) // go through each bit mask
         {
            // Normalize between 0.0 and 1.0 for transmission
            F32 panelHealthRatio = mPanelHealth[i] / mStartingPanelHealth;

            // writeFloatZeroOrNonZero will Compensate for low resolution by sending zero only if it is actually zero
            writeFloatZeroOrNonZero(*stream, panelHealthRatio, 4);     // 4 bits -> 1/16 increments, all we really need
         }
      }
   }

   stream->writeFlag(mAttackedWarningTimer.getCurrent());

   return retMask;
}



void CoreItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(mInitial)
   {
      readThisTeam(stream);
   }

   if(stream->readFlag())     // Exploding!  Take cover!!
   {
      for(S32 i = 0; i < CORE_PANELS; i++)
         mPanelHealth[i] = 0;

      if(!mHasExploded)    // Just exploded!
      {
         mHasExploded = true;
         disableCollision();
         onItemExploded(getPos());
      }
   }
   else                             // Haven't exploded, getting health
   {
      for(S32 i = 0; i < CORE_PANELS; i++)
      {
         if(stream->readFlag())                    // Panel damaged
         {
            // De-normalize to real health
            bool hadHealth = mPanelHealth[i] > 0;
            mPanelHealth[i] = mStartingPanelHealth * stream->readFloat(4);

            // Check if panel just died
            if(hadHealth && mPanelHealth[i] == 0)  
               emitPanelDiedSparks(getGame(), getPos(), getGame()->getGameType()->getRemainingGameTimeInMs(), i);
         }
      }
   }

   mBeingAttacked = stream->readFlag();
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


bool CoreItem::isBeingAttacked()
{
   return mBeingAttacked;
}


F32 CoreItem::calcCoreWidth() const
{
   return CoreStartWidth; //(F32(CoreStartWidth - CoreMinWidth) * mHealth) + CoreMinWidth;
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
   method(CoreItem, getHealth),

   {0,0}    // End method list
};


S32 CoreItem::getClassID(lua_State *L)
{
   return returnInt(L, CoreTypeNumber);
}


S32 CoreItem::getHealth(lua_State *L)
{
   return returnFloat(L, getTotalHealth());
}


void CoreItem::push(lua_State *L)
{
   Lunar<CoreItem>::push(L, this);
}


} /* namespace Zap */
