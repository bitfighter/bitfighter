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
#include "stringUtils.h"
#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIEditorMenus.h"
#  include "UI.h"
#  include "gameObjectRender.h"
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
   return string(getClassName()) + " " + mGameTimer.toString_minutes();
}


// Runs on client
void CoreGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
#ifndef ZAP_DEDICATED
   Parent::renderInterfaceOverlay(scoreboardVisible);

   BfObject *object = static_cast<ClientGame *>(getGame())->getConnectionToServer()->getControlObject();

   if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return;

   Ship *ship = static_cast<Ship *>(object);

   for(S32 i = mCores.size() - 1; i >= 0; i--)
   {
      CoreItem *coreItem = mCores[i];
      if(!coreItem)  // Core may have been destroyed
         mCores.erase(i);
      else
         if(coreItem->getTeam() != ship->getTeam())
            renderObjectiveArrow(coreItem, coreItem->getColor());
   }
#endif
}


bool CoreGameType::isTeamCoreBeingAttacked(S32 teamIndex)
{
   for(S32 i = mCores.size() - 1; i >= 0; i--)
   {
      CoreItem *coreItem = mCores[i];
      if(!coreItem)  // Core may have been destroyed
         mCores.erase(i);
      else if(coreItem->getTeam() == teamIndex)
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

   if(U32(team) < U32(getGame()->getTeamCount()))
      static_cast<Team *>(getGame()->getTeam(team))->addScore(1);
}

void CoreGameType::updateScore(ClientInfo *player, S32 team, ScoringEvent event, S32 data)
{
   if(isGameOver()) // Game play ended, no changing score
      return;

   if(player != NULL)  // Individual scores is only for game reports statistics not seen during game play
   {
      S32 points = getEventScore(IndividualScore, event, data);
      TNLAssert(points != naScore, "Bad score value");
      player->addScore(points);
   }

   if((event == OwnCoreDestroyed || event == EnemyCoreDestroyed) && U32(team) < U32(getGame()->getTeamCount()))
   {
      ((Team *)getGame()->getTeam(team))->addScore(-1); // Count down when a core is destoryed
      S32 numberOfTeamsHaveSomeCores = 0;
      s2cSetTeamScore(team, ((Team *)(getGame()->getTeam(team)))->getScore());     // Broadcast result
      for(S32 i = 0; i < getGame()->getTeamCount(); i++)
      {
         if(((Team *)getGame()->getTeam(i))->getScore() != 0)
            numberOfTeamsHaveSomeCores++;
      }
      if(numberOfTeamsHaveSomeCores <= 1)
         gameOverManGameOver();
   }
}

S32 CoreGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   if(scoreGroup == TeamScore)
   {
      return naScore;  // We never use TeamScore in CoreGameType
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


GameTypeId CoreGameType::getGameTypeId() const { return CoreGame; }

const char *CoreGameType::getShortName()         const { return "Core"; }
const char *CoreGameType::getInstructionString() const { return "Destroy all the opposing team's Cores"; }


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

const F32 CoreItem::PANEL_ANGLE = FloatTau / (F32) CORE_PANELS;

// Constructor
CoreItem::CoreItem() : Parent(Point(0,0), F32(CoreRadius * 2))
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = CoreTypeNumber;
   setStartingHealth(F32(CoreDefaultStartingHealth) / DamageReductionRatio);      // Hits to kill

   mHasExploded = false;

   mHeartbeatTimer.reset(CoreHeartbeatStartInterval);
   mCurrentExplosionNumber = 0;

   mPanelGeom.isValid = false;

   mKillString = "crashed into a core";    // TODO: Really needed?

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
CoreItem::~CoreItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


CoreItem *CoreItem::clone() const
{
   return new CoreItem(*this);
}


F32 CoreItem::getCoreAngle(U32 time)
{
   return F32(time & 16383) / 16384.f * FloatTau;
}


void CoreItem::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   if(!mHasExploded)
   {
      GameType *gameType = getGame()->getGameType();

      S32 time = gameType->getRemainingGameTimeInMs() + gameType->getRenderingOffset();

      renderCore(pos, getColor(), time, getPanelGeom(), mPanelHealth, mStartingPanelHealth);
   }
#endif
}


void CoreItem::renderDock()
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();
   renderCoreSimple(pos, &Colors::white, 10);
#endif
}


void CoreItem::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();
   renderCoreSimple(pos, getColor(), CoreRadius * 2);
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


