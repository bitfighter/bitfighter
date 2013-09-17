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

#include "LevelSource.h"

#include "config.h"           // For FolderManager
#include "gameType.h"
#include "GameSettings.h"

#include "md5wrapper.h"
#include "stringUtils.h"

#include "tnlAssert.h"


namespace Zap
{

// Constructor
LevelInfo::LevelInfo()      
{
   initialize();
}


// Constructor, only used on client
LevelInfo::LevelInfo(const StringTableEntry &name, GameTypeId type)
{
   initialize();

   mLevelName = name;  
   mLevelType = type; 
}


// Constructor
LevelInfo::LevelInfo(const string &filename, const string &folder)
{
   initialize();

   this->filename = filename;
   this->folder   = folder;
}


// Destructor
LevelInfo::~LevelInfo()
{
   // Do nothing
}


void LevelInfo::initialize()
{
   mLevelName = "";
   mLevelType = BitmatchGame;
   filename = "";
   folder = "";
   minRecPlayers = 0;
   maxRecPlayers = 0;
}


const char *LevelInfo::getLevelTypeName()
{
   return GameType::getGameTypeName(mLevelType);
}


// Provide a default name if name is blank
void LevelInfo::ensureLevelInfoHasValidName()
{
   if(mLevelName == "")
      mLevelName = filename;   
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
LevelSource::LevelSource()
{
   // Do nothing
}


// Destructor
LevelSource::~LevelSource()
{
   // Do nothing
}


S32 LevelSource::getLevelCount() const
{
   return mLevelInfos.size();
}


LevelInfo LevelSource::getLevelInfo(S32 index)
{
   return mLevelInfos[index];
}


// Parse through the chunk of data passed in and find parameters to populate levelInfo with
// This is only used on the server to provide quick level information without having to load the level
// (like with playlists or menus)
// Warning: Mungs chunk!
void LevelSource::getLevelInfoFromCodeChunk(char *chunk, S32 size, LevelInfo &levelInfo)
{
   S32 cur = 0;
   S32 startingCur = 0;

   bool foundGameType   = false;
   bool foundLevelName  = false;
   bool foundMinPlayers = false;
   bool foundMaxPlayers = false;

   while(cur < size && !(foundGameType && foundLevelName && foundMinPlayers && foundMaxPlayers))
   {
      if(chunk[cur] < 32)
      {
         if(cur - startingCur > 5)
         {
            char c = chunk[cur];
            chunk[cur] = 0;
            Vector<string> list = parseString(&chunk[startingCur]);
            chunk[cur] = c;

            if(list.size() >= 1 && list[0].find("GameType") != string::npos)
            {
               // validateGameType() will return a valid GameType string -- either what's passed in, or the default if something bogus was specified
               TNL::Object *theObject = TNL::Object::create(GameType::validateGameType(list[0].c_str()));

               GameType *gt = dynamic_cast<GameType *>(theObject); 
               if(gt)
               {
                  levelInfo.mLevelType = gt->getGameTypeId();
                  foundGameType = true;
               }

               delete theObject;
            }
            else if(list.size() >= 2 && list[0] == "LevelName")
            {
               string levelName = list[1];

               // Append additional words to levelName
               for(S32 i = 2; i < list.size(); i++)   
                  levelName += " " + list[i];

               levelInfo.mLevelName = levelName;

               foundLevelName = true;
            }
            else if(list.size() >= 2 && list[0] == "MinPlayers")
            {
               levelInfo.minRecPlayers = atoi(list[1].c_str());
               foundMinPlayers = true;
            }
            else if(list.size() >= 2 && list[0] == "MaxPlayers")
            {
               levelInfo.maxRecPlayers = atoi(list[1].c_str());
               foundMaxPlayers = true;
            }
         }
         startingCur = cur + 1;
      }
      cur++;
   }

   levelInfo.ensureLevelInfoHasValidName();
}


// User has uploaded a file and wants to add it to the current playlist
pair<S32, bool> LevelSource::addLevel(LevelInfo levelInfo)
{
   // Check if we already have this one -- matches by filename and folder
   for(S32 i = 0; i < mLevelInfos.size(); i++)
   {
      if(mLevelInfos[i].filename == levelInfo.filename && mLevelInfos[i].folder == levelInfo.folder)
         return pair<S32, bool>(i, false);
   }

   // We don't have it... so add it!
   mLevelInfos.push_back(levelInfo);

   return pair<S32, bool>(getLevelCount() - 1, true);
}


string LevelSource::getLevelName(S32 index)
{
   return mLevelInfos[index].mLevelName.getString(); 
}


string LevelSource::getLevelFileName(S32 index)
{   
   if(index < 0 || index >= mLevelInfos.size())
      return "";
   else

   return mLevelInfos[index].filename;
}


GameTypeId LevelSource::getLevelType(S32 index)
{
   return mLevelInfos[index].mLevelType;
}


void LevelSource::remove(S32 index)
{
   mLevelInfos.erase(index);
}


extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);     // Sort alphanumerically

Vector<string> LevelSource::findAllLevelFilesInFolder(const string &levelDir)
{
   Vector<string> levelList;

   // Build our level list by looking at the filesystem 
   const string extList[] = {"level"};

   if(!getFilesFromFolder(levelDir, levelList, extList, ARRAYSIZE(extList)))    // Returns true if error 
   {
      logprintf(LogConsumer::LogError, "Could not read any levels from the levels folder \"%s\".", levelDir.c_str());
      return levelList;   
   }

   levelList.sort(alphaSort);   // Just to be sure...
   return levelList;
}


bool LevelSource::populateLevelInfoFromSource(const string &fullFilename, S32 index)
{
   return populateLevelInfoFromSource(fullFilename, mLevelInfos[index]);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor -- pass in a list fo level names and a folder; create LevelInfos for each
FolderLevelSource::FolderLevelSource(const Vector<string> &levelList, const string &folder)
{
   for(S32 i = 0; i < levelList.size(); i++)
      mLevelInfos.push_back(LevelInfo(levelList[i], folder));
}


// Destructor
FolderLevelSource::~FolderLevelSource()
{
   // Do noting
}


// Populate all our levelInfos from disk; return true if we managed to load any, false otherwise
bool FolderLevelSource::loadLevels(FolderManager *folderManager)
{
   bool anyLoaded = false;

   for(S32 i = 0; i < mLevelInfos.size(); i++)
   {
      string filename = folderManager->findLevelFile(mLevelInfos[i].folder, mLevelInfos[i].filename);

      if(Parent::populateLevelInfoFromSource(filename, i))
         anyLoaded = true;
      else
      {
         mLevelInfos.erase(i);
         i--;
      }
   }

   return anyLoaded;
}


// Load specified level, put results in gameObjectDatabase.  Return md5 hash of level
string FolderLevelSource::loadLevel(S32 index, Game *game, GridDatabase *gameObjectDatabase)
{
   TNLAssert(index >= 0 && index < mLevelInfos.size(), "Index out of bounds!");

   LevelInfo *levelInfo = &mLevelInfos[index];

   string filename = FolderManager::findLevelFile(levelInfo->folder, levelInfo->filename);

   if(filename == "")
   {
      logprintf("Unable to find level file \"%s\".  Skipping...", levelInfo->filename.c_str());
      return "";
   }

   if(game->loadLevelFromFile(filename, gameObjectDatabase))
      return Game::md5.getHashFromFile(filename);    // TODO: Combine this with the reading of the file we're doing anyway in initLevelFromFile()
   else
   {
      logprintf("Unable to process level file \"%s\".  Skipping...", levelInfo->filename.c_str());
      return "";
   }
}


// Returns a textual level descriptor good for logging and error messages and such
string FolderLevelSource::getLevelFileDescriptor(S32 index) const
{
   return "levelfile \"" + mLevelInfos[index].filename + "\"";
}


// Populates levelInfo with data from fullFilename -- returns true if successful, false otherwise
// Reads 4kb of file and uses what it finds there to populate the levelInfo
bool FolderLevelSource::populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo)
{
   FILE *f = fopen(fullFilename.c_str(), "rb");

   if(f)
   {
      char data[1024 * 4];  // 4 kb should be enough to fit all parameters at the beginning of level; we don't need to read everything
      S32 size = (S32)fread(data, 1, sizeof(data), f);
      fclose(f);

      getLevelInfoFromCodeChunk(data, size, levelInfo);     // Fills levelInfo with data from file

      levelInfo.ensureLevelInfoHasValidName();

      return true;
   }
   else
   {
      logprintf(LogConsumer::LogWarning, "Could not load level %s [%s].  Skipping...", 
                                          levelInfo.filename.c_str(), fullFilename.c_str());
      return false;
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
StringLevelSource::StringLevelSource(const string &levelCode)
{
   mLevelCode = levelCode;

   LevelInfo levelInfo;
   mLevelInfos.push_back(levelInfo);
}


// Destructor
StringLevelSource::~StringLevelSource()
{
   // Do nothing
}


bool StringLevelSource::populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo)
{
   char chunk[1024 * 4];

   strncpy(chunk, mLevelCode.c_str(), sizeof(chunk));
   getLevelInfoFromCodeChunk(chunk, strlen(chunk), levelInfo);

   return true;
}


}