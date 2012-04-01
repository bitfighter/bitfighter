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

//#include "../lua/luaprofiler-2.0.2/src/luaprofiler.h"      // For... the profiler!
#include "oglconsole.h"

#ifndef ZAP_DEDICATED
#  include "UI.h"
#endif

#include <math.h>


#define hypot _hypot    // Kill some warnings

namespace Zap
{

// Statics:
bool EventManager::anyPending = false; 
Vector<lua_State *> EventManager::subscriptions[EventTypes];
Vector<lua_State *> EventManager::pendingSubscriptions[EventTypes];
Vector<lua_State *> EventManager::pendingUnsubscriptions[EventTypes];
bool EventManager::mConstructed = false;  // Prevent duplicate instantiation

// The list of the function names to be called in the bot when a particular event is fired
static const char *eventNames[] = {
   "Tick",
   "ShipSpawned",
   "ShipKilled",
   "PlayerJoined",
   "PlayerLeft",
   "MsgReceived",
   "NexusOpened",
   "NexusClosed"
};

static const char *eventFunctions[] = {
   "onTick",
   "onShipSpawned",
   "onShipKilled",
   "onPlayerJoined",
   "onPlayerLeft",
   "onMsgReceived",
   "onNexusOpened",
   "onNexusClosed"
};


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
   // Do nothing
}


// Provide access to the single EventManager instance
EventManager *EventManager::get()
{
   return &eventManager;
}


void EventManager::subscribe(lua_State *L, EventType eventType)
{
   // First, see if we're already subscribed
   if(isSubscribed(L, eventType) || isPendingSubscribed(L, eventType))
      return;

   // Make sure the script has the proper event listener
   lua_getglobal(L, eventFunctions[eventType]);

   if(!lua_isfunction(L, -1))
   {
      lua_pop(L, 1);    // Remove the item from the stack

      logprintf(LogConsumer::LogError, "Error subscribing to %s event: couldn't find handler function.  Unsubscribing.", eventNames[eventType]);
      OGLCONSOLE_Print("Error subscribing to %s event: couldn't find handler function.  Unsubscribing.", eventNames[eventType]);

      return;
   }

   removeFromPendingUnsubscribeList(L, eventType);
   pendingSubscriptions[eventType].push_back(L);
   anyPending = true;
}


void EventManager::unsubscribe(lua_State *L, EventType eventType)
{
   if((isSubscribed(L, eventType) || isPendingSubscribed(L, eventType)) && !isPendingUnsubscribed(L, eventType))
   {
      removeFromPendingSubscribeList(L, eventType);
      pendingUnsubscriptions[eventType].push_back(L);
      anyPending = true;
   }
}


void EventManager::removeFromPendingSubscribeList(lua_State *subscriber, EventType eventType)
{
   for(S32 i = 0; i < pendingSubscriptions[eventType].size(); i++)
      if(pendingSubscriptions[eventType][i] == subscriber)
      {
         pendingSubscriptions[eventType].erase_fast(i);
         return;
      }
}


void EventManager::removeFromPendingUnsubscribeList(lua_State *unsubscriber, EventType eventType)
{
   for(S32 i = 0; i < pendingUnsubscriptions[eventType].size(); i++)
      if(pendingUnsubscriptions[eventType][i] == unsubscriber)
      {
         pendingUnsubscriptions[eventType].erase_fast(i);
         return;
      }
}


void EventManager::removeFromSubscribedList(lua_State *subscriber, EventType eventType)
{
   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
      if(subscriptions[eventType][i] == subscriber)
      {
         subscriptions[eventType].erase_fast(i);
         return;
      }
}


// Unsubscribe an event bypassing the pending unsubscribe queue, when we know it will be OK
void EventManager::unsubscribeImmediate(lua_State *L, EventType eventType)
{
   removeFromSubscribedList(L, eventType);
   removeFromPendingSubscribeList(L, eventType);
   removeFromPendingUnsubscribeList(L, eventType);    // Probably not really necessary...
}


