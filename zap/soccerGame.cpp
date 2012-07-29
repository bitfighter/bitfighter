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

#include "soccerGame.h"
#include "gameNetInterface.h"
#include "projectile.h"
#include "gameObjectRender.h"
#include "goalZone.h"
#include "gameConnection.h"
#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIGame.h"
#endif


namespace Zap
{

// Constructor
SoccerGameType::SoccerGameType()
{
   // Do nothing
}


TNL_IMPLEMENT_NETOBJECT(SoccerGameType);

TNL_IMPLEMENT_NETOBJECT_RPC(SoccerGameType, s2cSoccerScoreMessage,
   (U32 msgIndex, StringTableEntry clientName, RangedU32<0, GameType::gMaxTeamCount> teamIndex), (msgIndex, clientName, teamIndex),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED

   S32 teamIndexAdjusted = (S32) teamIndex + GameType::gFirstTeamNumber;      // Before calling this RPC, we subtracted gFirstTeamNumber, so we need to add it back here...
   string msg;
   SoundSystem::playSoundEffect(SFXFlagCapture);

   // Compose the message

   if(clientName.isNull())    // Unknown player scored
   {
      if(teamIndexAdjusted >= 0)
         msg = "A goal was scored on team " + string(getGame()->getTeamName(teamIndexAdjusted).getString());
      else if(teamIndexAdjusted == -1)
         msg = "A goal was scored on a neutral goal!";
      else if(teamIndexAdjusted == -2)
         msg = "A goal was scored on a hostile goal!";
      else
         msg = "A goal was scored on an unknown goal!";
   }
   else if(msgIndex == SoccerMsgScoreGoal)
   {
      if(isTeamGame())
      {
         if(teamIndexAdjusted >= 0)
            msg = string(clientName.getString()) + " scored a goal on team " + string(getGame()->getTeamName(teamIndexAdjusted).getString());
         else if(teamIndexAdjusted == -1)
            msg = string(clientName.getString()) + " scored a goal on a neutral goal!";
         else if(teamIndexAdjusted == -2)
            msg = string(clientName.getString()) + " scored a goal on a hostile goal (for negative points!)";
         else
            msg = string(clientName.getString()) + " scored a goal on an unknown goal!";
      }
      else  // every man for himself
      {
         if(teamIndexAdjusted >= -1)      // including neutral goals
            msg = string(clientName.getString()) + " scored a goal!";
         else if(teamIndexAdjusted == -2)
            msg = string(clientName.getString()) + " scored a goal on a hostile goal (for negative points!)";
      }
   }
   else if(msgIndex == SoccerMsgScoreOwnGoal)
   {
      msg = string(clientName.getString()) + " scored an own-goal, giving the other team" + 
                  (getGame()->getTeamCount() == 2 ? "" : "s") + " a point!";
   }


   ClientGame *clientGame = static_cast<ClientGame *>(getGame());

   // Print the message
   clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f), msg.c_str());

#endif
}


void SoccerGameType::addZone(GoalZone *theZone)
{
   mGoals.push_back(theZone);
}


void SoccerGameType::setBall(SoccerBallItem *theBall)
{
   mBall = theBall;
}


// Helper function to make sure the two-arg version of updateScore doesn't get a null ship
void SoccerGameType::updateSoccerScore(Ship *ship, S32 scoringTeam, ScoringEvent scoringEvent, S32 score)
{
   if(ship)
      updateScore(ship, scoringEvent, score);
   else
      updateScore(NULL, scoringTeam, scoringEvent, score);
}


void SoccerGameType::scoreGoal(Ship *ship, StringTableEntry scorerName, S32 scoringTeam, S32 goalTeamIndex, S32 score)
{
   if(scoringTeam == NO_TEAM)
   {
      s2cSoccerScoreMessage(SoccerMsgScoreGoal, scorerName, (U32) (goalTeamIndex - gFirstTeamNumber));
      return;
   }

   if(isTeamGame() && (scoringTeam == TEAM_NEUTRAL || scoringTeam == goalTeamIndex))    // Own-goal
   {
      updateSoccerScore(ship, scoringTeam, ScoreGoalOwnTeam, score);

      // Subtract gFirstTeamNumber to fit goalTeamIndex into a neat RangedU32 container
      s2cSoccerScoreMessage(SoccerMsgScoreOwnGoal, scorerName, (U32) (goalTeamIndex - gFirstTeamNumber));
   }
   else     // Goal on someone else's goal
   {
      if(goalTeamIndex == TEAM_HOSTILE)
         updateSoccerScore(ship, scoringTeam, ScoreGoalHostileTeam, score);
      else
         updateSoccerScore(ship, scoringTeam, ScoreGoalEnemyTeam, score);

      s2cSoccerScoreMessage(SoccerMsgScoreGoal, scorerName, (U32) (goalTeamIndex - gFirstTeamNumber));      // See comment above
   }
}


