//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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
   bool foundScriptFileName = false;

   while(cur < size && !(foundGameType && foundLevelName && foundMinPlayers && foundMaxPlayers && foundScriptFileName))
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
            else if(list.size() >= 2 && list[0] == "Script")
            {
               levelInfo.mScriptFileName = list[1];
               foundScriptFileName = true;
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


// Should be overridden in each subclass of LevelSource
bool LevelSource::loadLevels(FolderManager *folderManager)
{
	return true;
}


////////////////////////////////////////
////////////////////////////////////////


MultiLevelSource::MultiLevelSource()
{

}


MultiLevelSource::~MultiLevelSource()
{

}

// Populate all our levelInfos from disk; return true if we managed to load any, false otherwise
bool MultiLevelSource::loadLevels(FolderManager *folderManager)
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
string MultiLevelSource::loadLevel(S32 index, Game *game, GridDatabase *gameObjectDatabase)
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
string MultiLevelSource::getLevelFileDescriptor(S32 index) const
{
   return "levelfile \"" + mLevelInfos[index].filename + "\"";
}


// Populates levelInfo with data from fullFilename -- returns true if successful, false otherwise
// Reads 4kb of file and uses what it finds there to populate the levelInfo
bool MultiLevelSource::populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo)
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
      logprintf(LogConsumer::LogWarning, "Could not load level %s [%s]... Skipping...",
                                          levelInfo.filename.c_str(), fullFilename.c_str());
      return false;
   }
}


bool MultiLevelSource::isEmptyLevelDirOk() const
{
	return false;
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor -- pass in a list of level names and a folder; create LevelInfos for each
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


////////////////////////////////////////
////////////////////////////////////////

// Constructor -- pass in a list of level names and a file; create LevelInfos for each
FileListLevelSource::FileListLevelSource(const Vector<string> &levelList, const string &playlist)
{
	for(S32 i = 0; i < levelList.size(); i++)
      mLevelInfos.push_back(LevelInfo(levelList[i], playlist));
}


// Destructor
FileListLevelSource::~FileListLevelSource()
{
   // Do nothing
}


// Load specified level, put results in gameObjectDatabase.  Return md5 hash of level.
string FileListLevelSource::loadLevel(S32 index, Game *game, GridDatabase *gameObjectDatabase)
{
   TNLAssert(index >= 0 && index < mLevelInfos.size(), "Index out of bounds!");

   LevelInfo *levelInfo = &mLevelInfos[index];

	string filename = FolderManager::findLevelFile(GameSettings::getFolderManager()->levelDir, levelInfo->filename);

	if(filename == "")
	{
		logprintf("Unable to find level file \"%s\".  Skipping...", levelInfo->filename.c_str());
		return "";
	}

	if(game->loadLevelFromFile(filename, gameObjectDatabase))
	   return Game::md5.getHashFromFile(filename);   
	else
	{
		logprintf("Unable to process level file \"%s\".  Skipping...", levelInfo->filename.c_str());
		return "";
	}
}


// Static method
Vector<string> FileListLevelSource::getFilePlaylist(const string &fileName, Game *game)
{
   Vector<string> levels;
   Vector<string> lines = parseString(readFile(fileName));

   const string levelDir = GameSettings::getFolderManager()->levelDir;

   for(S32 i = 0; i < lines.size(); i++)
	{
      string line = trim(chopComment(lines[i]));
      if(line == "")    // Probably a comment or blank line
         continue;

      string filename = FolderManager::findLevelFile(levelDir, line);

      if(filename == "")
      {
         logprintf("Unable to find level file \"%s\".  Skipping...", filename.c_str());
         continue;
      }

   	levels.push_back(filename);
	}

	return levels;
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


string StringLevelSource::loadLevel(S32 index, Game *game, GridDatabase *gameObjectDatabase)
{
   game->loadLevelFromString(mLevelCode, gameObjectDatabase, "");
   return Game::md5.getHashFromString(mLevelCode); 
}


// Returns a textual level descriptor good for logging and error messages and such
string StringLevelSource::getLevelFileDescriptor(S32 index) const
{
   return "string input (" + itos((U32) mLevelCode.length()) + " chars)";
}


bool StringLevelSource::isEmptyLevelDirOk() const
{
   return true;      // No folder needed -- level was passed into constructor!
}


}
