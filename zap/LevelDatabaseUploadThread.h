#ifndef LEVELDATABASEUPLOADTHREAD_H
#define LEVELDATABASEUPLOADTHREAD_H

#include "tnlThread.h"
#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

class ClientGame;
class LevelDatabaseUploadThread : public TNL::Thread
{
public:
   static const char* UploadRequest;

   LevelDatabaseUploadThread(ClientGame* game);
   virtual ~LevelDatabaseUploadThread();

   U32 run();

private:
   string mLevelCode;
   string mLevelgenCode;
   ClientGame* mGame;
};

}

#endif // LEVELDATABASEUPLOADTHREAD_H
