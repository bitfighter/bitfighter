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

#include "tnl.h"
#include "tnlAssert.h"
#include <stdarg.h>
#include "tnlLog.h"

namespace TNL {

#ifdef TNL_ENABLE_ASSERTS

//-------------------------------------- STATIC Declaration
static bool processing = false;

//--------------------------------------
static bool displayMessageBox(const char *title, const char *message, bool retry)
{
   if (retry)
      return Platform::AlertRetry(title, message);

   Platform::AlertOK(title, message);
   return false;
}   

//--------------------------------------
void Assert::processAssert(const char *filename, U32 lineNumber, const char  *message)
{
   processing = true;

   char buffer[2048];
   dSprintf(buffer, sizeof(buffer), "Fatal: (%s: %ld)", filename, lineNumber);
#ifdef TNL_DEBUG
   // In debug versions, allow a retry even for ISVs...
   bool retry = displayMessageBox(buffer, message, true);
#else
   bool retry = displayMessageBox(buffer, message, ((assertType == Fatal) ? true : false) );
#endif
   if (!retry)
      Platform::forceQuit();

   processing = false;
}

bool Assert::processingAssert()
{
   return processing;
}

#endif

//--------------------------------------
const char* avar(const char *message, ...)
{
   static char buffer[4096];
   va_list args;
   va_start(args, message);
   dVsprintf(buffer, sizeof(buffer), message, args);
   return( buffer );
}

};
