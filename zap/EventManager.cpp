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

#include "EventManager.h"

#include "playerInfo.h"          // For RobotPlayerInfo constructor
#include "robot.h"
#include "Zone.h"

//#include "../lua/luaprofiler-2.0.2/src/luaprofiler.h"      // For... the profiler!

#ifndef ZAP_DEDICATED
#  include "UI.h"
#endif

#include <math.h>


#define hypot _hypot    // Kill some warnings

namespace Zap
{


struct Subscription {
   LuaScriptRunner *subscriber;
   LuaBase::ScriptContext context;
};


// Statics:
bool EventManager::anyPending = false; 
static Vector<Subscription>      subscriptions         [EventManager::EventTypes];
static Vector<Subscription>      pendingSubscriptions  [EventManager::EventTypes];
static Vector<LuaScriptRunner *> pendingUnsubscriptions[EventManager::EventTypes];

bool EventManager::mConstructed = false;  // Prevent duplicate instantiation


struct EventDef {
   const char *name;
   const char *function;
};


static const EventDef eventDefs[] = {
   // The following expands to a series of lines like this, based on values in EVENT_TABLE
   //   { "Tick",  "onTick"  },
#define EVENT(a, b, c) { b, c },
   EVENT_TABLE
#undef EVENT
};

static EventManager *eventManager = NULL;   // Singleton event manager, one copy is used by all listeners


// C++ constructor
EventManager::EventManager()
{
   TNLAssert(!mConstructed, "There is only one EventManager to rule them all!");

   mIsPaused = false;
   mStepCount = -1;
   mConstructed = true;
}


// Lua constructor
EventManager::EventManager(lua_State *L)
{
   TNLAssert(false, "Should never be called!");
}

// Destructor
EventManager::~EventManager()
{
   // Do nothing
}


void EventManager::shutdown()
{
   if(eventManager)
   {
      delete eventManager;
      eventManager = NULL;
   }
}


// Provide access to the single EventManager instance; lazily initialized
EventManager *EventManager::get()
{
   if(!eventManager)
      eventManager = new EventManager();      // Deleted in shutdown(), which is called from Game destuctor

   return eventManager;
}


void EventManager::subscribe(LuaScriptRunner *subscriber, EventType eventType, LuaBase::ScriptContext context, bool failSilently)
{
   // First, see if we're already subscribed
   if(isSubscribed(subscriber, eventType) || isPendingSubscribed(subscriber, eventType))
      return;

   lua_State *L = LuaScriptRunner::getL();

   // Make sure the script has the proper event listener
   bool ok = LuaScriptRunner::loadFunction(L, subscriber->getScriptId(), eventDefs[eventType].function);     // -- function

   if(!ok)
   {
      if(!failSilently)
         logprintf(LogConsumer::LogError, "Error subscribing to %s event: couldn't find handler function.  Unsubscribing.", 
                                          eventDefs[eventType].name);
      lua_pop(L, -1);    // Remove function from stack    

      return;
   }

   removeFromPendingUnsubscribeList(subscriber, eventType);

   Subscription s;
   s.subscriber = subscriber;
   s.context = context;

   pendingSubscriptions[eventType].push_back(s);
   anyPending = true;

   lua_pop(L, -1);    // Remove function from stack                                  -- <<empty stack>>
}


void EventManager::unsubscribe(LuaScriptRunner *subscriber, EventType eventType)
{
   if((isSubscribed(subscriber, eventType) || isPendingSubscribed(subscriber, eventType)) && !isPendingUnsubscribed(subscriber, eventType))
   {
      removeFromPendingSubscribeList(subscriber, eventType);

      pendingUnsubscriptions[eventType].push_back(subscriber);
      anyPending = true;
   }
}


void EventManager::removeFromPendingSubscribeList(LuaScriptRunner *subscriber, EventType eventType)
{
   for(S32 i = 0; i < pendingSubscriptions[eventType].size(); i++)
      if(pendingSubscriptions[eventType][i].subscriber == subscriber)
      {
         pendingSubscriptions[eventType].erase_fast(i);
         return;
      }
}


void EventManager::removeFromPendingUnsubscribeList(LuaScriptRunner *subscriber, EventType eventType)
{
   for(S32 i = 0; i < pendingUnsubscriptions[eventType].size(); i++)
      if(pendingUnsubscriptions[eventType][i] == subscriber)
      {
         pendingUnsubscriptions[eventType].erase_fast(i);
         return;
      }
}


void EventManager::removeFromSubscribedList(LuaScriptRunner *subscriber, EventType eventType)
{
   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
      if(subscriptions[eventType][i].subscriber == subscriber)
      {
         subscriptions[eventType].erase_fast(i);
         return;
      }
}


// Unsubscribe an event bypassing the pending unsubscribe queue, when we know it will be OK
void EventManager::unsubscribeImmediate(LuaScriptRunner *subscriber, EventType eventType)
{
   removeFromSubscribedList        (subscriber, eventType);
   removeFromPendingSubscribeList  (subscriber, eventType);
   removeFromPendingUnsubscribeList(subscriber, eventType);    // Probably not really necessary...
}


// Check if we're subscribed to an event
bool EventManager::isSubscribed(LuaScriptRunner *subscriber, EventType eventType)
{
   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
      if(subscriptions[eventType][i].subscriber == subscriber)
         return true;

   return false;
}


bool EventManager::isPendingSubscribed(LuaScriptRunner *subscriber, EventType eventType)
{
   for(S32 i = 0; i < pendingSubscriptions[eventType].size(); i++)
      if(pendingSubscriptions[eventType][i].subscriber == subscriber)
         return true;

   return false;
}


bool EventManager::isPendingUnsubscribed(LuaScriptRunner *subscriber, EventType eventType)
{
   for(S32 i = 0; i < pendingUnsubscriptions[eventType].size(); i++)
      if(pendingUnsubscriptions[eventType][i] == subscriber)
         return true;

   return false;
}


// Process all pending subscriptions and unsubscriptions
void EventManager::update()
{
   if(anyPending)
   {
      for(S32 i = 0; i < EventTypes; i++)
         for(S32 j = 0; j < pendingUnsubscriptions[i].size(); j++)     // Unsubscribing first means less searching!
            removeFromSubscribedList(pendingUnsubscriptions[i][j], (EventType) i);

      for(S32 i = 0; i < EventTypes; i++)
         for(S32 j = 0; j < pendingSubscriptions[i].size(); j++)     
            subscriptions[i].push_back(pendingSubscriptions[i][j]);

      for(S32 i = 0; i < EventTypes; i++)
      {
         pendingSubscriptions[i].clear();
         pendingUnsubscriptions[i].clear();
      }

      anyPending = false;
   }
}


// onNexusOpened, onNexusClosed
void EventManager::fireEvent(EventType eventType)
{
   if(suppressEvents(eventType))   
      return;

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
      fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
}


// onTick
void EventManager::fireEvent(EventType eventType, U32 deltaT)
{
   if(suppressEvents(eventType))   
      return;

   if(eventType == TickEvent)
      mStepCount--;   

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_pushinteger(L, deltaT);   // -- deltaT
      fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
   }
}


