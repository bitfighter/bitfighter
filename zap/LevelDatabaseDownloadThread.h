//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef LEVELDATABASEDOWNLOADTHREAD_H
#define LEVELDATABASEDOWNLOADTHREAD_H

#include "tnlThread.h"
#include "../master/DatabaseAccessThread.h"

#include <string>

using namespace std;
namespace Zap
{

class ClientGame;
class LevelDatabaseDownloadThread  : public Master::ThreadEntry
{
typedef Thread Parent;
public:
   static string LevelRequest;
   static string LevelgenRequest;
   static const S32 UrlLength = 2048;

   explicit LevelDatabaseDownloadThread(string levelId, ClientGame* game);
   virtual ~LevelDatabaseDownloadThread();

   void run();
   void finish();
private:
   char url[UrlLength];
   string mLevelId;
   string levelDir;
   string levelFileName;
   string levelGenFileName;
   ClientGame* mGame;
   S32 errorNumber;
};

}

#endif // LEVELDATABASEDOWNLOADTHREAD_H
