#ifndef LEVELDATABASERATETHREAD_H_
#define LEVELDATABASERATETHREAD_H_

#include "tnlThread.h"
#include <string>

namespace Zap
{

class ClientGame;
class LevelDatabaseRateThread: public TNL::Thread
{
public:

#define LEVEL_RATINGS_TABLE           \
   LEVEL_RATING(PlusOne,  "up" )      \
   LEVEL_RATING(Neutral,  "neutral" ) \
   LEVEL_RATING(MinusOne, "down" )    \


   enum LevelRating {
#define LEVEL_RATING(val, b) val,
    LEVEL_RATINGS_TABLE
#undef LEVEL_RATING
    RatingsCount
};


   static const string RatingStrings[];
      
   static const string LevelDatabaseRateUrl;
   LevelDatabaseRateThread(ClientGame* game, LevelRating rating);
   virtual ~LevelDatabaseRateThread();
   virtual U32 run();

   ClientGame* mGame;
   string mLevelId;
   LevelRating mRating;
};

} /* namespace Zap */
#endif /* LEVELDATABASERATETHREAD_H_ */
