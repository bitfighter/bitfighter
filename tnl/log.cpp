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
#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

namespace TNL
{

LogConsumer *LogConsumer::mLinkedList = NULL;

LogConsumer::LogConsumer()    // Constructor
{
   mFilterType = GeneralFilter;
   mNextConsumer = mLinkedList;
   if(mNextConsumer)
      mNextConsumer->mPrevConsumer = this;
   mPrevConsumer = NULL;
   mLinkedList = this;
}

LogConsumer::~LogConsumer()
{
   if(mNextConsumer)
      mNextConsumer->mPrevConsumer = mPrevConsumer;
   if(mPrevConsumer)
      mPrevConsumer->mNextConsumer = mNextConsumer;
   else
      mLinkedList = mNextConsumer;
}

void LogConsumer::setFilterType(FilterType type)
{
   mFilterType = type;
}

LogType *LogType::linkedList = NULL;
LogType *LogType::current = NULL;

#ifdef TNL_ENABLE_LOGGING

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

void LogConsumer::logString(const char *string)
{
   // by default the LogConsumer will output to the platform debug 
   // string printer, but only if we're in debug mode
   // this will be overridden by child classes
#ifdef TNL_DEBUG
   Platform::outputDebugString(string);
#endif
}

void logger(LogConsumer::FilterType filtertype, const char *format, void *args)
{
   char buffer[4096];
   U32 bufferStart = 0;
   if(LogType::current)
   {
      strncpy(buffer, LogType::current->typeName, sizeof(buffer));
      bufferStart = strlen(buffer);

      buffer[bufferStart] = ':';
      buffer[bufferStart+1] = ' ';
      bufferStart += 2;
   }

  // -1 below makes sure we have enough room for a "\n" if we need to append one
   vsnprintf(buffer + bufferStart, sizeof(buffer) - bufferStart - 1, format, (va_list) args);
   
   // If last char is a "\", chop it off, otherwise append newline
   U32 len = strlen(buffer);  // Should never be >= our buffer length, so appending newline should be ok

   if(len > 0 && buffer[len - 1] == '\\')
      buffer[len - 1] = '\0';    // Don't use NULL here, will cause type problems
   else
      strcat(buffer, "\n");

   for(LogConsumer *walk = LogConsumer::getLinkedList(); walk; walk = walk->getNext())
      if(walk->mFilterType == filtertype)     // Only log to the requested type of logfile
         walk->logString(buffer);

   Platform::outputDebugString(buffer);
}


void logprintf(const char *format, ...)
{
   va_list s;    
   va_start( s, format );

   logger(LogConsumer::GeneralFilter, format, s);
   va_end(s);
}

// Log a message to the server log --> wraps logger
void s_logprintf(const char *format, ...)
{
   va_list s;    
   va_start( s, format );

   logger(LogConsumer::ServerFilter, format, s);
   va_end(s);
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

  strftime(buffer, TIMESIZE, "%a %d-%m-%Y %H:%M:%S", timeinfo);

  return(std::string(buffer));      // Does this not seem a ridiculous use of strings??
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

  return(std::string(buffer));      // Does this not seem a ridiculous use of strings??
}


};