// onShipSpawned, onShipKilled
void EventManager::fireEvent(EventType eventType, Ship *ship)
{
   if(suppressEvents(eventType))   
      return;

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      ship->push(L);                // -- ship
      fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
   }
}


// Note that player can be NULL, in which case we'll pass nil to the listeners
// callerId will be NULL when player sends message
void EventManager::fireEvent(LuaScriptRunner *sender, EventType eventType, const char *message, LuaPlayerInfo *playerInfo, bool global)
{
   if(suppressEvents(eventType))   
      return;

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      if(sender == subscriptions[eventType][i].subscriber)    // Don't alert sender about own message!
         continue;

      lua_pushstring(L, message);   // -- message

      if(playerInfo)
         playerInfo->push(L);       // -- message, playerInfo
      else
         lua_pushnil(L);            

      lua_pushboolean(L, global);   // -- message, player, isGlobal

      fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
   }
}


// onPlayerJoined, onPlayerLeft, onPlayerTeamChanged
void EventManager::fireEvent(LuaScriptRunner *player, EventType eventType, LuaPlayerInfo *playerInfo)
{
   if(suppressEvents(eventType))   
      return;

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      if(player == subscriptions[eventType][i].subscriber)    // Don't trouble player with own joinage or leavage!
         continue;

      playerInfo->push(L);          // -- playerInfo
      fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
   }
}


