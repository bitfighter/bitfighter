//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelDatabaseDownloadThread.h"
#include "HttpsRequest.h"
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

   string safeLevelId = extractFilename(mLevelId);
   if(safeLevelId.empty() || safeLevelId != mLevelId || !safeFilename(safeLevelId.c_str()) || safeLevelId.find("..") != string::npos)
   {
      mGame->displayErrorMessage("!!! Invalid level ID requested.");
      errorNumber = 100;
      return;
   }

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

   dSprintf(url, UrlLength, (HttpsRequest::LevelDatabaseBaseUrl + LevelRequest).c_str(), mLevelId.c_str());
   HttpsRequest req(url);

   if(!req.send())
   {
      dSprintf(url, UrlLength, "!!! Error connecting to server");
      errorNumber = 1;
      return;
   }

   if(req.getResponseCode() != HttpsRequest::OK)
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

   dSprintf(url, UrlLength, (HttpsRequest::LevelDatabaseBaseUrl + LevelgenRequest).c_str(), mLevelId.c_str());
   req = HttpsRequest(url);
   if(!req.send())
   {
      dSprintf(url, UrlLength, "!!! Error connecting to server", levelFileName.c_str());
      errorNumber = 2;
      return;
   }

   if(req.getResponseCode() != HttpsRequest::OK)
   {
      dSprintf(url, UrlLength, "!!! Server returned an error: %d", req.getResponseCode());
      errorNumber = 2;
      return;
   }

   string levelgenCode = req.getResponseBody();

   // no data is sent if the level has no levelgen
   if(levelgenCode.length() > 3 && levelgenCode.substr(0, 3) == "-- ")
   {
      // the leveldb prepends a lua comment with the target filename, and here we parse it
      size_t breakIndex = levelgenCode.find_first_of("\r\n");
      if(breakIndex != string::npos && breakIndex > 3)
      {
         string rawFileName = levelgenCode.substr(3, breakIndex - 3);
         string parsedLevelgenFileName = extractFilename(rawFileName);
         bool hasValidExt = (parsedLevelgenFileName.size() >= 9 && parsedLevelgenFileName.rfind(".levelgen") == parsedLevelgenFileName.size() - 9) ||
                            (parsedLevelgenFileName.size() >= 4 && parsedLevelgenFileName.rfind(".lua") == parsedLevelgenFileName.size() - 4);
         if(!parsedLevelgenFileName.empty() && parsedLevelgenFileName == rawFileName &&
            safeFilename(parsedLevelgenFileName.c_str()) &&
            hasValidExt &&
            parsedLevelgenFileName.find('/') == string::npos &&
            parsedLevelgenFileName.find('\\') == string::npos &&
            parsedLevelgenFileName.find("..") == string::npos)
         {
            size_t contentStart = levelgenCode.find_first_not_of("\r\n", breakIndex);
            if(contentStart != string::npos)
               levelgenCode = levelgenCode.substr(contentStart);
            else
               levelgenCode = "";

            filePath = strictjoindir(levelDir, parsedLevelgenFileName);
            if(writeFile(filePath, levelgenCode))
            {
               levelGenFileName = parsedLevelgenFileName;
            }
            else
            {
               dSprintf(url, UrlLength, "!!! Error writing levelgen file");
               errorNumber = 2;
               return;
            }
         }
         else
         {
            dSprintf(url, UrlLength, "!!! Invalid levelgen filename in response");
            errorNumber = 2;
            return;
         }
      }
      else
      {
         dSprintf(url, UrlLength, "!!! Malformed levelgen response");
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

