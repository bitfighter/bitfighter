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
#include "UIGame.h"
#include "gameNetInterface.h"
#include "ship.h"
#include "projectile.h"
#include "gameObjectRender.h"
#include "goalZone.h"
#include "../glut/glutInclude.h"

namespace Zap
{

class Ship;
class SoccerBallItem;

TNL_IMPLEMENT_NETOBJECT(SoccerGameType);

TNL_IMPLEMENT_NETOBJECT_RPC(SoccerGameType, s2cSoccerScoreMessage,
   (U32 msgIndex, StringTableEntry clientName, RangedU32<0, GameType::gMaxTeamCount> teamIndex), (msgIndex, clientName, teamIndex),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   S32 teamIndexAdjusted = (S32) teamIndex + GameType::gFirstTeamNumber;      // Before calling this RPC, we subtracted gFirstTeamNumber, so we need to add it back here...
   string msg;
   SoundSystem::playSoundEffect(SFXFlagCapture);

   // Compose the message

   if(clientName.isNull())    // Unknown player scored
   {
      if(teamIndexAdjusted >= 0)
         msg = "A goal was scored on team " + string(getTeamName(teamIndexAdjusted).getString());
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
            msg = string(clientName.getString()) + " scored a goal on team " + string(getTeamName(teamIndexAdjusted).getString());
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
      msg = string(clientName.getString()) + " scored an own-goal, giving the other team" + (mTeams.size() == 2 ? "" : "s") + " a point!";
   }


   ClientGame *clientGame = dynamic_cast<ClientGame *>(getGame());
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   // Print the message
   clientGame->mGameUserInterface->displayMessage(Color(0.6f, 1.0f, 0.8f), msg.c_str());
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
   if(scoringTeam == Item::NO_TEAM)
   {
      s2cSoccerScoreMessage(SoccerMsgScoreGoal, scorerName, (U32) (goalTeamIndex - gFirstTeamNumber));
      return;
   }

   if(isTeamGame() && (scoringTeam == Item::TEAM_NEUTRAL || scoringTeam == goalTeamIndex))    // Own-goal
   {
      updateSoccerScore(ship, scoringTeam, ScoreGoalOwnTeam, score);

      // Subtract gFirstTeamNumber to fit goalTeamIndex into a neat RangedU32 container
      s2cSoccerScoreMessage(SoccerMsgScoreOwnGoal, scorerName, (U32) (goalTeamIndex - gFirstTeamNumber));
   }
   else     // Goal on someone else's goal
   {
      if(goalTeamIndex == Item::TEAM_HOSTILE)
         updateSoccerScore(ship, scoringTeam, ScoreGoalHostileTeam, score);
      else
         updateSoccerScore(ship, scoringTeam, ScoreGoalEnemyTeam, score);

      s2cSoccerScoreMessage(SoccerMsgScoreGoal, scorerName, (U32) (goalTeamIndex - gFirstTeamNumber));      // See comment above
   }
}


// Runs on client
void SoccerGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!ship)
      return;

   S32 team = ship->getTeam();

   for(S32 i = 0; i < mGoals.size(); i++)
   {
      if(mGoals[i]->getTeam() != team)
         renderObjectiveArrow(mGoals[i], getTeamColor(mGoals[i]->getTeam()));
   }
   if(mBall.isValid())
      renderObjectiveArrow(mBall, getTeamColor(-1));
}


void SoccerGameType::shipTouchZone(Ship *ship, GoalZone *zone)
{
   // See if this ship is carrying a ball
   if(!ship->isCarryingItem(SoccerBallItemType))
      return;

   // The ship has a ball
   SoccerBallItem *ball = dynamic_cast<SoccerBallItem *>(ship->unmountItem(SoccerBallItemType));
   ball->collide(zone);
}


// Runs when ship fires, return true or false on whether firing should proceed
bool SoccerGameType::onFire(Ship *ship) 
{ 
   if(!Parent::onFire(ship))
      return false;


   // If we have the ball, drop it
   if(ship->isCarryingItem(SoccerBallItemType))
   {
      for(S32 i = 0; i < ship->mMountedItems.size(); i++)
      {
         if(ship->mMountedItems[i]->getObjectTypeMask() & SoccerBallItemType)
         {
            SoccerBallItem *ball = dynamic_cast<SoccerBallItem *>(ship->mMountedItems[i].getPointer());
            TNLAssert(ball != NULL, "SoccerGameType::onFire NULL ball");

            if(getGame()->getCurrentTime() - ball->mPickupTime < 1000)
               return false;
            //Point dir = ship->getAimVector();
            //Point vel = ship->getActualVel();

            //dir.normalize();     // needed?

            //Point ballVel = dir * 2 + dir * vel.dot(dir);

            // ball->setActualPos(ship->getActualPos() + dir * (ship->getRadius())); //get set in onItemDropped

            ball->onItemDropped();
            //ball->setActualVel(ship->getAimVector() * 200 + ship->getActualVel()); 
				//dir * F32(wi->projVelocity) + dir * shooterVel.dot(dir);
            ball->setActualVel(ship->getAimVector() * 200 + ship->getAimVector() * ship->getActualVel().dot(ship->getAimVector()));
         }
      }
      
      return false;
   }

   return true;

}             


