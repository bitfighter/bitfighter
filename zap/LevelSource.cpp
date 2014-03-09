//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelSource.h"

#include "config.h"           // For FolderManager
#include "gameType.h"
#include "GameSettings.h"

#include "Md5Utils.h"
#include "stringUtils.h"

#include "tnlAssert.h"

#include <sstream>

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


// Constructor
LevelInfo::LevelInfo(const string &levelName, GameTypeId levelType, S32 minPlayers, S32 maxPlayers, const string &script)
{
   mLevelName = levelName;
   mLevelType = levelType;
   minRecPlayers = minPlayers;
   maxRecPlayers = maxPlayers;
   mScriptFileName = script;
}


// Destructor
LevelInfo::~LevelInfo()
{
   // Do nothing
}


void LevelInfo::initialize()
{
   mLevelType = BitmatchGame;
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


// Remove any "s in place
static void stripQuotes(string &str)      // not const; will be modified!
{
   str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
}


// Parse through the chunk of data passed in and find parameters to populate levelInfo with
// This is only used on the server to provide quick level information without having to load the level
// (like with playlists or menus)
void LevelSource::getLevelInfoFromCodeChunk(const string &code, LevelInfo &levelInfo)
{
   istringstream stream(code);
   string line;

   bool foundGameType   = false, foundLevelName  = false, foundMinPlayers = false, 
        foundMaxPlayers = false, foundScriptName = false;

   static const S32 gameTypeLen = strlen("GameType");
   static const S32 levelNameLen = strlen("LevelName");
   static const S32 minMaxPlayersLen = strlen("MinPlayers");
   static const S32 scriptLen = strlen("Script");

   std::size_t pos;

   // Iterate until we've either exhausted all the lines, or found everything we're looking for
   while(getline(stream, line) && (
         !foundGameType || !foundLevelName || !foundMinPlayers || !foundMaxPlayers || !foundScriptName))
   {
      // Check for GameType
      if(!foundGameType)
      {
         pos = line.find("GameType");
         if(pos != string::npos)
         {
            string gameTypeName = line.substr(0, pos + gameTypeLen); 
            TNL::Object *theObject = TNL::Object::create(GameType::validateGameType(gameTypeName.c_str()));

            GameType *gt = dynamic_cast<GameType *>(theObject); 
            if(gt)
               levelInfo.mLevelType = gt->getGameTypeId();

            delete theObject;
            foundGameType = true;
            continue;
         }
      }

      // Check for LevelName
      if(!foundLevelName)
      {
         if(line.substr(0, levelNameLen) == "LevelName")
         {
            pos = line.find_first_not_of(" ", levelNameLen + 1);
            string levelName = line.substr(pos);
            stripQuotes(levelName);
            levelInfo.mLevelName = trim(levelName);

            foundLevelName = true;
            continue;
         }
      }

      // Check for MinPlayers
      if(!foundMinPlayers)
      {
         if(line.substr(0, minMaxPlayersLen) == "MinPlayers")
         {
            pos = line.find_first_not_of(" ", levelNameLen + 1);
            if(pos != string::npos)
               levelInfo.minRecPlayers = atoi(line.substr(pos).c_str());

            foundMinPlayers = true;
            continue;
         }
      }

      // Check for MaxPlayers
      if(!foundMaxPlayers)
      {
         if(line.substr(0, minMaxPlayersLen) == "MaxPlayers")
         {
            pos = line.find_first_not_of(" ", levelNameLen + 1);
            if(pos != string::npos)
               levelInfo.maxRecPlayers = atoi(line.substr(pos).c_str());

            foundMaxPlayers = true;
            continue;
         }
      }

      // Check for Script
      if(!foundScriptName)
      {
         if(line.substr(0, scriptLen) == "Script")
         {
            pos = line.find_first_not_of(" ", scriptLen + 1);
            if(pos != string::npos)
            {
               string scriptName = line.substr(pos);
               stripQuotes(scriptName);
               levelInfo.mScriptFileName = scriptName;
            }
            foundScriptName = true;
            continue;
         }
      }
   }
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

// static method
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
   // Do nothing
}


MultiLevelSource::~MultiLevelSource()
{
   // Do nothing
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
      return Md5::getHashFromFile(filename);    // TODO: Combine this with the reading of the file we're doing anyway in initLevelFromFile()
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
      S32 t1, t2;
      // Method 1
      {
      S64 ts = Platform::getHighPrecisionTimerValue();
      string contents = readFile(fullFilename);
      getLevelInfoFromCodeChunk(contents, levelInfo);     // Fills levelInfo with data from file
      string hash = Md5::getHashFromString(contents); 
      S64 te = Platform::getHighPrecisionTimerValue();
      t1 = te - ts;
      }

      {
      // Method 2
      S64 ts = Platform::getHighPrecisionTimerValue();

      char data[1024 * 4];  // Should be enough to fit all parameters at the beginning of level; we don't need to read everything
		S32 size = (S32)fread(data, 1, sizeof(data), f);
	   fclose(f);

 	   getLevelInfoFromCodeChunk(string(data, size), levelInfo);     // Fills levelInfo with data from file

      // See if this slows things down... serves no other purpose at the moment
      // Tests suggest this takes between 0 and 1 ms
      string hash = Md5::getHashFromFile(fullFilename); 
      S64 te = Platform::getHighPrecisionTimerValue();
      t2 = (S32)(te - ts);
      }

      logprintf("Timings: %s %d / %d,    %f2.2 >>> ", fullFilename.c_str(), t1, t2, (F64)t1 / (F64)t2);

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
FileListLevelSource::FileListLevelSource(const Vector<string> &levelList, const string &folder)
{
	for(S32 i = 0; i < levelList.size(); i++)
      mLevelInfos.push_back(LevelInfo(levelList[i], folder));
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
	   return Md5::getHashFromFile(filename);   
	else
	{
		logprintf("Unable to process level file \"%s\".  Skipping...", levelInfo->filename.c_str());
		return "";
	}
}


// Static method
Vector<string> FileListLevelSource::findAllFilesInPlaylist(const string &fileName, const string &levelDir)
{
   Vector<string> levels;
   Vector<string> lines = parseString(readFile(fileName));

   for(S32 i = 0; i < lines.size(); i++)
	{
      string filename = trim(chopComment(lines[i]));
      if(filename == "")    // Probably a comment or blank line
         continue;

      string fullFileName = FolderManager::findLevelFile(levelDir, filename);

      if(fullFileName == "")
      {
         logprintf("Unable to find level file \"%s\".  Skipping...", filename.c_str());
         continue;
      }

   	levels.push_back(filename);      // We will append the folder name later
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
   getLevelInfoFromCodeChunk(mLevelCode, levelInfo);

   return true;
}


string StringLevelSource::loadLevel(S32 index, Game *game, GridDatabase *gameObjectDatabase)
{
   game->loadLevelFromString(mLevelCode, gameObjectDatabase, "");
   return Md5::getHashFromString(mLevelCode); 
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
