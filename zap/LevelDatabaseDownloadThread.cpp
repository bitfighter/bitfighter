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

#include "LevelDatabaseDownloadThread.h"
#include "ClientGame.h"
#include "HttpRequest.h"
#include "stringUtils.h"

#include "tnlThread.h"

#include <stdio.h>
#include <iostream>

namespace Zap
{

const char* LevelDatabaseDownloadThread::LevelRequest = "bitfighter.org/pleiades/levels/raw/%s";
const char* LevelDatabaseDownloadThread::LevelgenRequest = "bitfighter.org/pleiades/levels/raw/%s/levelgen";

LevelDatabaseDownloadThread::LevelDatabaseDownloadThread(string levelId, ClientGame *game)
   : mLevelId(levelId), mGame(game)
{
}

LevelDatabaseDownloadThread::~LevelDatabaseDownloadThread()
{
}

U32 LevelDatabaseDownloadThread::run()
{
   char url[UrlLength];
   mGame->displaySuccessMessage("Downloading %s", mLevelId.c_str());
   snprintf(url, UrlLength, LevelRequest, mLevelId.c_str());
   HttpRequest req(url);
   if(!req.send())
   {
      mGame->displayErrorMessage("Error connecting to server");
      delete this;
      return 0;
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      mGame->displayErrorMessage("Server returned an error: %d", req.getResponseCode());
      delete this;
      return 0;
   }

   string levelCode = req.getResponseBody();
   string levelFileName = "downloaded_" + mLevelId + ".level";

   FolderManager *fm = mGame->getSettings()->getFolderManager();
   string filePath = joindir(fm->levelDir, levelFileName);
   FILE *f = fopen(filePath.c_str(), "w");
   fprintf(f, "%s", levelCode.c_str());
   fclose(f);

   mGame->displaySuccessMessage("Saved to %s", levelFileName.c_str());

   snprintf(url, UrlLength, LevelgenRequest, mLevelId.c_str());
   req = HttpRequest(url);
   if(!req.send())
   {
      mGame->displayErrorMessage("Error connecting to server");
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      mGame->displayErrorMessage("Server returned an error: %d", req.getResponseCode());
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
      FILE *f = fopen(filePath.c_str(), "w");
      fprintf(f, "%s", levelgenCode.c_str());
      fclose(f);

      mGame->displaySuccessMessage("Saved to %s", levelgenFileName.c_str());
   }
   delete this;
   return 0;
}


}