// Runs on client
void SoccerGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
#ifndef ZAP_DEDICATED

   Parent::renderInterfaceOverlay(scoreboardVisible);

   BfObject *object = static_cast<ClientGame *>(getGame())->getConnectionToServer()->getControlObject();

   if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return;

   Ship *ship = static_cast<Ship *>(object);

   S32 team = ship->getTeam();

   for(S32 i = 0; i < mGoals.size(); i++)
   {
      if(mGoals[i]->getTeam() != team)
         renderObjectiveArrow(mGoals[i], mGoals[i]->getColor());
   }
   if(mBall.isValid())
      renderObjectiveArrow(mBall, getTeamColor(-1));
#endif
}


GameTypeId SoccerGameType::getGameTypeId() const { return SoccerGame; }

const char *SoccerGameType::getShortName()      const { return "S"; }
const char *SoccerGameType::getInstructionString() const { return "Push the ball into the opposing team's goal!"; } 

bool SoccerGameType::canBeTeamGame()       const { return true; }
bool SoccerGameType::canBeIndividualGame() const { return true; }


// Runs when ship fires, return true or false on whether firing should proceed
bool SoccerGameType::onFire(Ship *ship) 
{ 
   if(!Parent::onFire(ship))
      return false;

   return true;
}


// Runs on server only, and only when player deliberately drops ball... gets run when dropping resurce item
void SoccerGameType::itemDropped(Ship *ship, MoveItem *item)
{
   TNLAssert(!isGhost(), "Should run on server only!");

   //static StringTableEntry dropString("%e0 dropped the ball!");

   //Vector<StringTableEntry> e;
   //e.push_back(ship->getClientInfo()->getName());

   //broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
}


// What does a particular scoring event score?
S32 SoccerGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
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
         case ScoreGoalEnemyTeam:
            return data;
         case ScoreGoalOwnTeam:
            return -data;
         case ScoreGoalHostileTeam:
            return -data;
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
         case ScoreGoalEnemyTeam:
            return 5 * data;
         case ScoreGoalOwnTeam:
            return -5 * data;
         case ScoreGoalHostileTeam:
            return -5 * data;
         default:
            return naScore;
      }
   }
}

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(SoccerBallItem);

static const F32 SOCCER_BALL_ITEM_MASS = 4;