// Runs on server only, and only when player deliberately drops ball
void SoccerGameType::itemDropped(Ship *ship, Item *item)
{
   //logprintf("%s SoccerGameType->itemDropped", isGhost()? "Client:" : "Server:");
   TNLAssert(!isGhost(), "Should run on server only!");

   static StringTableEntry dropString("%e0 dropped the ball!");

   Vector<StringTableEntry> e;
   e.push_back(ship->getName());

   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
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

// Constructor
SoccerBallItem::SoccerBallItem(Point pos) : Item(pos, true, SoccerBallItem::SOCCER_BALL_RADIUS, 4)
{
   mObjectTypeMask |= CommandMapVisType | TurretTargetType | SoccerBallItemType;
   mNetFlags.set(Ghostable);
   initialPos = pos;
   mLastPlayerTouch = NULL;
   mLastPlayerMounted = NULL;
   mLastPlayerTouchTeam = Item::NO_TEAM;
   mLastPlayerTouchName = StringTableEntry(NULL);
   mDragFactor = 1.0;     // 1.0 for no drag
}


const char SoccerBallItem::className[] = "SoccerBallItem";      // Class name as it appears to Lua scripts


bool SoccerBallItem::processArguments(S32 argc, const char **argv)
{
   if(!Parent::processArguments(argc, argv))
      return false;

   initialPos = mMoveState[ActualState].pos;

   // Add the ball's starting point to the list of flag spawn points
   gServerGame->getGameType()->mFlagSpawnPoints.push_back(FlagSpawn(initialPos, 0));

   return true;
}


// Define the methods we will expose to Lua
Lunar<SoccerBallItem>::RegType SoccerBallItem::methods[] =
{
   // Standard gameItem methods
   method(SoccerBallItem, getClassID),
   method(SoccerBallItem, getLoc),
   method(SoccerBallItem, getRad),
   method(SoccerBallItem, getVel),
   method(SoccerBallItem, getTeamIndx),
 
   // item methods
   method(SoccerBallItem, isInCaptureZone),
   method(SoccerBallItem, getCaptureZone),
   method(SoccerBallItem, isOnShip),
   method(SoccerBallItem, getShip),

   {0,0}    // End method list
};


void SoccerBallItem::onAddedToGame(Game *theGame)
{
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
      gt->mHaveSoccer = true;
      SoccerGameType * sgt = dynamic_cast<SoccerGameType *>(gt);
      if(sgt) sgt->setBall(this);
   }
   getGame()->mObjectsLoaded++;
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


void SoccerBallItem::renderItem(Point pos)
{
   renderSoccerBall(pos);
}


void SoccerBallItem::idle(GameObject::IdleCallPath path)
{
   if(mSendHomeTimer.update(mCurrentMove.time))
      if(!isGhost())
         sendHome();

   else if(mSendHomeTimer.getCurrent())      // Goal has been scored, waiting for ball to reset
   {
      F32 accelFraction = 1 - (0.95 * mCurrentMove.time * 0.001f);

      mMoveState[ActualState].vel *= accelFraction;
      mMoveState[RenderState].vel *= accelFraction;
   }

   // The following block will add some friction to the soccer ball
   else
   {
      F32 accelFraction = 1 - (mDragFactor * mCurrentMove.time * 0.001f);
   
      mMoveState[ActualState].vel *= accelFraction;
      mMoveState[RenderState].vel *= accelFraction;
   }

   Parent::idle(path);

   // If crash into something, the ball will hit first, so we want to make sure it has an up-to-date velocity vector
   if(isMounted())
   if(mMount)      //client side NULL when the soccer is mounted to far away ship.
      mMoveState[ActualState].vel.set(mMount->getActualVel());
}


void SoccerBallItem::damageObject(DamageInfo *theInfo)
{
   if(mMount != NULL)
      onItemDropped();
  
   // Compute impulse direction
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;

   if(theInfo->damagingObject)
   {
      if(theInfo->damagingObject->getObjectTypeMask() & (ShipType | RobotType))
      {
         mLastPlayerTouch = dynamic_cast<Ship *>(theInfo->damagingObject);
         mLastPlayerTouchTeam = mLastPlayerTouch->getTeam();
         mLastPlayerTouchName = mLastPlayerTouch->getName();
      }

      else if(theInfo->damagingObject->getObjectTypeMask() & (BulletType | MineType | SpyBugType))
      {
         Projectile *p = dynamic_cast<Projectile *>(theInfo->damagingObject);
         Ship *ship = dynamic_cast<Ship *>(p->mShooter.getPointer());
         mLastPlayerTouch = ship ? ship : NULL;    // If shooter was a turret, say, we'd expect s to be NULL.
         mLastPlayerTouchTeam = p->getTeam(); // Projectile always have a team from what fired it, can be used to credit a team.
         mLastPlayerTouchName = ship ? ship->getName() : StringTableEntry(NULL);
      }
      else
         mLastPlayerTouch = NULL;
   }
}


void SoccerBallItem::sendHome()
{
   TNLAssert(!isGhost(), "Should only run on server!");

   // In soccer game, we use flagSpawn points to designate where the soccer ball should spawn.
   // We'll simply redefine "initial pos" as a random selection of the flag spawn points

   Vector<FlagSpawn> spawnPoints = getGame()->getGameType()->mFlagSpawnPoints;

   S32 spawnIndex = TNL::Random::readI() % spawnPoints.size();
   initialPos = spawnPoints[spawnIndex].getPos();

   mMoveState[ActualState].pos = mMoveState[RenderState].pos = initialPos;
   mMoveState[ActualState].vel = mMoveState[RenderState].vel = Point(0,0);
   setMaskBits(WarpPositionMask | PositionMask);      // By warping, we eliminate the "drifting" effect we got when we used PositionMask
   updateExtent();
}


bool SoccerBallItem::collide(GameObject *hitObject)
{
   if(mSendHomeTimer.getCurrent())     // If we've already scored, and we're waiting for the ball to reset, there's nothing to do
      return true;

   if(hitObject->getObjectTypeMask() & (ShipType | RobotType))
   {
     if(mMount == hitObject)    // Sometimes we get collisions between ship and an already mounted soccer ball.
        return false;  //false = don't hit self

     if(mIsMounted)  // fix problem with random ball jumping from ship to ship?
        return false;

    if(!isGhost())  //Server side
    {
         // mLastPlayerMounted == hitObject &&   without this, that can limit anyone to pickup ball
      if(mDroppedTimer.getCurrent())      // Have to wait a bit after dropping to pick the ball back up!
         return false;   //False - Go through soccer looks better while dropping, and allow better sync to client.


      Ship *ship = dynamic_cast<Ship *>(hitObject);
      mLastPlayerTouch = ship;
      mLastPlayerTouchTeam = mLastPlayerTouch->getTeam();      // Used to credit team if ship quits game before goal is scored
      mLastPlayerTouchName = mLastPlayerTouch->getName();      // Used for making nicer looking messages in same situation
      GameType *gt = getGame()->getGameType();
      if(gt && !gt->mAllowSoccerPickup)
         return true;
      
      
      mDroppedTimer.clear();
      this->mountToShip(ship);
      mLastPlayerMounted = ship;
      mPickupTime = getGame()->getCurrentTime();
    }else{ //client side
         // Not needed
         //if(getGame()->getGameType())
         //   getGame()->getGameType()->c2sResendItemStatus(mItemId);
       if(gClientGame && gClientGame->getConnectionToServer())
          return gClientGame->getConnectionToServer()->mSoccerCollide;
       return false; //let server do the collision.
    }
   }
   else if(hitObject->getObjectTypeMask() & GoalZoneType)      // SCORE!!!!
   {
      GoalZone *goal = dynamic_cast<GoalZone *>(hitObject);

      if(goal && !isGhost())
      {
         SoccerGameType *g = dynamic_cast<SoccerGameType *>(getGame()->getGameType());
         if(g) g->scoreGoal(mLastPlayerTouch, mLastPlayerTouchName, mLastPlayerTouchTeam, goal->getTeam(), goal->mScore);

         static const S32 POST_SCORE_HIATUS = 1500;
         mSendHomeTimer.reset(POST_SCORE_HIATUS);
      }
      return false;
   }
   return true;
}


};