const char *CoreItem::getOnScreenName()     { return "Core";  }
const char *CoreItem::getOnDockName()       { return "Core";  }
const char *CoreItem::getPrettyNamePlural() { return "Cores"; }
const char *CoreItem::getEditorHelpString() { return "Core.  Destroy to score."; }


F32 CoreItem::getEditorRadius(F32 currentScale)
{
   return CoreRadius * currentScale + 5;
}


bool CoreItem::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   center = getPos();
   radius = CoreRadius;
   return true;
}


bool CoreItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}


bool CoreItem::isPanelDamaged(S32 panelIndex)
{
   return mPanelHealth[panelIndex] < mStartingPanelHealth && mPanelHealth[panelIndex] > 0;
}


bool CoreItem::isPanelInRepairRange(const Point &origin, S32 panelIndex)
{
   PanelGeom *panelGeom = getPanelGeom();

   F32 distanceSq1 = (panelGeom->getStart(panelIndex)).distSquared(origin);
   F32 distanceSq2 = (panelGeom->getEnd(panelIndex)).distSquared(origin);
   S32 radiusSq = Ship::RepairRadius * Ship::RepairRadius;

   // Ignoring case where center is in range while endpoints are not...
   return (distanceSq1 < radiusSq) || (distanceSq2 < radiusSq);
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
      // Heal each damaged core if it is in range
      for(S32 i = 0; i < CORE_PANELS; i++)
         if(isPanelDamaged(i))
            if(isPanelInRepairRange(theInfo->damagingObject->getPos(), i))
            {
               mPanelHealth[i] -= theInfo->damageAmount / DamageReductionRatio;

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

   // Which panel was hit?  Look at shot position, compare it to core position
   F32 shotAngle;
   Point p = getPos();

   // Determine angle for Point projectiles like Phaser
   if(theInfo->damageType == DamageTypePoint)
      shotAngle = p.angleTo(theInfo->collisionPoint);

   // Area projectiles
   else
      shotAngle = p.angleTo(theInfo->damagingObject->getPos());


   PanelGeom *panelGeom = getPanelGeom();
   F32 coreAngle = panelGeom->angle;

   F32 combinedAngle = (shotAngle - coreAngle);

   // Make sure combinedAngle is between 0 and Tau -- sometimes angleTo returns odd values
   while(combinedAngle < 0)
      combinedAngle += FloatTau;
   while(combinedAngle >= FloatTau)
      combinedAngle -= FloatTau;

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
         if(gameType->getGameTypeId() == CoreGame)
            static_cast<CoreGameType*>(gameType)->score(destroyer, getTeam(), CoreGameType::DestroyedCoreScore);
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
}


#ifndef ZAP_DEDICATED
void CoreItem::doExplosion(const Point &pos)
{
   ClientGame *game = static_cast<ClientGame *>(getGame());

   Color teamColor = getColor();
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

   F32 x = TNL::Random::readF() * xNeg * FloatSqrtHalf * CoreRadius;  // exactly sin(45)
   F32 y = TNL::Random::readF() * yNeg * FloatSqrtHalf * CoreRadius;

   // First explosion is at the center
   Point blastPoint = isStart ? pos : pos + Point(x, y);

   // Also add in secondary sound at start
//   if(isStart)
//      SoundSystem::playSoundEffect(SFXCoreExplodeSecondary, blastPoint);

   SoundSystem::playSoundEffect(SFXCoreExplode, blastPoint, Point(), 1 - 0.25f * F32(mCurrentExplosionNumber));

   game->emitBlast(blastPoint, 600 - 100 * mCurrentExplosionNumber);
   game->emitExplosion(blastPoint, 4.f - F32(mCurrentExplosionNumber), CoreExplosionColors, ARRAYSIZE(CoreExplosionColors));

   mCurrentExplosionNumber++;
}
#endif


PanelGeom *CoreItem::getPanelGeom()
{
   if(!mPanelGeom.isValid)
      fillPanelGeom(getPos(), getGame()->getGameType()->getRemainingGameTimeInMs() + getGame()->getGameType()->getRenderingOffset(), mPanelGeom);

   return &mPanelGeom;
}


// static method
void CoreItem::fillPanelGeom(const Point &pos, S32 time, PanelGeom &panelGeom)
{
   F32 size = CoreRadius;

   F32 angle = getCoreAngle(time);
   panelGeom.angle = angle;

   F32 angles[CORE_PANELS];

   for(S32 i = 0; i < CORE_PANELS; i++)
      angles[i] = i * PANEL_ANGLE + angle;

   for(S32 i = 0; i < CORE_PANELS; i++)
      panelGeom.vert[i].set(pos.x + cos(angles[i]) * size, pos.y + sin(angles[i]) * size);

   Point start, end, mid;
   for(S32 i = 0; i < CORE_PANELS; i++)
   {
      start = panelGeom.vert[i];
      end   = panelGeom.vert[(i + 1) % CORE_PANELS];      // Next point, with wrap-around
      mid   = (start + end) * .5;

      panelGeom.mid[i].set(mid);
      panelGeom.repair[i].interp(.6f, mid, pos);
   }

   panelGeom.isValid = true;
}

#ifndef ZAP_DEDICATED

void CoreItem::doPanelDebris(S32 panelIndex)
{
   ClientGame *game = static_cast<ClientGame *>(getGame());

   Point pos = getPos();               // Center of core

   PanelGeom *panelGeom = getPanelGeom();
   
   Point dir = panelGeom->mid[panelIndex] - pos;   // Line extending from the center of the core towards the center of the panel
   dir.normalize(100);
   Point cross(dir.y, -dir.x);         // Line parallel to the panel, perpendicular to dir

   // Debris line is relative to (0,0)
   Vector<Point> points;
   points.push_back(Point(0, 0));
   points.push_back(Point(0, 0));      // Dummy point will be replaced below

   // Draw debris for the panel
   S32 num = Random::readI(5, 15);
   const Color *teamColor = getColor();

   Point chunkPos, chunkVel;           // Reusable containers

   for(S32 i = 0; i < num; i++)
   {
      static const S32 MAX_CHUNK_LENGTH = 10;
      points[1].set(0, Random::readF() * MAX_CHUNK_LENGTH);

      chunkPos = panelGeom->getStart(panelIndex) + (panelGeom->getEnd(panelIndex) - panelGeom->getStart(panelIndex)) * Random::readF();
      chunkVel = dir * (Random::readF() * 10  - 3) * .2f + cross * (Random::readF() * 30  - 15) * .05f;

      S32 ttl = Random::readI(2500, 3000);
      F32 startAngle = Random::readF() * FloatTau;
      F32 rotationRate = Random::readF() * 4 - 2;

      // Every-other chunk is team color instead of panel color
      Color chunkColor = i % 2 == 0 ? Colors::gray80 : *teamColor;

      game->emitDebrisChunk(points, chunkColor, chunkPos, chunkVel, ttl, startAngle, rotationRate);
   }


   // Draw debris for the panel health 'stake'
   num = Random::readI(5, 15);
   for(S32 i = 0; i < num; i++)
   {
      points.erase(1);
      points.push_back(Point(0, Random::readF() * 10));

      Point sparkVel = cross * (Random::readF() * 20  - 10) * .05f + dir * (Random::readF() * 2  - .5f) * .2f;
      S32 ttl = Random::readI(2500, 3000);
      F32 angle = Random::readF() * FloatTau;
      F32 rotation = Random::readF() * 4 - 2;

      game->emitDebrisChunk(points, Colors::gray20, (panelGeom->mid[i] + pos) / 2, sparkVel, ttl, angle, rotation);
   }

   // And do the sound effect
   SoundSystem::playSoundEffect(SFXCorePanelExplode, panelGeom->mid[panelIndex]);
}

#endif


void CoreItem::idle(BfObject::IdleCallPath path)
{
   mPanelGeom.isValid = false;      // Force recalculation of panel geometry next time it's needed

   // Update attack timer on the server
   if(path == BfObject::ServerIdleMainLoop)
   {
      // If timer runs out, then set this Core as having a changed state so the client
      // knows it isn't being attacked anymore
      if(mAttackedWarningTimer.update(mCurrentMove.time))
         setMaskBits(ItemChangedMask);
   }

#ifndef ZAP_DEDICATED
   // Only run the following on the client
   if(path != BfObject::ClientIdleMainRemote)
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

   // Emit some sparks from dead panels
   if(Platform::getRealMilliseconds() % 100 < 20)  // 20% of the time...
   {
      Point cross, dir;


      for(S32 i = 0; i < CORE_PANELS; i++)
      {
         if(mPanelHealth[i] == 0)                  // Panel is dead     TODO: And if panel is close enough to be worth it
         {
            Point sparkEmissionPos = getPos();
            sparkEmissionPos += dir * 3;

            Point dir = getPanelGeom()->mid[i] - getPos();  // Line extending from the center of the core towards the center of the panel
            dir.normalize(100);
            Point cross(dir.y, -dir.x);                     // Line parallel to the panel, perpendicular to dir

            Point vel = dir * (Random::readF() * 3 + 2) + cross * (Random::readF() - .2f);
            U32 ttl = Random::readI(0, 1000) + 500;

            static_cast<ClientGame *>(getGame())->emitSpark(sparkEmissionPos, vel, Colors::gray20, ttl);
         }
      }
   }
#endif
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


Vector<Point> CoreItem::getRepairLocations(const Point &repairOrigin)
{
   Vector<Point> repairLocations;

   PanelGeom *panelGeom = getPanelGeom();

   for(S32 i = 0; i < CORE_PANELS; i++)
      if(isPanelDamaged(i))
         if(isPanelInRepairRange(repairOrigin, i))
            repairLocations.push_back(panelGeom->repair[i]);

   return repairLocations;
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
   if(gameType->getGameTypeId() == CoreGame)
      static_cast<CoreGameType*>(gameType)->addCore(this, getTeam());
}


// Compatible with readFloat at the same number of bits
static void writeFloatZeroOrNonZero(BitStream &s, F32 &val, U8 bitCount)
{
   TNLAssert(val >= 0 && val <= 1, "writeFloat Must be between 0.0 and 1.0");
   if(val == 0)
      s.writeInt(0, bitCount);  // always writes zero
   else
   {
      U32 transmissionValue = U32(val * ((1 << bitCount) - 1));  // rounds down

      // If we're not truly at zero, don't send '0', send '1'
      if(transmissionValue == 0)
         s.writeInt(1, bitCount);
      else
         s.writeInt(transmissionValue, bitCount);
   }
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

            // writeFloatZeroOrNonZero will compensate for low resolution by sending zero only if it is actually zero
            // 4 bits -> 1/16 increments, all we really need - this means that client-side
            // will NOT have the true health, rather a ratio of precision 4 bits
            writeFloatZeroOrNonZero(*stream, panelHealthRatio, 4);
         }
      }
   }

   stream->writeFlag(mAttackedWarningTimer.getCurrent());

   return retMask;
}


