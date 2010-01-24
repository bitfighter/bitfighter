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

namespace TNL
{

/// Global interface to the TNL logging system.
///
/// The TNL logging system is consumer based. This is just a
/// global stub that routes the log string to all the consumers.
///
/// You should <b>really</b> be using the TNLLogMessage() and
/// TNLLogMessageV() macros instead of calling this.
///
/// @see LogConsumer
extern void logprintf(const char *format, ...);
extern void s_logprintf(const char *format, ...);
extern std::string getTimeStamp();
extern std::string getShortTimeStamp();

/// LogConsumer is the base class for the message logging system in TNL.
///
/// TNL by default doesn't log messages anywhere, but users of the library
/// can instantiate subclasses that override the logString method.
/// Any instantiated subclass of LogConsumer will receive all general
/// logprintf's, as well as any TNLLogMessages that are enabled via
/// the TNLLogEnable macro.
class LogConsumer
{
   LogConsumer *mNextConsumer; ///< Next LogConsumer in the global linked list of log consumers.
   LogConsumer *mPrevConsumer; ///< Previous LogConsumer in the global linked list of log consumers.

   static LogConsumer *mLinkedList;       ///< Head of the global linked list of log consumers.

public:

   enum FilterType {
      GeneralFilter,          // For logging general messages
      ServerFilter,           // For logging messages specific to the server
   };

   /// Constructor adds this LogConsumer to the global linked list.
   LogConsumer();

   /// Destructor removes this LogConsumer from the global linked list, and updates the log flags.
   virtual ~LogConsumer();

   /// Set the consumer type, that controls which loggers get written to
   void setFilterType(FilterType type);


   /// Returns the head of the linked list of all log consumers.
   static LogConsumer *getLinkedList() { return mLinkedList; }

   /// Returns the next LogConsumer in the linked list.
   LogConsumer *getNext() { return mNextConsumer; }

   FilterType mFilterType;

   /// Writes a string to this instance of LogConsumer.
   ///
   /// By default the string is sent to the Platform::outputDebugString function. Subclasses
   /// might log to a file, a remote service, or even a message box.
   virtual void logString(const char *string);
};

struct LogType
{
   LogType *next;
   static LogType *linkedList;

   bool isEnabled;
   const char *typeName;
   static LogType *current;

#ifdef TNL_ENABLE_LOGGING
   static void setCurrent(LogType *theType) { current = theType; }
   static LogType *find(const char *name);
#endif
};

#ifdef TNL_ENABLE_LOGGING

struct LogTypeRef
{
   LogType *theLogType;
   LogTypeRef(const char *name)
   {
      theLogType = LogType::find(name);
   }
};

///   LogConnectionProtocol,
///   LogNetConnection,
///   LogEventConnection,
///   LogGhostConnection,
///   LogNetInterface,
///   LogPlatform,

/// Logs a message of the specified type to the currently active LogConsumers.
#define TNLLogMessage(logType, message) { static TNL::LogTypeRef theType(#logType); if(theType.theLogType->isEnabled) { TNL::LogType::setCurrent(theType.theLogType); logprintf("%s", message); TNL::LogType::setCurrent(NULL); } }

/// Logs a printf-style variable argument message of the specified type to the currently active LogConsumers.
#define TNLLogMessageV(logType, message) { static TNL::LogTypeRef theType(#logType); if(theType.theLogType->isEnabled) { TNL::LogType::setCurrent(theType.theLogType); logprintf message; TNL::LogType::setCurrent(NULL); } }

#define TNLLogEnable(logType, enabled) { static TNL::LogTypeRef theType(#logType); theType.theLogType->isEnabled = enabled; }

#define TNLLogBlock(logType, code) { static TNL::LogTypeRef theType(#logType); if(theType.theLogType->isEnabled) { TNL::LogType::setCurrent(theType.theLogType); { code } TNL::LogType::setCurrent(NULL); } }

#else
#define TNLLogMessage(logType, message)  { }
#define TNLLogMessageV(logType, message) { }
#define TNLLogEnable(logType, enabled) { }
#define TNLLogBlock(logType, code) { }
#endif

};

#endif
