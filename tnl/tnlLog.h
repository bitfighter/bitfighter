//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _TNL_LOG_H_
#define _TNL_LOG_H_

#include "tnlBitSet.h"
#include "tnl.h"
#include <string.h>
#include <stdarg.h>

namespace TNL
{
///   
/// LogConsumer is the base class for the message logging system in TNL.
///
/// TNL by default doesn't log messages anywhere, but users of the library
/// can instantiate subclasses that override the logString method.
/// Any instantiated subclass of LogConsumer will receive all general
/// logprintf's, as well as any TNLLogMessages that are enabled via
/// the setMsgType() and setMsgTypes() functions.
///
   
class LogConsumer
{
private:
   LogConsumer *mNextConsumer;         ///< Next LogConsumer in the global linked list of log consumers.
   LogConsumer *mPrevConsumer;         ///< Previous LogConsumer in the global linked list of log consumers.

   static LogConsumer *mLinkedList;    ///< Head of the global linked list of log consumers.

public:
   enum MsgType {   
      LogNone                    = 0,

      // Error logging
      LogFatalError           = BIT(0),      // Log fatal errors; should be left on
      LogError                = BIT(1),      // Log serious errors; should be left on
      LogWarning              = BIT(2),      // Log less serious errors
 
      LogConnection           = BIT(3),      // High level logging connections with remote machines

      // Master server events
      LogConnectionManager    = BIT(4),      // Log server attempts to manage connections between clients and servers
      LogChat                 = BIT(5),      // Log global chat messages relayed through master

      // TNL network events
      LogConnectionProtocol	= BIT(6),      // Details about packets sent/recv'd
      LogNetConnection        = BIT(7),      // Packet send/rcv info
      LogEventConnection      = BIT(8),      // Event connection info
      LogGhostConnection      = BIT(9),      // Info about ghosting
      LogNetInterface		   = BIT(10),     // Higher level network events such as connection attempts and the like
      LogPlatform             = BIT(11),     // Log message in lieu of showing message to user on non-Windows platforms; only used for Asserts
      LogNetBase              = BIT(12),     // Info about network object classes
      LogUDP                  = BIT(13),     // Logs UDP socket bindings and params

      LogLevelLoaded          = BIT(14),     // When a level is loaded

      LogLuaObjectLifecycle   = BIT(15),     // Creation and destruciton of lua objects
      LuaLevelGenerator       = BIT(16),     // Messages from the LuaLevelGenerator     
      LuaBotMessage           = BIT(17),     // Message from a bot, to go to lua msg console

      ServerFilter            = BIT(18),     // For logging messages specific to hosting games
      StatisticsFilter        = BIT(19),     // For logging player/game statistics

      DatabaseFilter          = BIT(20),     // For logging issues with writing to database
      ConfigurationError      = BIT(21),     // For logging configuation issues

      LogLevelError           = BIT(22),     // Logs errors and warnings in levels
      ConsoleMsg              = BIT(23),     // Message that goes only to the console
      
      All = 0xFFFFFFFF,
      AllErrorTypes = LogFatalError | LogError | LogWarning | LogLevelError | ConfigurationError
   };


   /// Constructor adds this LogConsumer to the global linked list.
   LogConsumer();

   /// Destructor removes this LogConsumer from the global linked list, and updates the log flags.
   virtual ~LogConsumer();

   /// Set the consumer type, that controls which loggers get written to
   void setMsgTypes(S32 types);                    // Set one or more types or'ed together
   void setMsgType(MsgType msgType, bool enable);  // Enable or disable a single type


   /// Returns the head of the linked list of all log consumers.
   static LogConsumer *getLinkedList() { return mLinkedList; }

   /// Returns the next LogConsumer in the linked list.
   LogConsumer *getNext() { return mNextConsumer; }


   void logprintf(const char *format, ...);   // Writes a string to this instance of LogConsumer, bypassing all filtering

   static void logString(LogConsumer::MsgType msgType, std::string message);

private:
   S32 mMsgTypes;    // A bitmap of MsgType values
   void prepareAndLogString(std::string message);
   virtual void writeString(const char *string) = 0;
};


////////////////////////////////////////
////////////////////////////////////////

class FileLogConsumer : public LogConsumer    // Dumps logs to file
{
protected:
   FILE *f;

public:
   FileLogConsumer();      // Constructor
   ~FileLogConsumer();     // Destructor

   void init(std::string logFile, const char *mode = "a");

private:
   void writeString(const char *string);
}; 


////////////////////////////////////////
////////////////////////////////////////

class StdoutLogConsumer : public LogConsumer    // Dumps to stdout
{
private:
   void writeString(const char *string);
};


////////////////////////////////////////
////////////////////////////////////////


struct LogType
{
   LogType *next;
   static LogType *linkedList;

   bool isEnabled;
   const char *typeName;
   static LogType *current;

#ifndef TNL_DISABLE_LOGGING
   static void setCurrent(LogType *theType) { current = theType; }
   static LogType *find(const char *name);
#endif
};


/// Global interface to the TNL logging system.
///
/// The TNL logging system is consumer based. This is just a
/// global stub that routes the log string to all the consumers.
///
/// It is best to specify a message type when using logprintf... using the form without a message type will write the message to all
/// logs, and should only be used for short-term debug logging.
///
/// @see LogConsumer
extern void logprintf(LogConsumer::MsgType msgType, const char *format, ...);
extern void logprintf(const char *format, ...);

extern std::string getTimeStamp();
extern std::string getShortTimeStamp();

};

#endif