#ifndef ZAP_DEDICATED

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
               doPanelDebris(i);
         }
      }
   }

   mBeingAttacked = stream->readFlag();
}

#endif


bool CoreItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 4)         // CoreItem <team> <health> <x> <y>
      return false;

   setTeam(atoi(argv[0]));
   setStartingHealth((F32)atof(argv[1]) / DamageReductionRatio);

   if(!Parent::processArguments(argc-2, argv+2, game))
      return false;

   return true;
}


string CoreItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(getTeam()) + " " + ftos(mStartingHealth * DamageReductionRatio) + " " + geomToString(gridSize);
}


bool CoreItem::isBeingAttacked()
{
   return mBeingAttacked;
}


bool CoreItem::collide(BfObject *otherObject)
{
   return true;
}


#ifndef ZAP_DEDICATED
// Client only
void CoreItem::onItemExploded(Point pos)
{
   mCurrentExplosionNumber = 0;
   mExplosionTimer.reset(ExplosionInterval);

   // Start with an explosion at the center.  See idle() for other called explosions
   doExplosion(pos);
}

void CoreItem::onItemDragging()
{
   GameType *gameType = getGame()->getGameType();
   fillPanelGeom(getPos(), gameType->getRemainingGameTimeInMs() + gameType->getRenderingOffset(), mPanelGeom);
}
#endif


bool CoreItem::canBeHostile() { return false; }
bool CoreItem::canBeNeutral() { return false; }


/////
// Lua interface

//               Fn name    Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getHealth, ARRAYDEF({{          END }}), 1 ) \
   METHOD(CLASS, setHealth, ARRAYDEF({{ NUM_GE0, END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(CoreItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(CoreItem, LUA_METHODS);

#undef LUA_METHODS


const char *CoreItem::luaClassName = "CoreItem";
REGISTER_LUA_SUBCLASS(CoreItem, Item);


S32 CoreItem::getHealth(lua_State *L) { return returnFloat(L, getTotalHealth()); }


S32 CoreItem::setHealth(lua_State *L) 
{ 
   checkArgList(L, functionArgs, "CoreItem", "setHealth");
   setStartingHealth(getFloat(L, 1));

   return 0;     
}


}; /* namespace Zap */
