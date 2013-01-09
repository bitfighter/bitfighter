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

#include "tnlLog.h"
#include "tnlDataChunker.h"
#include "../zap/oglconsole.h"   // For logging to the console
#include <time.h>
#include <string.h>
#include <stdio.h>               // For newer versions of gcc?
#include <stdarg.h>              // For va_list

#ifdef TNL_OS_ANDROID
#include <android/log.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

namespace TNL
{

////////////////////////////////////////
////////////////////////////////////////

LogConsumer *LogConsumer::mLinkedList = NULL;

// Constructor -- add log to consumer list
LogConsumer::LogConsumer()    
{
   //mFilterType = GeneralFilter;
   mMsgTypes = 0xFFFFFFFF;       // All types on by default

   mNextConsumer = mLinkedList;

   if(mNextConsumer)
      mNextConsumer->mPrevConsumer = this;

   mPrevConsumer = NULL;
   mLinkedList = this;
}

// Destructor -- remove log from consumer list
LogConsumer::~LogConsumer()
{
   if(mNextConsumer)
      mNextConsumer->mPrevConsumer = mPrevConsumer;

   if(mPrevConsumer)
      mPrevConsumer->mNextConsumer = mNextConsumer;
   else
      mLinkedList = mNextConsumer;
}


void LogConsumer::setMsgTypes(S32 types)
{
   mMsgTypes = types;
}


void LogConsumer::setMsgType(MsgType msgType, bool enable)
{
   if(enable)
      mMsgTypes |= msgType;
   else
      mMsgTypes &= ~msgType;
}


// Find all logs that are listenting to a specified MessageType and forward the message to them.  Static method.
void LogConsumer::logString(LogConsumer::MsgType msgType, std::string message)
{
   for(LogConsumer *walk = LogConsumer::getLinkedList(); walk; walk = walk->getNext())
      if(walk->mMsgTypes & msgType)     // Only log to the requested type of logfile
         walk->prepareAndLogString(message);
}


// Create reusable buffer for our logging functions.  Make it big because when we use datadumper 
// in a script, some messages can get very long
static char msg[1024 * 8];


void LogConsumer::logprintf(const char *format, ...)
{
   va_list args; 
   va_start(args, format); 

   vsnprintf(msg, sizeof(msg), format, args); 

   va_end(args);

   std::string message(msg);

   prepareAndLogString(message);
}


#ifndef TNL_DISABLE_LOGGING

// All logging should pass through this method -- disabling it via the ifdef should cause logging to not happen, but it's untested
void LogConsumer::prepareAndLogString(std::string message)
{
   // Unless string ends with a '\', add a newline char
   if(message.length() > 0 && message[message.length() - 1] == '\\')
      message.erase(message.length() - 1, 1);
   else
      message.append("\n");
#ifdef TNL_OS_ANDROID
   __android_log_print(ANDROID_LOG_DEBUG, "Bitfighter", message.c_str());
#else
   writeString(message.c_str());    // <== each log class will have it's own way of doing this
#endif
}

#else

void LogConsumer::prepareAndLogString(std::string message)
{
   // Do nothing
}

#endif

////////////////////////////////////////
////////////////////////////////////////

// Constructor -- open the file
FileLogConsumer::FileLogConsumer()    // Constructor
{
   // Do nothing
}


// Destructor -- close the file
FileLogConsumer::~FileLogConsumer()    
{
   if(f)
      fclose(f);
}

void FileLogConsumer::init(std::string logFile, const char *mode)
{
   if(f)
      fclose(f);

   f = fopen(logFile.c_str(), mode);
   if(!f)
      TNLAssert(false, "Can't open log file for writing!");    // TODO: What should we really do?
}


void FileLogConsumer::writeString(const char *string)
{
   if(f)
   {
      fprintf(f, "%s", string);
      fflush(f);
   }
   else
      TNLAssert(false, "Logfile not initialized!");
}


////////////////////////////////////////
////////////////////////////////////////

void StdoutLogConsumer::writeString(const char *string)
{
   printf("%s", string);
}


////////////////////////////////////////
////////////////////////////////////////


LogType *LogType::linkedList = NULL;
LogType *LogType::current = NULL;

#ifndef TNL_DISABLE_LOGGING

LogType *LogType::find(const char *name)
{
   static ClassChunker<LogType> logTypeChunker(4096);

   for(LogType *walk = linkedList; walk; walk = walk->next)
      if(!strcmp(walk->typeName, name))
         return walk;
   LogType *ret = logTypeChunker.alloc();
   ret->next = linkedList;
   linkedList = ret;
   ret->isEnabled = false;
   ret->typeName = name;
   return ret;
}
#endif


// Logs to logfiles that have subscribed to specified message type
void logprintf(LogConsumer::MsgType msgType, const char *format, ...)
{
   va_list args; 
   va_start(args, format); 

   vsnprintf(msg, sizeof(msg), format, args); 

   va_end(args);

   std::string message(msg);

   LogConsumer::logString(msgType, message);
}


// Logs to general log
void logprintf(const char *format, ...)
{
   va_list args; 
   va_start(args, format); 

   vsnprintf(msg, sizeof(msg), format, args); 

   va_end(args);

   std::string message(msg);

   LogConsumer::logString(LogConsumer::All, message);
}


// Return a nicely formatted date/time stamp
std::string getTimeStamp()
{
  static const U32 TIMESIZE = 40;
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[TIMESIZE];

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  strftime(buffer, TIMESIZE, "%Y-%m-%d %a %H:%M:%S", timeinfo);

  return(std::string(buffer));   
}


// Return a nicely formatted date/time stamp
std::string getShortTimeStamp()
{
  static const U32 TIMESIZE = 40;
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[TIMESIZE];

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  strftime(buffer, TIMESIZE, "%H:%M", timeinfo);

  return(std::string(buffer));     
}


};
