//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelDatabaseCommentThread.h"

#include "ClientGame.h"
#include "HttpRequest.h"
#include "stringUtils.h"

#include <sstream>

using namespace std;

namespace Zap
{

// Define statics
const string LevelDatabaseCommentThread::LevelDatabaseRateUrl = "bitfighter.org/levels/comments/add";

// Constructor
LevelDatabaseCommentThread::LevelDatabaseCommentThread(ClientGame* game, const string &comment)
{
   mGame   = game;
   mComment = comment;

   errorNumber = 0;
   responseCode = 0;

   TNLAssert(mGame->isLevelInDatabase(), "Level should already have been checked by now!");
   if(!mGame->isLevelInDatabase())
   {
      mGame->displayErrorMessage("Level should already have been checked by now!");
      errorNumber = 100;
   }

   // Why on earth is this needed to get the ID??
   stringstream id;
   id << mGame->getLevelDatabaseId();
   mLevelId = id.str();

   reqURL = LevelDatabaseRateUrl;
   username = mGame->getPlayerName();
   user_password = mGame->getPlayerPassword();
}


// Destructor
LevelDatabaseCommentThread::~LevelDatabaseCommentThread()
{
   // Do nothing
}


void LevelDatabaseCommentThread::run()
{
   if(errorNumber == 100)
      return;

   HttpRequest req = HttpRequest(reqURL);
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]",      username);
   req.setData("data[User][user_password]", user_password);
   req.setData("data[Comment][level_id]",   mLevelId);
   req.setData("data[Comment][text]",       mComment);

   logprintf("2 level id: %s", mLevelId);

   if(!req.send())
   {
      errorNumber = 1;
      return;
   }

   responseCode = req.getResponseCode();
   responseBody = req.getResponseBody();

   if(responseCode != HttpRequest::OK && responseCode != HttpRequest::Found)
   {
      errorNumber = 2;
      return;
   }
}


void LevelDatabaseCommentThread::finish()
{
   if(errorNumber == 0)
   {
      mGame->displaySuccessMessage("Done");
      mGame->updateOriginalRating();
   }
   else if(errorNumber == 1)
   {
      mGame->displayErrorMessage("!!! Error posting comment to level: Cannot connect to server");
      mGame->restoreOriginalRating();
   }
   else
   {
      mGame->displayErrorMessage("!!! Error posting comment to level: %s", responseBody.c_str());
      logprintf(LogConsumer::ConsoleMsg, "Error posting comment to level: %s (code %i)", responseBody.c_str(), responseCode);
      mGame->restoreOriginalRating();
   }
}


} /* namespace Zap */