// onShipEnteredZone, onShipLeftZone
void EventManager::fireEvent(EventType eventType, Ship *ship, Zone *zone)
{
   if(suppressEvents(eventType))   
      return;

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      try   
      {
         // Passing ship, zone, zoneType, zoneId
         ship->push(L);                                     // -- ship
         zone->push(L);                                     // -- ship, zone   
         lua_pushinteger(L, zone->getObjectTypeNumber());   // -- ship, zone, zone->objTypeNumber
         lua_pushinteger(L, zone->getUserAssignedId());     // -- ship, zone, zone->objTypeNumber, zone->id

         fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
      }
      catch(LuaException &e)
      {
         handleEventFiringError(L, subscriptions[eventType][i], eventType, e.what());
         LuaBase::clearStack(L);
         return;
      }
   }
}


// onScoreChanged
void EventManager::fireEvent(EventType eventType, S32 score, S32 team, LuaPlayerInfo *playerInfo)
{
   if(suppressEvents(eventType))
         return;

   lua_State *L = LuaScriptRunner::getL();

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_pushinteger(L, score);   // -- score
      lua_pushinteger(L, team);    // -- score, team

      if(playerInfo)
         playerInfo->push(L);      // -- score, team, playerInfo
      else
         lua_pushnil(L);

      fire(L, subscriptions[eventType][i].subscriber, eventDefs[eventType].function, subscriptions[eventType][i].context);
   }
}


// Actually fire the event, called by one of the fireEvent() methods above
// Returns true if there was an error, false if everything ran ok
bool EventManager::fire(lua_State *L, LuaScriptRunner *scriptRunner, const char *function, LuaBase::ScriptContext context)
{
   LuaBase::setScriptContext(L, context);
   return scriptRunner->runCmd(function, 0);
}


void EventManager::handleEventFiringError(lua_State *L, const Subscription &subscriber, EventType eventType, const char *errorMsg)
{
   if(subscriber.context == LuaBase::RobotContext)
   {
      subscriber.subscriber->logError("Error handling event %s: %s. Shutting bot down.", eventDefs[eventType].name, errorMsg);
      delete subscriber.subscriber;
   }
   else
   {
      // It was a levelgen
      logprintf(LogConsumer::LogError, "Error firing event %s: %s", eventDefs[eventType].name, errorMsg);
   }

   LuaBase::clearStack(L);
}


// If true, events will not fire!
bool EventManager::suppressEvents(EventType eventType)
{
   if(subscriptions[eventType].size() == 0)
      return true;

   return mIsPaused && mStepCount <= 0;    // Paused bots should still respond to events as long as stepCount > 0
}


void EventManager::setPaused(bool isPaused)
{
   mIsPaused = isPaused;
}


void EventManager::togglePauseStatus()
{
   mIsPaused = !mIsPaused;
}


bool EventManager::isPaused()
{
   return mIsPaused;
}


// Each firing of TickEvent is considered a step
void EventManager::addSteps(S32 steps)
{
   if(mIsPaused)           // Don't add steps if not paused to avoid hitting pause and having bot still run a few steps
      mStepCount = steps;     
}


};
