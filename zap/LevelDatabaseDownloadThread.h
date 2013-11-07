//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef LEVELDATABASEDOWNLOADTHREAD_H
#define LEVELDATABASEDOWNLOADTHREAD_H

#include "tnlThread.h"

#include <string>

using namespace std;
namespace Zap
{

class ClientGame;
class LevelDatabaseDownloadThread : public TNL::Thread
{
typedef Thread Parent;
public:
   static string LevelRequest;
   static string LevelgenRequest;
   static const S32 UrlLength = 2048;

   explicit LevelDatabaseDownloadThread(string levelId, ClientGame* game);
   virtual ~LevelDatabaseDownloadThread();

   U32 run();
private:
   string mLevelId;
   ClientGame* mGame;
};

}

#endif // LEVELDATABASEDOWNLOADTHREAD_H
