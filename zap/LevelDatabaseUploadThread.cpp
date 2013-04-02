#include "LevelDatabaseUploadThread.h"
#include "HttpRequest.h"
#include "ClientGame.h"
#include "gameType.h"
#include "UIEditor.h"
#include "stringUtils.h"

#include <sstream>

namespace Zap
{

const char* LevelDatabaseUploadThread::UploadRequest = "bitfighter.org/pleiades/levels/upload";

LevelDatabaseUploadThread::LevelDatabaseUploadThread(ClientGame* game)
   : mGame(game)
{
}

LevelDatabaseUploadThread::~LevelDatabaseUploadThread()
{
}

U32 LevelDatabaseUploadThread::run()
{
   HttpRequest req(UploadRequest);
   EditorUserInterface* editor = mGame->getUIManager()->getEditorUserInterface();
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]", mGame->getSettings()->getPlayerName());
   req.setData("data[User][user_password]", mGame->getLoginPassword());
   req.setData("data[Level][content]", editor->getLevelText());

   string levelgenFilename;
   levelgenFilename = mGame->getGameType()->getScriptName();

   if(levelgenFilename != "")
   {
      const string& levelgenContents = readFile(mGame->getSettings()->getFolderManager()->findLevelGenScript(levelgenFilename));
      req.setData("data[Level][levelgen]", levelgenContents);
   }

   if(!req.send())
   {
      editor->setSaveMessage(string("Error connecting to server"), false);

      delete this;
      return 0;
   }

   if(req.getResponseCode() != HttpRequest::OK)
   {
      stringstream message;
      message << "Error uploading level: " << req.getResponseCode();
      editor->setSaveMessage(message.str(), false);

      delete this;
      return 0;
   }

   editor->setSaveMessage("Uploaded successfully", true);

   delete this;
   return 0;
}

}