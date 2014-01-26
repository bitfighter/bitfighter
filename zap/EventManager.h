//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_

#include "LuaBase.h"    // For ScriptContext def

#include "tnlTypes.h"
#include "tnlVector.h"


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
 * @luaenum Event(1,3)
 * The Event enum represents different events that the game fires that scripts might want to handle.  Below
 * are the signatures of methods you can implement to respond to these events.  Note that you will also have
 * to subscribe to the event to be notified of it.
 *
 * See the \e subscribe methods for \link Robot::subscribe bots\endlink and \link LuaLevelGenerator::subscribe levelGens \endlink, and the 
 * \e Events section of the scripting overview page.
 */

// See http://stackoverflow.com/questions/6635851/real-world-use-of-x-macros
//          Enum                 Name                 Lua event handler      Lua event handler signature (documentation only)
#define EVENT_TABLE \
   EVENT(TickEvent,              "Tick",              "onTick",              "onTick()"                                                               ) \
   EVENT(ShipSpawnedEvent,       "ShipSpawned",       "onShipSpawned",       "onShipSpawned(Ship ship)"                                               ) \
   EVENT(ShipKilledEvent,        "ShipKilled",        "onShipKilled",        "onShipKilled(Ship ship, BfObject damagingObject, BfObject shooter)"     ) \
   EVENT(PlayerJoinedEvent,      "PlayerJoined",      "onPlayerJoined",      "onPlayerJoined(PlayerInfo player)"                                      ) \
   EVENT(PlayerLeftEvent,        "PlayerLeft",        "onPlayerLeft",        "onPlayerLeft(PlayerInfo player)"                                        ) \
   EVENT(PlayerTeamChangedEvent, "PlayerTeamChanged", "onPlayerTeamChanged", "onPlayerTeamChanged(PlayerInfo player)"                                 ) \
   EVENT(MsgReceivedEvent,       "MsgReceived",       "onMsgReceived",       "onMsgReceived(string message, PlayerInfo sender, bool messageIsGlobal)" ) \
   EVENT(NexusOpenedEvent,       "NexusOpened",       "onNexusOpened",       "onNexusOpened()"                                                        ) \
   EVENT(NexusClosedEvent,       "NexusClosed",       "onNexusClosed",       "onNexusClosed()"                                                        ) \
   EVENT(ShipEnteredZoneEvent,   "ShipEnteredZone",   "onShipEnteredZone",   "onShipEnteredZone(Ship ship, Zone zone)"                                ) \
   EVENT(ShipLeftZoneEvent,      "ShipLeftZone",      "onShipLeftZone",      "onShipLeftZone(Ship ship, Zone zone)"                                   ) \
   EVENT(ScoreChangedEvent,      "ScoreChanged",      "onScoreChanged",      "onScoreChanged(num scoreChange, num teamIndex, PlayerInfo player)"      ) \
   EVENT(GameOverEvent,          "GameOver",          "onGameOver",          "onGameOver()"                                                           ) \

public:

// Define an enum from the first values in EVENT_TABLE
enum EventType {
#define EVENT(a, b, c, d) a,
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
   bool fire(lua_State *L, LuaScriptRunner *scriptRunner, const char *function, ScriptContext context);
      
   bool mIsPaused;
   S32 mStepCount;           // If running for a certain number of steps, this will be > 0, while mIsPaused will be true
   static bool mConstructed;

public:
   EventManager();                       // C++ constructor
   explicit EventManager(lua_State *L);  // Lua Constructor
   virtual ~EventManager();

   static void shutdown();

   static EventManager *get();         // Provide access to the single EventManager instance
   bool suppressEvents(EventType eventType);

   //static Vector<Subscription> subscriptions[EventTypes];
   //static Vector<Subscription> pendingSubscriptions[EventTypes];
   //static Vector<pendingUnsubscriptions *> pendingUnsubscriptions[EventTypes];
   static bool anyPending;

   void subscribe  (LuaScriptRunner *subscriber, EventType eventType, ScriptContext context, bool failSilently = false);
   void unsubscribe(LuaScriptRunner *subscriber, EventType eventType);

    // Used when bot dies, and we know there won't be subscription conflicts
   void unsubscribeImmediate(LuaScriptRunner *subscriber, EventType eventType); 
   void update();                                                      // Act on events sitting in the pending lists

   // We'll have several different signatures for this one...
   void fireEvent(EventType eventType);
   void fireEvent(EventType eventType, U32 deltaT);      // Tick
   void fireEvent(EventType eventType, Ship *ship);      // ShipSpawned
   void fireEvent(EventType eventType, Ship *ship, BfObject *damagingObject, BfObject *shooter);  // ShipKilled
   void fireEvent(LuaScriptRunner *sender, EventType eventType, const char *message, LuaPlayerInfo *playerInfo, bool global);  // MsgReceived
   void fireEvent(LuaScriptRunner *player, EventType eventType, LuaPlayerInfo *playerInfo);  // PlayerJoined, PlayerLeft, PlayerTeamChanged
   void fireEvent(EventType eventType, Ship *ship, Zone *zone); // ShipEnteredZoneEvent, ShipLeftZoneEvent
   void fireEvent(EventType eventType, S32 score, S32 team, LuaPlayerInfo *playerInfo);

   // Allow the pausing of event firing for debugging purposes
   void setPaused(bool isPaused);
   void togglePauseStatus();
   bool isPaused();
   void addSteps(S32 steps);        // Each robot will cause the step counter to decrement
};


};

#endif