// Check if we're subscribed to an event
bool EventManager::isSubscribed(lua_State *L, EventType eventType)
{
   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
      if(subscriptions[eventType][i] == L)
         return true;

   return false;
}


bool EventManager::isPendingSubscribed(lua_State *L, EventType eventType)
{
   for(S32 i = 0; i < pendingSubscriptions[eventType].size(); i++)
      if(pendingSubscriptions[eventType][i] == L)
         return true;

   return false;
}


bool EventManager::isPendingUnsubscribed(lua_State *L, EventType eventType)
{
   for(S32 i = 0; i < pendingUnsubscriptions[eventType].size(); i++)
      if(pendingUnsubscriptions[eventType][i] == L)
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
   if(suppressEvents())   
      return;

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_State *L = subscriptions[eventType][i];
      try
      {
         lua_getglobal(L, eventFunctions[eventType]);

         if(lua_pcall(L, 0, 0, 0) != 0)
            throw LuaException(lua_tostring(L, -1));
      }
      catch(LuaException &e)
      {
         handleEventFiringError(L, eventType, e.what());
         return;
      }
   }
}


// onTick
void EventManager::fireEvent(EventType eventType, U32 deltaT)
{
   if(suppressEvents())   
      return;

   if(eventType == TickEvent)
      mStepCount--;   

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_State *L = subscriptions[eventType][i];
      
      try   
      {
         lua_getglobal(L, eventFunctions[eventType]);  
         lua_pushinteger(L, deltaT);

         if(lua_pcall(L, 1, 0, 0) != 0)
            throw LuaException(lua_tostring(L, -1));
      }
      catch(LuaException &e)
      {
         handleEventFiringError(L, eventType, e.what());
         return;
      }
   }
}


void EventManager::fireEvent(EventType eventType, Ship *ship)
{
   if(suppressEvents())   
      return;

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_State *L = subscriptions[eventType][i];
      try
      {
         lua_getglobal(L, eventFunctions[eventType]);
         ship->push(L);

         if(lua_pcall(L, 1, 0, 0) != 0)
            throw LuaException(lua_tostring(L, -1));
      }
      catch(LuaException &e)
      {
         handleEventFiringError(L, eventType, e.what());
         return;
      }
   }
}


void EventManager::fireEvent(lua_State *caller_L, EventType eventType, const char *message, LuaPlayerInfo *player, bool global)
{
   if(suppressEvents())   
      return;

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_State *L = subscriptions[eventType][i];

      if(L == caller_L)    // Don't alert bot about own message!
         continue;

      try
      {
         lua_getglobal(L, eventFunctions[eventType]);  
         lua_pushstring(L, message);
         player->push(L);
         lua_pushboolean(L, global);

         if(lua_pcall(L, 3, 0, 0) != 0)
            throw LuaException(lua_tostring(L, -1));
      }
      catch(LuaException &e)
      {
         handleEventFiringError(L, eventType, e.what());
         return;
      }
   }
}


// onPlayerJoined, onPlayerLeft
void EventManager::fireEvent(lua_State *caller_L, EventType eventType, LuaPlayerInfo *player)
{
   if(suppressEvents())   
      return;

   for(S32 i = 0; i < subscriptions[eventType].size(); i++)
   {
      lua_State *L = subscriptions[eventType][i];
      
      if(L == caller_L)    // Don't alert bot about own joinage or leavage!
         continue;

      try   
      {
         lua_getglobal(L, eventFunctions[eventType]);  
         player->push(L);

         if(lua_pcall(L, 1, 0, 0) != 0)
            throw LuaException(lua_tostring(L, -1));
      }
      catch(LuaException &e)
      {
         handleEventFiringError(L, eventType, e.what());
         return;
      }
   }
}


void EventManager::handleEventFiringError(lua_State *L, EventType eventType, const char *errorMsg)
{
   // Figure out which bot caused the error
   Robot *robot = Robot::findBot(L);

   if(robot)
      robot->logError( "Robot error handling event %s: %s. Shutting bot down.", eventNames[eventType], errorMsg);

   delete robot;
}


// If true, events will not fire!
bool EventManager::suppressEvents()
{
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
