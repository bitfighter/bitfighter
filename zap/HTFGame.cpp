//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "goalZone.h"
#include "gameType.h"
#include "ship.h"
#include "flagItem.h"
#include "gameObjectRender.h"

namespace Zap
{

class HTFGameType : public GameType
{
   typedef GameType Parent;
   static StringTableEntry aString;
   static StringTableEntry theString;

   Vector<GoalZone *> mZones;
   Vector<SafePtr<FlagItem> > mFlags;
   enum {
      ScoreTime = 5000,    // Time flag is in your zone to get points for your team
   };
   Timer mScoreTimer;
public:
   HTFGameType() : mScoreTimer(ScoreTime) {}    // Constructor, such as it is

   void addFlag(FlagItem *theFlag)
   {
      S32 i;
      for(i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i] == NULL)
         {
            mFlags[i] = theFlag;
            break;
         }
      }
      if(i == mFlags.size())
         mFlags.push_back(theFlag);

      if(!isGhost())
         addItemOfInterest(theFlag);
   }

   void addZone(GoalZone *zone)
   {
      mZones.push_back(zone);
   }

   // Note -- neutral or enemy-to-all robots can't pick up the flag!!!  When we add robots, this will be important!!!
   void shipTouchFlag(Ship *theShip, FlagItem *theFlag)
   {
      // See if the ship is already carrying a flag - can only carry one at a time
      for(S32 i = 0; i < theShip->mMountedItems.size(); i++)
         if(theShip->mMountedItems[i].isValid() && (theShip->mMountedItems[i]->getObjectTypeMask() & FlagType))
            return;

      S32 flagIndex;

      for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
         if(mFlags[flagIndex] == theFlag)
            break;

      GameConnection *controlConnection = theShip->getControllingClient();
      ClientRef *cl = controlConnection->getClientRef();
      if(!cl)
         return;

      // See if this flag is already in a flag zone owned by the ship's team
      if(theFlag->getZone() != NULL && theFlag->getZone()->getTeam() == theShip->getTeam())
         return;

      static StringTableEntry stealString("%e0 stole %e2 flag from team %e1!");
      static StringTableEntry takeString("%e0 of team %e1 took %e2 flag!");
      StringTableEntry r = takeString;
      U32 teamIndex;

      if(theFlag->getZone() == NULL)
         teamIndex = cl->teamId;
      else
      {
         r = stealString;
         teamIndex = theFlag->getZone()->getTeam();
      }
      Vector<StringTableEntry> e;
      e.push_back(cl->name);
      e.push_back(mTeams[teamIndex].name);

      if(mFlags.size() == 1)
         e.push_back(theString);
      else
         e.push_back(aString);

      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(
         GameConnection::ColorNuclearGreen, SFXFlagSnatch, r, e);
      theFlag->mountToShip(theShip);
      theFlag->setZone(NULL);
   }

   void flagDropped(Ship *theShip, FlagItem *theFlag)
   {
      static StringTableEntry aString("a");
      static StringTableEntry theString("the");
      static StringTableEntry dropString("%e0 dropped %e1 flag!");
      Vector<StringTableEntry> e;
      e.push_back(theShip->mPlayerName);
      if(mFlags.size() == 1)
         e.push_back(theString);
      else
         e.push_back(aString);

      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
   }

   void shipTouchZone(Ship *s, GoalZone *z)
   {
      GameConnection *controlConnection = s->getControllingClient();
      ClientRef *cl = controlConnection->getClientRef();

      if(!cl)
         return;

      // see if this is an opposing team's zone
      if(s->getTeam() != z->getTeam())
         return;

      // see if this zone already has a flag in it...
      for(S32 i = 0; i < mFlags.size(); i++)
         if(mFlags[i]->getZone() == z)
            return;

      // ok, it's an empty zone on our team:
      // see if this ship is carrying a flag
      S32 i;
      for(i = 0; i < s->mMountedItems.size(); i++)
         if(s->mMountedItems[i].isValid() && (s->mMountedItems[i]->getObjectTypeMask() & FlagType))
            break;
      if(i == s->mMountedItems.size())
         return;

      // ok, the ship has a flag and it's on the ship...
      Item *theItem = s->mMountedItems[i];
      FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
      if(mountedFlag)
      {
         static StringTableEntry capString("%e0 retrieved %e1 flag.  Team %e2 holds %e1 flag!");

         Vector<StringTableEntry> e;
         e.push_back(cl->name);
         if(mFlags.size() == 1)
            e.push_back(theString);
         else
            e.push_back(aString);

         e.push_back(mTeams[cl->teamId].name);

         for(S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

         mountedFlag->dismount();

         S32 flagIndex;
         for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
            if(mFlags[flagIndex] == mountedFlag)
               break;

         mFlags[flagIndex]->setZone(z);

         mountedFlag->setActualPos(z->getExtent().getCenter());
         // cl->score += 2;     // Event: ReturnFlagToZone

		 cl->score += getEventScore(IndividualScore, ReturnFlagToZone, 0);
		 setTeamScore(cl->teamId, mTeams[cl->teamId].score + getEventScore(TeamScore, ReturnFlagToZone, 0));

      }
   }

   void idle(GameObject::IdleCallPath path)
   {
      Parent::idle(path);

      if(path != GameObject::ServerIdleMainLoop)
         return;

 	  if(!mScoreTimer.update(mCurrentMove.time))
 		 return;

	  // Time to score!
	  mScoreTimer.reset();
	  for(S32 flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
	  {
		 if(mFlags[flagIndex]->getZone() != NULL)
		 {
		    S32 team = mFlags[flagIndex]->getZone()->getTeam();
		    // setTeamScore(team, mTeams[team].score + 1);           // Event: HoldFlagInZone
		    setTeamScore(team, mTeams[team].score + getEventScore(TeamScore, HoldFlagInZone, 0));
		    // No good way to award individual points for this event!!
		 }
	  }
   }

   void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
   {
      Parent::performProxyScopeQuery(scopeObject, connection);
      S32 uTeam = scopeObject->getTeam();

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i]->isAtHome() || mFlags[i]->getZone())
            connection->objectInScope(mFlags[i]);
         else
         {
            Ship *mount = mFlags[i]->getMount();
            if(mount && mount->getTeam() == uTeam)
            {
               connection->objectInScope(mount);
               connection->objectInScope(mFlags[i]);
            }
         }
      }
   }

   void renderInterfaceOverlay(bool scoreboardVisible)
   {
      Parent::renderInterfaceOverlay(scoreboardVisible);
      Ship *u = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      if(!u)
         return;
      bool uFlag = false;

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i].isValid() && mFlags[i]->getMount() == u)
         {
            for(S32 j = 0; j < mZones.size(); j++)
            {
               // see if this is one of our zones and that it doesn't have a flag in it.
               if(mZones[j]->getTeam() != u->getTeam())
                  continue;
               S32 k;
               for(k = 0; k < mFlags.size(); k++)
               {
                  if(!mFlags[k].isValid())
                     continue;
                  if(mFlags[k]->getZone() == mZones[j])
                     break;
               }
               if(k == mFlags.size())
                  renderObjectiveArrow(mZones[j], getTeamColor(u->getTeam()));
            }
            uFlag = true;
            break;
         }
      }

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(!mFlags[i].isValid())
            continue;

         if(!mFlags[i]->isMounted() && !uFlag)
         {
            GoalZone *gz = mFlags[i]->getZone();
            if(gz && gz->getTeam() != u->getTeam())
               renderObjectiveArrow(mFlags[i], getTeamColor(gz->getTeam()));
            else if(!gz)
               renderObjectiveArrow(mFlags[i], getTeamColor(-1));
         }
         else
         {
            Ship *mount = mFlags[i]->getMount();
            if(mount && mount != u)
               renderObjectiveArrow(mount, getTeamColor(mount->getTeam()));
         }
      }
   }
   const char *getGameTypeString() { return "Hold the Flag"; }
   const char *getInstructionString() { return "Hold the flags at your capture zones!"; }
   bool isTeamGame() { return true; }


   // What does a particular scoring event score?
   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
   {
      if(scoreGroup == TeamScore)
      {
         switch(scoreEvent)
         {
            case KillEnemy:
               return 0;
            case KillSelf:
               return 0;
            case KillTeammate:
               return 0;
            case ReturnFlagToZone:
               return 2;
            case HoldFlagInZone:		// Per ScoreTime ms
               return 1;
            default:
               logprintf("Unknown scoring event: %d", scoreEvent);
               return 0;
         }
      }
      else  // scoreGroup == IndividualScore
      {
         switch(scoreEvent)
         {
            case KillEnemy:
               return 1;
            case KillSelf:
               return -1;
            case KillTeammate:
               return 0;
		   case ReturnFlagToZone:
			  return 2;
		   // case HoldFlagInZone:		// There's not a good way to award these points
		   //	  return 0;             // and unless we really want them, let's not bother
            default:
               logprintf("Unknown scoring event: %d", scoreEvent);
               return 0;
         }
      }
   }

   TNL_DECLARE_CLASS(HTFGameType);
};

StringTableEntry HTFGameType::aString("a");
StringTableEntry HTFGameType::theString("the");

TNL_IMPLEMENT_NETOBJECT(HTFGameType);


};

