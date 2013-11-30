//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef LEVELDATABASEUPLOADTHREAD_H
#define LEVELDATABASEUPLOADTHREAD_H

#include "tnlThread.h"
#include <string>
#include "../master/DatabaseAccessThread.h"

using namespace std;
using namespace TNL;

namespace Zap
{

class ClientGame;
class EditorUserInterface;

class LevelDatabaseUploadThread : public Master::ThreadEntry
{
private:
   string mLevelCode;
   string mLevelgenCode;
   ClientGame* mGame;

   S32 done(EditorUserInterface* editor, const string &message, bool success);

   string uploadrequest;
   string username;
   string user_password;
   string content;
   string screenshot;
   string screenshot2;
   string levelgen;

   S32 errorNumber;
   S32 responseCode;
   string responseBody;

public:
   static const string UploadScreenshotFilename;
   static const string UploadRequest;

   LevelDatabaseUploadThread(ClientGame* game);
   virtual ~LevelDatabaseUploadThread();

   void run();
   void finish();
};

}

#endif // LEVELDATABASEUPLOADTHREAD_H
