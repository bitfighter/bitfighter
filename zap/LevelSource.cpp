//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelSource.h"

#include "config.h"           // For FolderManager
#include "gameType.h"
#include "GameSettings.h"
#include "Level.h"
#include "LevelInfoDatabaseMapping.h"
#include "stringUtils.h"

#include "tnlAssert.h"
#include "Md5Utils.h"

#include <fstream>
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
   this->folder = folder;
}


// Constructor
LevelInfo::LevelInfo(const string &levelName, GameTypeId levelType, S32 minPlayers, S32 maxPlayers, const string &script)
{
   initialize();

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
   mHosterLevelIndex = -1;
   mSqliteLevelId = -1;
}


static string STE2String(StringTableEntry ste)
{
   return ste.getString();
}


const string LevelInfo::LEVEL_INFO_DATABASE_NAME = "levelinfo.db";


static const S32 MAX_INDEX = 0
#  define LEVEL_INFO_ITEM(index, b, c, d, e, f)  * 0 + index
   LEVEL_INFO_DATABASE_MAPPING_TABLE
#  undef LEVEL_INFO_ITEM
   + 1;     // + 1 for level_info_id

   // static const S32 MAX_INDEX = 0 * 0 + 1 * 0 + 2 * 0 + 3... + 1


string LevelInfo::getSelectSql(const string &hash)
{
   return string("") + "SELECT "
#  define LEVEL_INFO_ITEM(a, col, c, d, e, f)  col + ","
      LEVEL_INFO_DATABASE_MAPPING_TABLE
#  undef LEVEL_INFO_ITEM
      "level_info_id FROM " + Sqlite::LEVEL_INFO_TABLE_NAME + " WHERE hash = '" + hash + "';";

   // Generates something that goes a little like this:
   // SELECT hash, level_name, game_type, min_players, max_players, script_file, 1
   // FROM levelinfo
   // WHERE hash = '8f0e423dd3cc5f1ef2d48a4781fb98bf';
}


void LevelInfo::populateFromDatabaseResults(char **results)
{
   // Note that the first line of results is the column headers, so we need to apply offset of MAX_INDEX
#  define LEVEL_INFO_ITEM(index, b, c, param, transformation, f)  param = transformation(results[index + MAX_INDEX + 1]);
   LEVEL_INFO_DATABASE_MAPPING_TABLE
#  undef LEVEL_INFO_ITEM

   // MAX_INDEX is 5; there are 6 items (including mSqliteLevelId), hence the + 1.  Also see comment above about MAX_INDEX.
   // Generates something that goes a bit like this:
   // mLevelHash      = (results[0 + MAX_INDEX + 1]);
   // mLevelName      = STE2String(results[1 + MAX_INDEX + 1]);
   // mLevelType      = GameType::getGameTypeIdFromName(results[2 + MAX_INDEX + 1]);
   // minRecPlayers   = atoi(results[3 + MAX_INDEX + 1]);
   // maxRecPlayers   = atoi(results[4 + MAX_INDEX + 1]);
   // mScriptFileName = (results[5 + MAX_INDEX + 1]); 

   mSqliteLevelId = atoi(results[MAX_INDEX + MAX_INDEX + 1]);
}


string LevelInfo::toSql() const
{
   ostringstream str;
   str << "INSERT INTO " << Sqlite::LEVEL_INFO_TABLE_NAME << "(" <<
#  define LEVEL_INFO_ITEM(a, dbField, c, d, e, f)  dbField << "," <<
      LEVEL_INFO_DATABASE_MAPPING_TABLE
#  undef LEVEL_INFO_ITEM
      "last_seen) VALUES (" <<
#  define LEVEL_INFO_ITEM(a, b, c, field, e, transform) " '" << transform(field) << "'," <<
      LEVEL_INFO_DATABASE_MAPPING_TABLE
#  undef LEVEL_INFO_ITEM
       "datetime('now'));";

   // Produces this:
   // INSERT INTO levels(hash,level_name,game_type,min_players,max_players,script_file,last_seen) 
   //          VALUES ('8f0e423dd3cc5f1ef2d48a4781fb98bf','Level Name','NexusGameType','1','5','killemall',datetime('now'));

   return str.str();
}

