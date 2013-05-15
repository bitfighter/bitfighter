#ifndef LEVELDATABASERATETHREAD_H_
#define LEVELDATABASERATETHREAD_H_

#include "tnlThread.h"

namespace Zap
{

class ClientGame;
class LevelDatabaseRateThread: public TNL::Thread
{
public:
   static const string LevelDatabaseRateUrl;
   LevelDatabaseRateThread(ClientGame* game, string rating);
   virtual ~LevelDatabaseRateThread();
   virtual U32 run();

   ClientGame* mGame;
   string mLevelId;
   string mRating;
};

} /* namespace Zap */
#endif /* LEVELDATABASERATETHREAD_H_ */
