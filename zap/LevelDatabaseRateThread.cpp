//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelDatabaseRateThread.h"

#include "ClientGame.h"
#include "HttpRequest.h"

#include <sstream>

using namespace std;

namespace Zap
{

// Define statics
const string LevelDatabaseRateThread::LevelDatabaseRateUrl = "bitfighter.org/pleiades/levels/rate/";

const string LevelDatabaseRateThread::RatingStrings[] = {
#define LEVEL_RATING(a, strval) strval,
   LEVEL_RATINGS_TABLE
#undef LEVEL_RATING
};


// temp, hacky solution
static string getRatingString(LevelDatabaseRateThread::LevelRating rating)
{
   if(rating == LevelDatabaseRateThread::MinusOne)  return "down";
   if(rating == LevelDatabaseRateThread::Neutral)   return "neutral";
   if(rating == LevelDatabaseRateThread::PlusOne)   return "up";

   TNLAssert(false, "Invalid rating!");

   return "";
}


// Constructor
LevelDatabaseRateThread::LevelDatabaseRateThread(ClientGame* game, LevelRating rating)
{
   mGame   = game;
   mRating = rating;
   errorNumber = 0;
   responseCode = 0;

   TNLAssert(mRating >= 0, "Invalid rating!");

   TNLAssert(mGame->isLevelInDatabase(), "Level should already have been checked by now!");
   if(!mGame->isLevelInDatabase())
   {
      mGame->displayErrorMessage("Level should already have been checked by now!");
      errorNumber = 100;
   }

   stringstream id;
   id << mGame->getLevelDatabaseId();

   reqURL = LevelDatabaseRateUrl + id.str() + "/" + getRatingString(mRating);
   username = mGame->getPlayerName();
   user_password = mGame->getPlayerPassword();
}


// Destructor
LevelDatabaseRateThread::~LevelDatabaseRateThread()
{
   // Do nothing
}


void LevelDatabaseRateThread::run()
{
   if(errorNumber == 100)
      return;

   HttpRequest req = HttpRequest(reqURL);
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]",      username);
   req.setData("data[User][user_password]", user_password);

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


void LevelDatabaseRateThread::finish()
{
   if(errorNumber == 0)
   {
      // Do nothing -- user already saw a preemptive success message
   }
   else if(errorNumber == 1)
      mGame->displayErrorMessage("!!! Error rating level: Cannot connect to server");
   else
   {
      mGame->displayErrorMessage("!!! Error rating level: %s", responseBody.c_str());
      logprintf(LogConsumer::ConsoleMsg, "Error rating level: %s (code %i)", responseBody.c_str(), responseCode);
   }
}


LevelDatabaseRateThread::LevelRating LevelDatabaseRateThread::getLevelRatingEnum(S32 rating)
{
   if(rating == -1)
      return MinusOne;
   else if(rating == 0)
      return Neutral;
   else if(rating == 1)
      return PlusOne;
   else
   {
      TNLAssert(false, "Invalid rating!");
      return Neutral;
   }
}


} /* namespace Zap */
