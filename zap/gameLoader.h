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

#ifndef _GAMELOADER_H_
#define _GAMELOADER_H_

#include "tnl.h"
#include "tnlNetStringTable.h"
#include "tnlVector.h"

using namespace TNL;

namespace Zap
{

class LevelLoader
{
public:
   // Put these in here so we can access them from luaLevelGenerator
   const static int MaxArgc = 128;    // At most MaxArgc args on a single line,
   const static int MaxArgLen = 100;  // Each at most MaxArgLen bytes long  (enforced in addCharToArg)
   const static int MaxIdLen = 11;    // Max 32-bit int is 10 digits, plus room for a null

   bool initLevelFromFile(const char *file);
   virtual void processLevelLoadLine(U32 argc, U32 id, const char **argv) = NULL;
   int parseArgs(const char *string);
};

// Provide a class to help organize loading of levels from disk
class LevelListLoader
{
public:
   static void buildLevelList();
};


};

#endif


