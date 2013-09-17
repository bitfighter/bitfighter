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

#ifndef _LEVEL_SOURCE_H_
#define _LEVEL_SOURCE_H_

#include "GameTypesEnum.h"       // For GameTypeId

#include "tnlNetStringTable.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include <boost/shared_ptr.hpp>
#include <string>


using namespace TNL;
using namespace std;

namespace Zap
{

struct LevelInfo
{   
private:
   void initialize();      // Called by constructors

public:
   string filename;                 // File level is stored in
   string folder;                   // File's folder
   StringTableEntry mLevelName;     // Level "in-game" names
   
   GameTypeId mLevelType;      
   S32 minRecPlayers;               // Min recommended number of players for this level
   S32 maxRecPlayers;               // Max recommended number of players for this level

   LevelInfo();      // Default constructor used on server side

   // Constructor, used on client side where we don't care about min/max players
   LevelInfo(const StringTableEntry &name, GameTypeId type);

   // Constructor, used on server side, augmented with setInfo method below
   LevelInfo(const string &filename, const string &folder);
   virtual ~LevelInfo();

   const char *getLevelTypeName();
   void ensureLevelInfoHasValidName();
};


////////////////////////////////////////
////////////////////////////////////////


class GridDatabase;
class Game;

class LevelSource
{
protected:
   Vector<LevelInfo> mLevelInfos;   // Info about these levels

public:
   LevelSource();             // Constructor
   virtual ~LevelSource();    // Destructor

   S32 getLevelCount() const;
   LevelInfo getLevelInfo(S32 index);

   void remove(S32 index);    // Remove level from the list of levels

   pair<S32, bool> addLevel(LevelInfo levelInfo);   // Yes, pass by value

   // Extract info from specified level
   string     getLevelName(S32 index);
   string     getLevelFileName(S32 index);
   GameTypeId getLevelType(S32 index);

   virtual bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo) = 0;
   virtual string loadLevel(S32 index, Game *game, GridDatabase *gameObjDatabase) = 0;
   virtual string getLevelFileDescriptor(S32 index) const = 0;

   bool populateLevelInfoFromSource(const string &sourceName, S32 levelInfoIndex);

   static Vector<string> findAllLevelFilesInFolder(const string &levelDir);
   static void getLevelInfoFromCodeChunk(char *chunk, S32 size, LevelInfo &levelInfo);     // Populates levelInfo
};


////////////////////////////////////////
////////////////////////////////////////


struct FolderManager;

class FolderLevelSource : public LevelSource
{
   typedef LevelSource Parent;

public:
   FolderLevelSource(const Vector<string> &levelList, const string &folder);  // Constructor
   virtual ~FolderLevelSource();                                              // Destructor

   bool loadLevels(FolderManager *folderManager);
   string loadLevel(S32 index, Game *game, GridDatabase *gameObjDatabase);
   string getLevelFileDescriptor(S32 index) const;

   bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo);
};


////////////////////////////////////////
////////////////////////////////////////


// This LevelSource only has one level, whose code is stored in mLevelCode
class StringLevelSource : public LevelSource
{
   typedef LevelSource Parent;

private:
   string mLevelCode;

public:
   StringLevelSource(const string &levelCode);     // Constructor
   virtual ~StringLevelSource();                   // Destructor

   bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo);
};


////////////////////////////////////////
////////////////////////////////////////

typedef boost::shared_ptr<LevelSource> LevelSourcePtr;


}

#endif