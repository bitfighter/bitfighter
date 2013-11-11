//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef LEVELDATABASEUPLOADTHREAD_H
#define LEVELDATABASEUPLOADTHREAD_H

#include "tnlThread.h"
#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

class ClientGame;
class EditorUserInterface;

class LevelDatabaseUploadThread : public TNL::Thread
{
private:
   string mLevelCode;
   string mLevelgenCode;
   ClientGame* mGame;

   S32 done(EditorUserInterface* editor, const string &message, bool success);

public:
   static const string UploadScreenshotFilename;
   static const string UploadRequest;

   LevelDatabaseUploadThread(ClientGame* game);
   virtual ~LevelDatabaseUploadThread();

   U32 run();
};

}

#endif // LEVELDATABASEUPLOADTHREAD_H