// Constructor
SoccerBallItem::SoccerBallItem(Point pos) : Parent(pos, true, (F32)SoccerBallItem::SOCCER_BALL_RADIUS, SOCCER_BALL_ITEM_MASS)
{
   mObjectTypeNumber = SoccerBallItemTypeNumber;
   mNetFlags.set(Ghostable);
   initialPos = pos;
   mLastPlayerTouch = NULL;
   mLastPlayerTouchTeam = NO_TEAM;
   mLastPlayerTouchName = StringTableEntry(NULL);

   const F32 NO_DRAG = 0.0;
   mDragFactor = NO_DRAG;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


SoccerBallItem::~SoccerBallItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


SoccerBallItem *SoccerBallItem::clone() const
{
   return new SoccerBallItem(*this);
}


bool SoccerBallItem::processArguments(S32 argc2, const char **argv2, Game *game)
{
   S32 argc = 0;
   const char *argv[16];

   GameType *gameType = game->getGameType();

   for(S32 i = 0; i < argc2; i++)      // The idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char firstChar = argv2[i][0];    // First character of arg

      if((firstChar < 'a' || firstChar > 'z') && (firstChar < 'A' || firstChar > 'Z'))    // firstChar is not a letter
      {
         if(argc < 16)
         {  
            argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(!Parent::processArguments(argc, argv, game))
      return false;

   initialPos = getActualPos();

   // Add the ball's starting point to the list of flag spawn points
   gameType->addFlagSpawn(FlagSpawn(initialPos, 0));

   return true;
}


// Yes, this method is superfluous, but makes it clear that it wasn't forgotten... always include toString() alongside processArguments()!
string SoccerBallItem::toString(F32 gridSize) const
{
   return Parent::toString(gridSize);
}


void SoccerBallItem::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   // Make soccer ball always visible
   if(!isGhost())
      setScopeAlways();

   // Make soccer ball only visible when in scope
   //if(!isGhost())
   //   theGame->getGameType()->addItemOfInterest(this);

   //((SoccerGameType *) theGame->getGameType())->setBall(this);
   GameType *gt = theGame->getGameType();
   if(gt)
   {
      if(gt->getGameTypeId() == SoccerGame)
         static_cast<SoccerGameType *>(gt)->setBall(this);
   }
}


// Runs on client & server?
void SoccerBallItem::onItemDropped()
{

   if(mMount.isValid() && !isGhost())   //Server only, to prevent desync
   {
      this->setActualPos(mMount->getActualPos()); 
      this->setActualVel(mMount->getActualVel() * 1.5);
   }   
   
   Parent::onItemDropped();
}


void SoccerBallItem::renderItem(const Point &pos)
{
   renderSoccerBall(pos);
}


const char *SoccerBallItem::getOnScreenName()     { return "Soccer Ball";  }
const char *SoccerBallItem::getOnDockName()       { return "Ball";         }
const char *SoccerBallItem::getPrettyNamePlural() { return "Soccer Balls"; }
const char *SoccerBallItem::getEditorHelpString() { return "Soccer ball, can only be used in Soccer games."; }


bool SoccerBallItem::hasTeam()      { return false; }
bool SoccerBallItem::canBeHostile() { return false; }
bool SoccerBallItem::canBeNeutral() { return false; }


void SoccerBallItem::renderDock()
{
   renderSoccerBall(getRenderPos(), 7);
}


void SoccerBallItem::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   renderItem(getRenderPos());
}



void SoccerBallItem::idle(BfObject::IdleCallPath path)
{
   if(mSendHomeTimer.update(mCurrentMove.time))
   {
      if(!isGhost())
         sendHome();
   }
   else if(mSendHomeTimer.getCurrent())      // Goal has been scored, waiting for ball to reset
   {
      F32 accelFraction = 1 - (0.95f * mCurrentMove.time * 0.001f);

      setActualVel(getActualVel() * accelFraction);
      setRenderVel(getActualVel() * accelFraction);
   }

   // The following block will add some friction to the soccer ball
   else
   {
      F32 accelFraction = 1 - (mDragFactor * mCurrentMove.time * 0.001f);
   
      setActualVel(getActualVel() * accelFraction);
      setRenderVel(getActualVel() * accelFraction);
   }


   Parent::idle(path);

   // If crash into something, the ball will hit first, so we want to make sure it has an up-to-date velocity vector
   if(isMounted())
   if(mMount)      //client side NULL when the soccer is mounted to far away ship.
      setActualVel(mMount->getActualVel());
}


void SoccerBallItem::damageObject(DamageInfo *theInfo)
{
   if(mMount != NULL)
      onItemDropped();
  
   computeImpulseDirection(theInfo);

   if(theInfo->damagingObject)
   {
      U8 typeNumber = theInfo->damagingObject->getObjectTypeNumber();

      if(isShipType(typeNumber))
      {
         mLastPlayerTouch = static_cast<Ship *>(theInfo->damagingObject);
         mLastPlayerTouchTeam = mLastPlayerTouch->getTeam();
         if(mLastPlayerTouch->getClientInfo())
            mLastPlayerTouchName = mLastPlayerTouch->getClientInfo()->getName();
         else
            mLastPlayerTouchName = NULL;
      }

      else if(isProjectileType(typeNumber))
      {
         BfObject *shooter;

         if(typeNumber == BulletTypeNumber)
            shooter = static_cast<Projectile*>(theInfo->damagingObject)->mShooter;
         else if(typeNumber == BurstTypeNumber || typeNumber == MineTypeNumber || typeNumber == SpyBugTypeNumber)
            shooter = static_cast<BurstProjectile*>(theInfo->damagingObject)->mShooter;
         else if(typeNumber == HeatSeekerTypeNumber)
            shooter = static_cast<HeatSeekerProjectile*>(theInfo->damagingObject)->mShooter;
         else
         {
            TNLAssert(false, "Undefined projectile type?");
            shooter = NULL;
         }

         if(shooter && isShipType(shooter->getObjectTypeNumber()))
         {
            Ship *ship = static_cast<Ship *>(shooter);
            mLastPlayerTouch = ship;             // If shooter was a turret, say, we'd expect s to be NULL.
            mLastPlayerTouchTeam = theInfo->damagingObject->getTeam(); // Projectile always have a team from what fired it, can be used to credit a team.
            if(ship->getClientInfo())
               mLastPlayerTouchName = ship->getClientInfo()->getName();
            else
               mLastPlayerTouchName = NULL;
         }
      }
      else
         resetPlayerTouch();
   }
}


void SoccerBallItem::resetPlayerTouch()
{
   mLastPlayerTouch = NULL;
   mLastPlayerTouchTeam = NO_TEAM;
   mLastPlayerTouchName = StringTableEntry(NULL);
}


void SoccerBallItem::sendHome()
{
   TNLAssert(!isGhost(), "Should only run on server!");

   // In soccer game, we use flagSpawn points to designate where the soccer ball should spawn.
   // We'll simply redefine "initial pos" as a random selection of the flag spawn points

   const Vector<FlagSpawn> *spawnPoints = getGame()->getGameType()->getFlagSpawns();

   S32 spawnIndex = TNL::Random::readI() % spawnPoints->size();
   initialPos = spawnPoints->get(spawnIndex).getPos();

   setPosVelAng(initialPos, Point(0,0), 0);

   setMaskBits(WarpPositionMask | PositionMask);      // By warping, we eliminate the "drifting" effect we got when we used PositionMask

   updateExtentInDatabase();

   resetPlayerTouch();
}


bool SoccerBallItem::collide(BfObject *hitObject)
{
   if(mSendHomeTimer.getCurrent())     // If we've already scored, and we're waiting for the ball to reset, there's nothing to do
      return true;

   if(isShipType(hitObject->getObjectTypeNumber()))
   {
      if(!isGhost())    //Server side
      {
         Ship *ship = static_cast<Ship *>(hitObject);
         mLastPlayerTouch = ship;
         mLastPlayerTouchTeam = mLastPlayerTouch->getTeam();                  // Used to credit team if ship quits game before goal is scored
         if(mLastPlayerTouch->getClientInfo())
            mLastPlayerTouchName = mLastPlayerTouch->getClientInfo()->getName(); // Used for making nicer looking messages in same situation
         else
            mLastPlayerTouchName = NULL;
      }
   }
   else if(hitObject->getObjectTypeNumber() == GoalZoneTypeNumber)      // SCORE!!!!
   {
      GoalZone *goal = static_cast<GoalZone *>(hitObject);

      if(!isGhost())
      {
         GameType *gameType = getGame()->getGameType();
         if(gameType && gameType->getGameTypeId() == SoccerGame)
            static_cast<SoccerGameType *>(gameType)->scoreGoal(mLastPlayerTouch, mLastPlayerTouchName, mLastPlayerTouchTeam, goal->getTeam(), goal->getScore());

         static const S32 POST_SCORE_HIATUS = 1500;
         mSendHomeTimer.reset(POST_SCORE_HIATUS);
      }
      return false;
   }
   return true;
}


U32 SoccerBallItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   return Parent::packUpdate(connection, updateMask, stream);
}


void SoccerBallItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
}


/////
// Lua interface

// No soccerball specific methods!
const luaL_reg           SoccerBallItem::luaMethods[]   = { { NULL, NULL } };
const LuaFunctionProfile SoccerBallItem::functionArgs[] = { { NULL, { }, 0 } };


const char *SoccerBallItem::luaClassName = "SoccerBallItem";
REGISTER_LUA_SUBCLASS(SoccerBallItem, MoveItem);


};

