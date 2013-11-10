//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelDatabaseDownloadThread.h"
#include "HttpRequest.h"
#include "ClientGame.h"
#include "ServerGame.h"
#include "LevelSource.h"

#include "stringUtils.h"

#include "tnlThread.h"

#include <stdio.h>
#include <iostream>

namespace Zap
{

string LevelDatabaseDownloadThread::LevelRequest = "/levels/raw/%s";
string LevelDatabaseDownloadThread::LevelgenRequest = "/levels/raw/%s/levelgen";

// Constructor
LevelDatabaseDownloadThread::LevelDatabaseDownloadThread(string levelId, ClientGame *game)
   : mLevelId(levelId), 
     mGame(game)
{
   // Do nothing
}


// Destructor
LevelDatabaseDownloadThread::~LevelDatabaseDownloadThread()    
{
   // Do nothing
}


U32 LevelDatabaseDownloadThread::run()
{
   char url[UrlLength];

   string levelFileName = "db_" + mLevelId + ".level";

   FolderManager *fm = mGame->getSettings()->getFolderManager();
   string filePath = joindir(fm->levelDir, levelFileName);

   if(fileExists(filePath))
   {
      // Check if file is on our delete list... if so, we can clobber it.  But we also need to remove it from the skiplist.
      if(mGame->getSettings()->isLevelOnSkipList(levelFileName))
      {
         mGame->getSettings()->removeLevelFromSkipList(levelFileName);
      }
      else     // File exists and is not on the skip list... show an error message
      {
         mGame->displayErrorMessage("!!! Already have a file called %s on the server.  Download aborted.", filePath.c_str());
         delete this;
         return 0;
      }
   }


   mGame->displaySuccessMessage("Downloading %s", mLevelId.c_str());
   dSprintf(url, UrlLength, (HttpRequest::LevelDatabaseBaseUrl + LevelRequest).c_str(), mLevelId.c_str());
   HttpRequest req(url);
   
   if(!req.send())
   {
      mGame->displayErrorMessage("!!! Error connecting to server");
      delete this;
      return 0;
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      mGame->displayErrorMessage("!!! Server returned an error: %d", req.getResponseCode());
      delete this;
      return 0;
   }

   string levelCode = req.getResponseBody();

   if(writeFile(filePath, levelCode))
   {
      mGame->displaySuccessMessage("Saved to %s", levelFileName.c_str());
      if(gServerGame)
      {
    	  LevelInfo levelInfo;
    	  levelInfo.filename = levelFileName;
        levelInfo.folder = fm->levelDir;

    	  if(gServerGame->populateLevelInfoFromSource(filePath, levelInfo))
        {
    	     gServerGame->addLevel(levelInfo);
           gServerGame->sendLevelListToLevelChangers(string("Level ") + levelInfo.mLevelName.getString() + " added to server");
        }
      }
   }
   else  // File writing went bad
   {
      mGame->displayErrorMessage("!!! Could not write to %s", levelFileName.c_str());
      delete this;
      return 0;
   }

   dSprintf(url, UrlLength, (HttpRequest::LevelDatabaseBaseUrl + LevelgenRequest).c_str(), mLevelId.c_str());
   req = HttpRequest(url);
   if(!req.send())
   {
      mGame->displayErrorMessage("!!! Error connecting to server");
      delete this;
      return 0;
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      mGame->displayErrorMessage("!!! Server returned an error: %d", req.getResponseCode());
      delete this;
      return 0;
   }

   string levelgenCode = req.getResponseBody();

   // no data is sent if the level has no levelgen
   if(levelgenCode.length() > 0)
   {
      // the leveldb prepends a lua comment with the target filename, and here we parse it
      int startIndex = 3; // the length of "-- "
      int breakIndex = levelgenCode.find_first_of("\r\n");
      string levelgenFileName = levelgenCode.substr(startIndex, breakIndex - startIndex);
      // trim the filename line before writing
      levelgenCode = levelgenCode.substr(breakIndex + 2, levelgenCode.length());

      FolderManager *fm = mGame->getSettings()->getFolderManager();
      filePath = joindir(fm->levelDir, levelgenFileName);
      if(writeFile(filePath, levelgenCode))
      {
         mGame->displaySuccessMessage("Saved to %s", levelgenFileName.c_str());
      }
      else
      {
         mGame->displayErrorMessage("!!! Could not write to %s", levelgenFileName.c_str());
      }
   }
   delete this;
   return 0;
}


}