void LevelInfo::writeToStream(ostream &stream, const string &hash) const
{
   stream << hash << ",\"" << mLevelName.getString() << "\"," << GameType::getGameTypeName(mLevelType) << ","
      << minRecPlayers << "," << maxRecPlayers << "," << mScriptFileName << '\n';
}


const char *LevelInfo::getLevelTypeName() const
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

// Statics
const string LevelSource::TestFileName = "editor.tmp";


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
   str.erase(remove(str.begin(), str.end(), '"'), str.end());
}


// Static method
bool LevelSource::getLevelInfoFromDatabase(const string &hash, LevelInfo &levelInfo)
{
   return false;
}


// Parse through the chunk of data passed in and find parameters to populate levelInfo with
// This is only used on the server to provide quick level information without having to load the level
// (like with playlists or menus).  Static method.
void LevelSource::getLevelInfoFromCodeChunk(const string &code, const string &hash, LevelInfo &levelInfo)
{
   istringstream stream(code);
   getLevelInfoFromStream(stream, hash, levelInfo);
}


void LevelSource::getLevelInfoFromStream(istream &stream, const string &hash, LevelInfo &levelInfo)
{
   levelInfo.mLevelHash = hash;

   DbWriter::DbQuery query(LevelInfo::LEVEL_INFO_DATABASE_NAME.c_str());

   if(populateFromDatabase(query, levelInfo, hash))
      return;

   string line;

   // Read until all these are true
   bool foundGameType   = false, foundLevelName  = false, foundMinPlayers = false,
        foundMaxPlayers = false, foundScriptName = false;

   static const S32 gameTypeLen      = strlen("GameType");
   static const S32 levelNameLen     = strlen("LevelName");
   static const S32 minMaxPlayersLen = strlen("MinPlayers");
   static const S32 scriptLen        = strlen("Script");

   U32 pos;

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

            // ValidateGameType is guaranteed to return a valid GameType name.  Or your money back!!!
            const string validatedName = GameType::validateGameType(gameTypeName);

            GameTypeId gameTypeId = GameType::getGameTypeIdFromName(validatedName);
            levelInfo.mLevelType = gameTypeId;

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
            if(pos != string::npos)
            {
               string levelName = line.substr(pos);
               stripQuotes(levelName);
               levelInfo.mLevelName = trim(levelName);
            }

            foundLevelName = true;
            continue;
         }
      }

      // Check for MinPlayers
      if(!foundMinPlayers)
      {
         if(line.substr(0, minMaxPlayersLen) == "MinPlayers")
         {
            pos = line.find_first_not_of(" ", minMaxPlayersLen + 1);
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
            pos = line.find_first_not_of(" ", minMaxPlayersLen + 1);
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

   levelInfo.ensureLevelInfoHasValidName();
   
   U64 id = query.runInsertQuery(levelInfo.toSql());     // Returns U64_MAX if there was an error, writes to log
   levelInfo.setSqliteLevelId(id);
}

   
void LevelInfo::setSqliteLevelId(U64 levelId)
{
   mSqliteLevelId = levelId;
}


U64 LevelInfo::getSqliteLevelId() const
{
   return mSqliteLevelId;
}


bool LevelInfo::isValidSqliteLevelId(U64 id)
{
   return id != U64_MAX;
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


void LevelSource::addNewLevel(const LevelInfo &levelInfo)
{
   mLevelInfos.push_back(levelInfo);
}


string LevelSource::getLevelName(S32 index)
{
   if(index < 0 || index >= mLevelInfos.size())
      return "";
   else
      return mLevelInfos[index].mLevelName.getString();
}


string LevelSource::getLevelFileName(S32 index)
{
   if(index < 0 || index >= mLevelInfos.size())
      return "";
   else
      return mLevelInfos[index].filename;
}


void LevelSource::setLevelFileName(S32 index, const string &filename)
{
   mLevelInfos[index].filename = filename;
}


GameTypeId LevelSource::getLevelType(S32 index)
{
   return mLevelInfos[index].mLevelType;
}


void LevelSource::remove(S32 index)
{
   mLevelInfos.erase(index);
}


// static method
Vector<string> LevelSource::findAllLevelFilesInFolder(const string &levelDir)
{
   Vector<string> levelList;

   // Build our level list by looking at the filesystem 
   const string extList[] = {"level"};

   if(!getFilesFromFolder(levelDir, levelList, FILENAME_ONLY, extList, ARRAYSIZE(extList)))    // Returns true if error 
   {
      logprintf(LogConsumer::LogError, "Could not read any levels from the levels folder \"%s\".", levelDir.c_str());
      return levelList;
   }

   levelList.sort(alphaSort);   // Just to be sure...
   return levelList;
}


bool LevelSource::populateLevelInfoFromSourceByIndex(S32 index)
{
   // If findLevelFile fails, it will return "", which populateLevelInfoFromSource will handle properly
   string filename = FolderManager::findLevelFile(mLevelInfos[index].folder, mLevelInfos[index].filename);
   return populateLevelInfoFromSource(filename, mLevelInfos[index]);
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
      if(Parent::populateLevelInfoFromSourceByIndex(i))
         anyLoaded = true;
      else
      {
         mLevelInfos.erase(i);
         i--;
      }
   }

   return anyLoaded;
}


// Load specified level, put results in gameObjectDatabase.
Level *MultiLevelSource::getLevel(S32 index) const
{
   TNLAssert(index >= 0 && index < mLevelInfos.size(), "Index out of bounds!");

   const LevelInfo &levelInfo = mLevelInfos[index];

   string filename = FolderManager::findLevelFile(levelInfo.folder, levelInfo.filename);

   if(filename == "")
   {
      logprintf("Unable to find level file \"%s\".  Skipping...", levelInfo.filename.c_str());
      return NULL;
   }

   Level *level = new Level();      // Deleted by Game

   if(!level->loadLevelFromFile(filename, levelInfo.getSqliteLevelId()))
   {
      logprintf("Unable to process level file \"%s\".  Skipping...", levelInfo.filename.c_str());
      delete level;
      return NULL;
   }

   return level;
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
   // Check if we got a dud... (FolderManager::findLevelFile() will, for example, return "" if it fails)
   if(fullFilename.empty())
      return false;

   //FILE *f = fopen(fullFilename.c_str(), "rb");
   //if(!f)
   //{
   //   logprintf(LogConsumer::LogWarning, "Could not read level file %s [%s]... Skipping...",
   //      levelInfo.filename.c_str(), fullFilename.c_str());
   //   return false;
   //}

   ifstream fileStream(fullFilename.c_str(), ios_base::in | ios_base::binary);
   if(fileStream.fail())
   {
      logprintf(LogConsumer::LogWarning, "Could not read level file %s [%s]... Skipping...",
         levelInfo.filename.c_str(), fullFilename.c_str());
      return false;
   }


   // If not, load the level
   //Level level;
   //level.loadLevelFromStream(fileStream, fullFilename, hash);



   // some ideas for getting the area of a level:
   //   if(loadLevel())
   //      {
   //         loaded = true;
   //         logprintf(LogConsumer::ServerFilter, "Done. [%s]", getTimeStamp().c_str());
   //      }
   //      else
   //      {
   //         logprintf(LogConsumer::ServerFilter, "FAILED!");
   //
   //         if(mLevelSource->getLevelCount() > 1)
   //            removeLevel(mCurrentLevelIndex);
   //         else
   //         {
   //            // No more working levels to load...  quit?
   //            logprintf(LogConsumer::LogError, "All the levels I was asked to load are corrupt.  Exiting!");
   //
   //            mShutdownTimer.reset(1); 
   //            mShuttingDown = true;
   //            mShutdownReason = "All the levels I was asked to load are corrupt or missing; "
   //                              "Sorry dude -- hosting mode shutting down.";
   //
   //            // To avoid crashing...
   //            if(!getGameType())
   //            {
   //               GameType *gameType = new GameType();
   //               gameType->addToGame(this, getLevel());
   //            }
   //            mLevel->makeSureTeamCountIsNotZero();
   //
   //            return;
   //         }
   //      }
   //   }
   //
   //   computeWorldObjectExtents();                       // Compute world Extents nice and early
   //
   //   if(!mGameRecorderServer && !mShuttingDown && getSettings()->getSetting<YesNo>(IniKey::GameRecording))
   //      mGameRecorderServer = new GameRecorderServer(this);
   //
   //
   //   ////// This block could easily be moved off somewhere else   
   //   fillVector.clear();
   //   getLevel()->findObjects(TeleporterTypeNumber, fillVector);
   //
   //   Vector<pair<Point, const Vector<Point> *> > teleporterData(fillVector.size());
   //   pair<Point, const Vector<Point> *> teldat;
   //
   //   for(S32 i = 0; i < fillVector.size(); i++)
   //   {
   //      Teleporter *teleporter = static_cast<Teleporter *>(fillVector[i]);
   //
   //      teldat.first  = teleporter->getPos();
   //      teldat.second = teleporter->getDestList();
   //
   //      teleporterData.push_back(teldat);
   //   }
   //
   //   // Get our parameters together
   //   Vector<DatabaseObject *> barrierList;
   //   getLevel()->findObjects((TestFunc)isWallType, barrierList, *getWorldExtents());
   //
   //   Vector<DatabaseObject *> turretList;
   //   getLevel()->findObjects(TurretTypeNumber, turretList, *getWorldExtents());
   //
   //   Vector<DatabaseObject *> forceFieldProjectorList;
   //   getLevel()->findObjects(ForceFieldProjectorTypeNumber, forceFieldProjectorList, *getWorldExtents());
   //
   //   bool triangulate;
   //
   //   // Try and load Bot Zones for this level, set flag if failed
   //   // We need to run buildBotMeshZones in order to set mAllZones properly, which is why I (sort of) disabled the use of hand-built zones in level files
   //#ifdef ZAP_DEDICATED
   //   triangulate = false;
   //#else
   //   triangulate = !isDedicated();
   //#endif
   //
   //   BotNavMeshZone::calcLevelSize(getWorldExtents(), barrierList, teleporterData);
   //
   //   ////////////////////////////
   //
   //


   // Reset stream to beginning
   fileStream.clear();
   fileStream.seekg(0, ios::beg);

   getLevelInfoFromStream(fileStream, Md5::getHashFromStream(fileStream), levelInfo);     // Fills levelInfo with data from file

   return true;
}


bool LevelSource::populateFromDatabase(DbWriter::DbQuery &query, LevelInfo &levelInfo, const string &hash)
{
   string sql = LevelInfo::getSelectSql(hash);

   Vector<Vector<string> > values;

   char *err = 0;
   char **results;
   S32 rows, cols;

   sqlite3_get_table(query.mSqliteDb, sql.c_str(), &results, &rows, &cols, &err);

   // results[0]...results[cols] contain the col headers ==> http://www.sqlite.org/c3ref/free_table.html
   TNLAssert(rows == 0 || rows == 1, "Index violation!");

   sqlite3_free(err);


   if(rows == 1)
   {
      levelInfo.populateFromDatabaseResults(results);
      sqlite3_free_table(results);
      return true;
   }

   sqlite3_free_table(results);
   return false;
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

// Constructor -- pass in a list of level names and a folder; create LevelInfos for each
PlaylistLevelSource::PlaylistLevelSource(const Vector<string> &levelList, const string &folder, GameSettings *settings)
{
   mGameSettings = settings;

   for(S32 i = 0; i < levelList.size(); i++)
      mLevelInfos.push_back(LevelInfo(levelList[i], folder));
}


// Destructor
PlaylistLevelSource::~PlaylistLevelSource()
{
   // Do nothing
}


// Load specified level, put results in gameObjectDatabase
Level *PlaylistLevelSource::getLevel(S32 index) const
{
   TNLAssert(index >= 0 && index < mLevelInfos.size(), "Index out of bounds!");

   const LevelInfo *levelInfo = &mLevelInfos[index];

   FolderManager *folderManager = mGameSettings->getFolderManager();

   string filename = folderManager->findLevelFile(folderManager->getLevelDir(), levelInfo->filename);

   if(filename == "")
   {
      logprintf("Unable to find level file \"%s\".  Skipping...", levelInfo->filename.c_str());
      return NULL;
   }

   Level *level = new Level();      // Will be deleted by caller

   if(!level->loadLevelFromFile(filename, levelInfo->getSqliteLevelId()))
   {
      logprintf("Unable to process level file \"%s\".  Skipping...", levelInfo->filename.c_str());
      delete level;
      return NULL;
   }

   return level;
}


// Static method
Vector<string> PlaylistLevelSource::findAllFilesInPlaylist(const string &playlistFile, const string &levelDir)
{
   Vector<string> levels;

   ifstream file(playlistFile.c_str());

   if(!file.is_open())
      return levels;

   string filename;

   while(getline(file, filename))
   {
      filename = trim(chopComment(filename));

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

TestPlaylistLevelSource::TestPlaylistLevelSource(const Vector<string> &levelList, GameSettings *settings)
   : Parent(levelList, "DummyFolderForTesting", settings)
{
   // Do nothing
}


Level *TestPlaylistLevelSource::getLevel(S32 index) const
{
   TNLAssert(index >= 0 && index < mLevelInfos.size(), "Index out of bounds!");

   const LevelInfo *levelInfo = &mLevelInfos[index];

   Level *level = new Level();      // Will be deleted by caller

   level->loadLevelFromString("", levelInfo->getSqliteLevelId());

   return level;
}


bool TestPlaylistLevelSource::isEmptyLevelDirOk() const
{
   return true;      // No folder needed -- we're testing!
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor -- single level
StringLevelSource::StringLevelSource(const string &levelCode)
{
   mLevelCodes.push_back(levelCode);

   LevelInfo levelInfo;
   mLevelInfos.push_back(levelInfo);
}


// Constructor -- multiple levels (only used for testing, at the moment)
StringLevelSource::StringLevelSource(const Vector<string> &levelCodes)
{
   mLevelCodes = levelCodes;

   for(S32 i = 0; i < levelCodes.size(); i++)
   {
      LevelInfo levelInfo;
      mLevelInfos.push_back(levelInfo);
   }
}


// Destructor
StringLevelSource::~StringLevelSource()
{
   // Do nothing
}


bool StringLevelSource::populateLevelInfoFromSourceByIndex(S32 levelInfoIndex)
{
   string hash = Md5::getHashFromString(mLevelCodes[levelInfoIndex]);

   getLevelInfoFromCodeChunk(mLevelCodes[levelInfoIndex], hash, mLevelInfos[levelInfoIndex]);
   return true;
}


bool StringLevelSource::populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo)
{
   TNLAssert(false, "This is never called!");
   return true;
}


Level *StringLevelSource::getLevel(S32 index) const
{
   Level *level = new Level();

   level->loadLevelFromString(mLevelCodes[index], -1, "");
   return level;
}


// Returns a textual level descriptor good for logging and error messages and such
string StringLevelSource::getLevelFileDescriptor(S32 index) const
{
   return "string input (" + itos((U32)mLevelCodes[index].length()) + " chars)";
}


bool StringLevelSource::isEmptyLevelDirOk() const
{
   return true;      // No folder needed -- level was passed into constructor!
}


}
