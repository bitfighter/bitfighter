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
#include "LuaBase.h"    // For ScriptContext def

using namespace TNL;

namespace Zap
{

class Robot;
class Ship;
class LuaPlayerInfo;
class LuaScriptRunner;
class Zone;

struct Subscription; 

class EventManager
{
/**
 * @luaenum Event(1,1)
 * The Event enum represents different events that the game fires that scripts might want to handle.
 */

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
   bool isSubscribed         (LuaScriptRunner *subscriber, EventType eventType);
   bool isPendingSubscribed  (LuaScriptRunner *subscriber, EventType eventType);
   bool isPendingUnsubscribed(LuaScriptRunner *subscriber, EventType eventType);

   void removeFromSubscribedList        (LuaScriptRunner *subscriber, EventType eventType);
   void removeFromPendingSubscribeList  (LuaScriptRunner *subscriber, EventType eventType);
   void removeFromPendingUnsubscribeList(LuaScriptRunner *subscriber, EventType eventType);

   void handleEventFiringError(lua_State *L, const Subscription &subscriber, EventType eventType, const char *errorMsg);
   bool fire(lua_State *L, LuaScriptRunner *scriptRunner, const char *function, LuaBase::ScriptContext context);
      
   bool mIsPaused;
   S32 mStepCount;           // If running for a certain number of steps, this will be > 0, while mIsPaused will be true
   static bool mConstructed;

public:
   EventManager();                       // C++ constructor
   explicit EventManager(lua_State *L);  // Lua Constructor

   static void shutdown();

   static EventManager *get();         // Provide access to the single EventManager instance
   bool suppressEvents(EventType eventType);

   //static Vector<Subscription> subscriptions[EventTypes];
   //static Vector<Subscription> pendingSubscriptions[EventTypes];
   //static Vector<pendingUnsubscriptions *> pendingUnsubscriptions[EventTypes];
   static bool anyPending;

   void subscribe  (LuaScriptRunner *subscriber, EventType eventType, LuaBase::ScriptContext context, bool failSilently = false);
   void unsubscribe(LuaScriptRunner *subscriber, EventType eventType);

    // Used when bot dies, and we know there won't be subscription conflicts
   void unsubscribeImmediate(LuaScriptRunner *subscriber, EventType eventType); 
   void update();                                                      // Act on events sitting in the pending lists

   // We'll have several different signatures for this one...
   void fireEvent(EventType eventType);
   void fireEvent(EventType eventType, U32 deltaT);      // Tick
   void fireEvent(EventType eventType, Ship *ship);      // ShipSpawned, ShipKilled
   void fireEvent(LuaScriptRunner *sender, EventType eventType, const char *message, LuaPlayerInfo *playerInfo, bool global);  // MsgReceived
   void fireEvent(LuaScriptRunner *player, EventType eventType, LuaPlayerInfo *playerInfo);  // PlayerJoined, PlayerLeft
   void fireEvent(EventType eventType, Ship *ship, Zone *zone); // ShipEnteredZoneEvent, ShipLeftZoneEvent

   // Allow the pausing of event firing for debugging purposes
   void setPaused(bool isPaused);
   void togglePauseStatus();
   bool isPaused();
   void addSteps(S32 steps);        // Each robot will cause the step counter to decrement
};


};

#endif
