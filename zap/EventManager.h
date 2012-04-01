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

#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_

#include "tnlTypes.h"
#include "tnlVector.h"
#include "lua.h"

using namespace TNL;

namespace Zap
{

class Robot;
class Ship;
class LuaPlayerInfo;


class EventManager
{
public:
   // Need to keep synced with eventFunctions!
   enum EventType {
      TickEvent = 0,          // (time) --> Standard game tick event
      ShipSpawnedEvent,       // (ship) --> Ship (or robot) spawns
      ShipKilledEvent,        // (ship) --> Ship (or robot) is killed
      PlayerJoinedEvent,      // (playerInfo) --> Player joined game
      PlayerLeftEvent,        // (playerInfo) --> Player left game
      MsgReceivedEvent,       // (message, sender-player, public-bool) --> Chat message sent
      NexusOpenedEvent,       // () --> Nexus opened (nexus games only)
      NexusClosedEvent,       // () --> Nexus closed (nexus games only)
      EventTypes
   };

private:
   // Some helper functions
   bool isSubscribed(lua_State *L, EventType eventType);
   bool isPendingSubscribed(lua_State *L, EventType eventType);
   bool isPendingUnsubscribed(lua_State *L, EventType eventType);

   void removeFromSubscribedList(lua_State *L, EventType eventType);
   void removeFromPendingSubscribeList(lua_State *subscriber, EventType eventType);
   void removeFromPendingUnsubscribeList(lua_State *unsubscriber, EventType eventType);

   void handleEventFiringError(lua_State *L, EventType eventType, const char *errorMsg);

   bool mIsPaused;
   S32 mStepCount;           // If running for a certain number of steps, this will be > 0, while mIsPaused will be true
   static bool mConstructed;

public:
   EventManager();                     // C++ constructor
   EventManager(lua_State *L);         // Lua Constructor

   static EventManager *get();         // Provide access to the single EventManager instance
   bool suppressEvents();

   static Vector<lua_State *> subscriptions[EventTypes];
   static Vector<lua_State *> pendingSubscriptions[EventTypes];
   static Vector<lua_State *> pendingUnsubscriptions[EventTypes];
   static bool anyPending;

   void subscribe(lua_State *L, EventType eventType);
   void unsubscribe(lua_State *L, EventType eventType);
   void unsubscribeImmediate(lua_State *L, EventType eventType);     // Used when bot dies, and we know there won't be subscription conflicts
   void update();                                                    // Act on events sitting in the pending lists

   // We'll have several different signatures for this one...
   void fireEvent(EventType eventType);
   void fireEvent(EventType eventType, U32 deltaT);      // Tick
   void fireEvent(EventType eventType, Ship *ship);      // ShipSpawned, ShipKilled
   void fireEvent(lua_State *L, EventType eventType, const char *message, LuaPlayerInfo *player, bool global);     // MsgReceived
   void fireEvent(lua_State *L, EventType eventType, LuaPlayerInfo *player);  // PlayerJoined, PlayerLeft

   // Allow the pausing of event firing for debugging purposes
   void setPaused(bool isPaused);
   void togglePauseStatus();
   bool isPaused();
   void addSteps(S32 steps);        // Each robot will cause the step counter to decrement
};


};

#endif