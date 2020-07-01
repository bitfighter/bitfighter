//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_DATABASE_COMMENT_THREAD_H_
#define _LEVEL_DATABASE_COMMENT_THREAD_H_

#include "tnlThread.h"
#include <string>
#include "../master/DatabaseAccessThread.h"

namespace Zap
{

class ClientGame;
class LevelDatabaseCommentThread : public Master::ThreadEntry
{
   string username;
   string user_password;
   string reqURL;
   string responseBody;
   S32 responseCode;
   S32 errorNumber;

public:
   static const string LevelDatabaseRateUrl;

   LevelDatabaseCommentThread(ClientGame* game, const string &comment);
   virtual ~LevelDatabaseCommentThread();

   void run();
   void finish();

   ClientGame* mGame;
   string mLevelId;
   string mComment;
};

} /* namespace Zap */
#endif /* _LEVEL_DATABASE_COMMENT_THREAD_H_ */
