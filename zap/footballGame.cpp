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
class Ship;

class ZoneControlGameType : public GameType
{
private:
   typedef GameType Parent;

   Vector<GoalZone*> mZones;
   SafePtr<FlagItem> mFlag;
   S32 mFlagTeam;

public:
   ZoneControlGameType()
   {
      mFlagTeam = -1;
   }

   void onGhostAvailable(GhostConnection *theConnection)
   {
      Parent::onGhostAvailable(theConnection);
      NetObject::setRPCDestConnection(theConnection);
      s2cSetFlagTeam(mFlagTeam);
      NetObject::setRPCDestConnection(NULL);
   }

   void shipTouchFlag(Ship *theShip, FlagItem *theFlag);
   void flagDropped(Ship *theShip, FlagItem *theFlag);
   void addZone(GoalZone *z);
   void addFlag(FlagItem *theFlag)
   {
      mFlag = theFlag;
      addItemOfInterest(theFlag);
   }
   void shipTouchZone(Ship *s, GoalZone *z);
   const char *getGameTypeString() { return "Zone Control"; }
   const char *getInstructionString() { return "Capture each zone by carrying the flag into it!"; }
   bool isTeamGame() { return true; }

   void renderInterfaceOverlay(bool scoreboardVisible);
   void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection);
   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);


   TNL_DECLARE_RPC(s2cSetFlagTeam, (S32 newFlagTeam));
   TNL_DECLARE_CLASS(ZoneControlGameType);
};

TNL_IMPLEMENT_NETOBJECT(ZoneControlGameType);

// Which team has the flag?
GAMETYPE_RPC_S2C(ZoneControlGameType, s2cSetFlagTeam, (S32 flagTeam), (flagTeam))
{
   mFlagTeam = flagTeam;
}


// Ship picks up the flag
void ZoneControlGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   GameConnection *controlConnection = theShip->getControllingClient();
   ClientRef *cl = controlConnection->getClientRef();
   if(!cl)
      return;

   static StringTableEntry takeString("%e0 of team %e1 has the flag!");
   Vector<StringTableEntry> e;
   e.push_back(cl->name);
   e.push_back(mTeams[cl->teamId].name);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
   theFlag->mountToShip(theShip);
   mFlagTeam = theShip->getTeam();
   s2cSetFlagTeam(mFlagTeam);
}

void ZoneControlGameType::flagDropped(Ship *theShip, FlagItem *theFlag)
{
   s2cSetFlagTeam(-1);
   static StringTableEntry dropString("%e0 dropped the flag!");
   Vector<StringTableEntry> e;
   e.push_back(theShip->mPlayerName);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
}

void ZoneControlGameType::addZone(GoalZone *z)
{
   mZones.push_back(z);
}

