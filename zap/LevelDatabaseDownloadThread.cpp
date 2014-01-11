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
LevelDatabaseDownloadThread::LevelDatabaseDownloadThread(const string &levelId, ClientGame *game)
   : mLevelId(levelId), 
     mGame(game)
{
   errorNumber = 0;
   levelFileName = "db_" + mLevelId + ".level";

   FolderManager *fm = mGame->getSettings()->getFolderManager();
   levelDir = fm->levelDir;
   string filePath = joindir(levelDir, levelFileName);

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
         errorNumber = 100;
         return;
      }
   }

   mGame->displaySuccessMessage("Downloading %s", mLevelId.c_str());
}


// Destructor
LevelDatabaseDownloadThread::~LevelDatabaseDownloadThread()    
{
   // Do nothing
}


void LevelDatabaseDownloadThread::run()
{
   if(errorNumber == 100)
      return;

   dSprintf(url, UrlLength, (HttpRequest::LevelDatabaseBaseUrl + LevelRequest).c_str(), mLevelId.c_str());
   HttpRequest req(url);
   
   if(!req.send())
   {
      dSprintf(url, UrlLength, "!!! Error connecting to server");
      errorNumber = 1;
      return;
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      dSprintf(url, UrlLength, "!!! Server returned an error: %d", req.getResponseCode());
      errorNumber = 1;
      return;
   }

   string levelCode = req.getResponseBody();

   string filePath = joindir(levelDir, levelFileName);
   if(writeFile(filePath, levelCode))
   {
      // Success
   }
   else  // File writing went bad
   {
      dSprintf(url, UrlLength, "!!! Could not write to %s", levelFileName.c_str());
      errorNumber = 1;
      return;
   }

   dSprintf(url, UrlLength, (HttpRequest::LevelDatabaseBaseUrl + LevelgenRequest).c_str(), mLevelId.c_str());
   req = HttpRequest(url);
   if(!req.send())
   {
      dSprintf(url, UrlLength, "!!! Error connecting to server", levelFileName.c_str());
      errorNumber = 2;
      return;
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      dSprintf(url, UrlLength, "!!! Server returned an error: %d", req.getResponseCode());
      errorNumber = 2;
      return;
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

      filePath = joindir(levelDir, levelgenFileName);
      if(writeFile(filePath, levelgenCode))
      {
         // Success
      }
      else
      {
         dSprintf(url, UrlLength, "!!! Server returned an error: %d", req.getResponseCode());
         errorNumber = 2;
         return;
      }
   }
}
void LevelDatabaseDownloadThread::finish()
{
   if(errorNumber == 100)
      return;

   if(errorNumber == 2)
      mGame->displayErrorMessage("!!! Downloaded level without levelgen");

   if(errorNumber != 0)
      mGame->displayErrorMessage("%s", url);
   else
   {
      mGame->displaySuccessMessage("Saved to %s", levelFileName.c_str());
      if(levelGenFileName.length() != 0)
         mGame->displaySuccessMessage("Saved to %s", levelGenFileName.c_str());

      ServerGame *serverGame = mGame->getServerGame();

      if(serverGame)
      {
         LevelInfo levelInfo;
         levelInfo.filename = levelFileName;
         levelInfo.folder = levelDir;

         string filePath = joindir(levelDir, levelFileName);
         if(serverGame->populateLevelInfoFromSource(filePath, levelInfo))
         {
            serverGame->addLevel(levelInfo);
            serverGame->sendLevelListToLevelChangers(string("Level ") + levelInfo.mLevelName.getString() + " added to server");
         }
      }
   }

}

}

