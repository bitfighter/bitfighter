//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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
   string mScriptFileName;
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
struct FolderManager;

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
   string          getLevelName(S32 index);
   virtual string  getLevelFileName(S32 index);
   GameTypeId      getLevelType(S32 index);

   virtual bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo) = 0;
   virtual string loadLevel(S32 index, Game *game, GridDatabase *gameObjDatabase) = 0;
   virtual bool loadLevels(FolderManager *folderManager);
   virtual string getLevelFileDescriptor(S32 index) const = 0;
   virtual bool isEmptyLevelDirOk() const = 0;

   bool populateLevelInfoFromSource(const string &sourceName, S32 levelInfoIndex);

   static Vector<string> findAllLevelFilesInFolder(const string &levelDir);
   static void getLevelInfoFromCodeChunk(char *chunk, S32 size, LevelInfo &levelInfo);     // Populates levelInfo
};


////////////////////////////////////////
////////////////////////////////////////


class MultiLevelSource : public LevelSource
{
   typedef LevelSource Parent;

public:
   MultiLevelSource();  				// Constructor
   virtual ~MultiLevelSource();     // Destructor

   bool loadLevels(FolderManager *folderManager);
   string loadLevel(S32 index, Game *game, GridDatabase *gameObjDatabase);
   string getLevelFileDescriptor(S32 index) const;
   bool isEmptyLevelDirOk() const;

   bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo);
};


////////////////////////////////////////
////////////////////////////////////////


class FolderLevelSource : public MultiLevelSource
{
   typedef MultiLevelSource Parent;

public:
   FolderLevelSource(const Vector<string> &levelList, const string &folder);  // Constructor
   virtual ~FolderLevelSource();                                              // Destructor
};


////////////////////////////////////////
////////////////////////////////////////


// This LevelSource loads levels according to instructions in a text file
class FileListLevelSource : public MultiLevelSource
{
   typedef MultiLevelSource Parent;

private:
   string playlistFile;

public:
   FileListLevelSource(const Vector<string> &levelList, const string &playlistFile);     // Constructor
   virtual ~FileListLevelSource();                                                                                                                // Destructor

   string loadLevel(S32 index, Game *game, GridDatabase *gameObjDatabase);

   static Vector<string> findAllFilesInPlaylist(const string &fileName, const string &levelDir);
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

   string loadLevel(S32 index, Game *game, GridDatabase *gameObjDatabase);
   string getLevelFileDescriptor(S32 index) const;
   bool isEmptyLevelDirOk() const;

   bool populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo);
};


////////////////////////////////////////
////////////////////////////////////////

typedef boost::shared_ptr<LevelSource> LevelSourcePtr;


}

#endif
