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
class Zone;

class EventManager
{
// See http://stackoverflow.com/questions/6635851/real-world-use-of-x-macros
//          Enum                 Name               Lua event handler 
#define EVENT_TABLE \
   EVENT(TickEvent,             "Tick",            "onTick"            ) \
   EVENT(ShipSpawnedEvent,      "ShipSpawned",     "onShipSpawned"     ) \
   EVENT(ShipKilledEvent,       "ShipKilled",      "onShipKilled"      ) \
   EVENT(PlayerJoinedEvent,     "PlayerJoined",    "onPlayerJoined"    ) \
   EVENT(PlayerLeftEvent,       "PlayerLeft",      "onPlayerLeft"      ) \
   EVENT(MsgReceivedEvent,      "MsgReceived",     "onMsgReceived"     ) \
   EVENT(NexusOpenedEvent,      "NexusOpened",     "onNexusOpened"     ) \
   EVENT(NexusClosedEvent,      "NexusClosed",     "onNexusClosed"     ) \
   EVENT(ShipEnteredZoneEvent,  "ShipEnteredZone", "onShipEnteredZone" ) \
   EVENT(ShipLeftZoneEvent,     "ShipLeftZone",    "onShipLeftZone"    ) \

public:

// Define an enum from the first values in EVENT_TABLE
enum EventType {
#define EVENT(a, b, c) a,
    EVENT_TABLE
#undef EVENT
    EventTypes
};


private:
   // Some helper functions
   bool isSubscribed(const char *subscriber, EventType eventType);
   bool isPendingSubscribed(const char *subscriber, EventType eventType);
   bool isPendingUnsubscribed(const char *subscriber, EventType eventType);

   void removeFromSubscribedList(const char *subscriber, EventType eventType);
   void removeFromPendingSubscribeList(const char *subscriber, EventType eventType);
   void removeFromPendingUnsubscribeList(const char *subscriber, EventType eventType);

   void handleEventFiringError(lua_State *L, const char *subscriber, EventType eventType, const char *errorMsg);

   bool mIsPaused;
   S32 mStepCount;           // If running for a certain number of steps, this will be > 0, while mIsPaused will be true
   static bool mConstructed;

public:
   EventManager();                     // C++ constructor
   EventManager(lua_State *L);         // Lua Constructor

   static void shutdown();

   static EventManager *get();         // Provide access to the single EventManager instance
   bool suppressEvents(EventType eventType);

   static Vector<const char *> subscriptions[EventTypes];
   static Vector<const char *> pendingSubscriptions[EventTypes];
   static Vector<const char *> pendingUnsubscriptions[EventTypes];
   static bool anyPending;

   void subscribe(const char *subscriber, EventType eventType, bool failSilently = false);
   void unsubscribe(const char *subscriber, EventType eventType);
   void unsubscribeImmediate(const char *, EventType eventType);     // Used when bot dies, and we know there won't be subscription conflicts
   void update();                                                    // Act on events sitting in the pending lists

   // We'll have several different signatures for this one...
   void fireEvent(EventType eventType);
   void fireEvent(EventType eventType, U32 deltaT);      // Tick
   void fireEvent(EventType eventType, Ship *ship);      // ShipSpawned, ShipKilled
   void fireEvent(const char *callerId, EventType eventType, const char *message, LuaPlayerInfo *player, bool global);     // MsgReceived
   void fireEvent(const char *callerId, EventType eventType, LuaPlayerInfo *player);  // PlayerJoined, PlayerLeft
   void fireEvent(EventType eventType, Ship *ship, Zone *zone);      // ShipEnteredZoneEvent, ShipLeftZoneEvent

   // Allow the pausing of event firing for debugging purposes
   void setPaused(bool isPaused);
   void togglePauseStatus();
   bool isPaused();
   void addSteps(S32 steps);        // Each robot will cause the step counter to decrement
};


};

#endif