#include "LevelDatabaseUploadThread.h"

#include "UIManager.h"
#include "HttpRequest.h"
#include "ClientGame.h"
#include "UIEditor.h"
#include "ScreenShooter.h"
#include "stringUtils.h"

#include <sstream>

namespace Zap
{

const string LevelDatabaseUploadThread::UploadRequest = "bitfighter.org/pleiades/levels/upload";

LevelDatabaseUploadThread::LevelDatabaseUploadThread(ClientGame* game)
{
   mGame = game;
}

LevelDatabaseUploadThread::~LevelDatabaseUploadThread()
{
   // Do nothing
}

U32 LevelDatabaseUploadThread::run()
{
   EditorUserInterface* editor = mGame->getUIManager()->getUI<EditorUserInterface>();

   if(mGame->getLevelDatabaseId())
   {
      editor->setSaveMessage("Updating Level...", true);
   }
   else
   {
      editor->setSaveMessage("Uploading New Level...", true);
   }

   string fileData = readFile(joindir(mGame->getSettings()->getFolderManager()->screenshotDir, "upload_screenshot.png"));

   HttpRequest req(UploadRequest);
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]",      mGame->getPlayerName());
   req.setData("data[User][user_password]", mGame->getPlayerPassword());
   req.setData("data[Level][content]", editor->getLevelText());
   req.addFile("data[Level][screenshot]", "upload_screenshot.png", (const U8*) fileData.c_str(), fileData.length());

   string levelgenFilename;
   levelgenFilename = mGame->getScriptName();

   if(levelgenFilename != "")
   {
      const string &levelgenContents = readFile(mGame->getSettings()->getFolderManager()->findLevelGenScript(levelgenFilename));
      req.setData("data[Level][levelgen]", levelgenContents);
   }

   if(!req.send())
   {
      editor->setSaveMessage(string("Error connecting to server"), false);

      delete this;
      return 0;
   }

   S32 responseCode = req.getResponseCode();
   if(responseCode != HttpRequest::OK && responseCode != HttpRequest::Found)
   {
      stringstream message;
      message << "Error uploading level: " << responseCode;
      editor->setSaveMessage(message.str(), false);

      delete this;
      return 0;
   }

   editor->setSaveMessage("Uploaded successfully", true);
   mGame->setLevelDatabaseId(atoi(req.getResponseBody().c_str()));

   delete this;
   return 0;
}

}