// Ship enters a goal zone.  What happens?
void ZoneControlGameType::shipTouchZone(Ship *s, GoalZone *z)
{

   // Does the zone already belong to the ship's team?
   if(z->getTeam() == s->getTeam())
      return;

   // Is the ship carrying the flag?
   S32 i;
   for(i = 0; i < s->mMountedItems.size(); i++)
      if(s->mMountedItems[i].isValid() && (s->mMountedItems[i]->getObjectTypeMask() & FlagType))
         break;
   if(i == s->mMountedItems.size())
      return;

	ClientRef *cl = controlConnection->getClientRef();
	if(!cl)
      return;

   S32 oldTeam = z->getTeam();
   if(oldTeam >= 0)    // Zone is being captured from another team
   {
      static StringTableEntry takeString("%e0 captured a zone from team %e1!");
      Vector<StringTableEntry> e;
      e.push_back(s->mPlayerName);
      e.push_back(mTeams[oldTeam].name);

      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);

      // setTeamScore(oldTeam, mTeams[oldTeam].score - 1);     // Event: UncaptureZone
	  cl->score += getEventScore(IndividualScore, UncaptureZone, 0);
	  setTeamScore(z->teamId, mTeams[z->teamId].score + getEventScore(TeamScore, UncaptureZone, 0));
   }
   else                 // Zone is neutral
   {
      static StringTableEntry takeString("%e0 captured an unclaimed zone!");
      Vector<StringTableEntry> e;
      e.push_back(s->mPlayerName);
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
   }

   // setTeamScore(s->getTeam(), mTeams[s->getTeam()].score + 1);    // Capturing team gets a point      // Event: CaptureZone
   cl->score += getEventScore(IndividualScore, CaptureZone, 0);
   setTeamScore(cl->teamId, mTeams[cl->teamId].score + getEventScore(TeamScore, CaptureZone, 0));


   z->setTeam(s->getTeam());                                      // and the zone is assigned to capturing team

   // Check to see if team now controls all zones
   for(S32 i = 0; i < mZones.size(); i++)
   {
      if(mZones[i]->getTeam() != s->getTeam())     // No?
         return;                                   // ...then bail
   }

   // Team DOES control all zones.  Broadcast a message
   static StringTableEntry tdString("Team %e0 scored a touchdown!");
   Vector<StringTableEntry> e;
   e.push_back(mTeams[s->getTeam()].name);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, tdString, e);

   // Reset zones to neutral
   for(S32 i = 0; i < mZones.size(); i++)
      mZones[i]->setTeam(-1);

   // Return the flag to its starting point
   for(S32 i = 0; i < s->mMountedItems.size(); i++)
   {
      Item *theItem = s->mMountedItems[i];
      FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
      if(mountedFlag)
      {
         mountedFlag->dismount();
         mountedFlag->sendHome();
      }
   }
   mFlagTeam = -1;
   s2cSetFlagTeam(-1);
}

void ZoneControlGameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
{
   Parent::performProxyScopeQuery(scopeObject, connection);
   S32 uTeam = scopeObject->getTeam();
   if(mFlag.isValid())
   {
      if(mFlag->isAtHome())
         connection->objectInScope(mFlag);
      else
      {
         Ship *mount = mFlag->getMount();
         if(mount && mount->getTeam() == uTeam)
         {
            connection->objectInScope(mount);
            connection->objectInScope(mFlag);
         }
      }
   }
}

// Do some extra rendering required by this game
void ZoneControlGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!ship)
      return;

   bool hasFlag = mFlag.isValid() && mFlag->getMount() == ship;

   // First, render all zones that have not yet been taken by the flag holding team
   if(mFlagTeam != -1)
   {
      for(S32 i = 0; i < mZones.size(); i++)
      {
         if(mZones[i]->getTeam() != mFlagTeam)
            renderObjectiveArrow(mZones[i], getTeamColor(mZones[i]), hasFlag ? 1.0f : 0.4f);
         else if(mZones[i]->didRecentlyChangeTeam() && mZones[i]->getTeam() != -1 && ship->getTeam() != mFlagTeam)
         {
            // Render a blinky arrow for a recently taken zone
            Color c = getTeamColor(mZones[i]);
            if(mZones[i]->isFlashing())
               c *= 0.7f;
            renderObjectiveArrow(mZones[i], c);
         }
      }
   }
   if(!hasFlag)
   {
      if(mFlag.isValid())
      {
         if(!mFlag->isMounted())
            renderObjectiveArrow(mFlag, getTeamColor(mFlag));     // Arrow to flag, if not held by player
         else
         {
            Ship *mount = mFlag->getMount();
            if(mount)
               renderObjectiveArrow(mount, getTeamColor(mount));
         }
      }
   }
}


// What does a particular scoring event score?
S32 ZoneControlGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
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
         case CaptureZone:
         	return 1;
         case UncaptureZone:
            return -1;
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
         case CaptureZone:
         	return 1;
		 case UncaptureZone:
			return 0;
         default:
            logprintf("Unknown scoring event: %d", scoreEvent);
            return 0;
      }
   }
}


};

