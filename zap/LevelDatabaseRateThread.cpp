/*
 * LevelDatabaseRateThread.cpp
 *
 *  Created on: May 14, 2013
 *      Author: charon
 */

#include "LevelDatabaseRateThread.h"

#include "ClientGame.h"
#include "HttpRequest.h"

#include <sstream>

namespace Zap
{

const string LevelDatabaseRateThread::LevelDatabaseRateUrl = "bitfighter.org/pleiades/levels/rate/";

LevelDatabaseRateThread::LevelDatabaseRateThread(ClientGame* game, string rating)
   : mGame(game), mRating(rating)
{
}

LevelDatabaseRateThread::~LevelDatabaseRateThread()
{
}

U32 LevelDatabaseRateThread::run()
{
   if(!mGame->getLevelDatabaseId())
   {
      mGame->displayErrorMessage("Level ID not found -- redownload the level from the DB to enable rating");
      return 1;
   }

   mGame->displaySuccessMessage("Rating level...");
   stringstream id;
   id << mGame->getLevelDatabaseId();

   HttpRequest req = HttpRequest(LevelDatabaseRateUrl + id.str() + "/" + mRating);
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]",      mGame->getPlayerName());
   req.setData("data[User][user_password]", mGame->getPlayerPassword());

   if(!req.send())
   {
      mGame->displayErrorMessage("Error connecting to server");

      delete this;
      return 0;
   }

   S32 responseCode = req.getResponseCode();
   if(responseCode != HttpRequest::OK && responseCode != HttpRequest::Found)
   {
      stringstream message("Error rating level: ");
      message << responseCode;
      logprintf(req.getResponseBody().c_str());
      mGame->displayErrorMessage(message.str().c_str());
      delete this;
      return 0;
   }

   mGame->displaySuccessMessage("Rated successfully", true);

   delete this;
   return 0;
}

} /* namespace Zap */
