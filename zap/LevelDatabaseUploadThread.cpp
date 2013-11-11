//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelDatabaseUploadThread.h"

#include "UIManager.h"
#include "HttpRequest.h"
#include "ClientGame.h"
#include "UIEditor.h"
#include "UIErrorMessage.h"
#include "ScreenShooter.h"
#include "stringUtils.h"

#include <sstream>

namespace Zap
{

const string LevelDatabaseUploadThread::UploadRequest = "/levels/upload";
const string LevelDatabaseUploadThread::UploadScreenshotFilename = "upload_screenshot";

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
   editor->lockQuit("CAN'T QUIT WHILE UPLOADING");

   if(mGame->getLevelDatabaseId())
      editor->queueSetLingeringMessage("Updating Level in Pleiades [[SPINNER]]");
   else
      editor->queueSetLingeringMessage("Uploading New Level to Pleiades [[SPINNER]]");

   string fileData = readFile(joindir(mGame->getSettings()->getFolderManager()->screenshotDir, UploadScreenshotFilename + string(".png")));

   HttpRequest req(HttpRequest::LevelDatabaseBaseUrl + UploadRequest);
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]",      mGame->getPlayerName());
   req.setData("data[User][user_password]", mGame->getPlayerPassword());
   req.setData("data[Level][content]",      editor->getLevelText());
   req.addFile("data[Level][screenshot]",   UploadScreenshotFilename + string(".png"), (const U8*) fileData.c_str(), fileData.length());

   string levelgenFilename;
   levelgenFilename = mGame->getScriptName();

   if(levelgenFilename != "")
   {
      const string &levelgenContents = readFile(mGame->getSettings()->getFolderManager()->findLevelGenScript(levelgenFilename));
      req.setData("data[Level][levelgen]", levelgenContents);
   }

   if(!req.send())
      return done(editor, "Error connecting to server", false);

   S32 responseCode = req.getResponseCode();
   if(responseCode != HttpRequest::OK && responseCode != HttpRequest::Found)
   {
      editor->showUploadErrorMessage(responseCode, req.getResponseBody());

      stringstream message;
      message << "Error " << responseCode << ": " << endl << req.getResponseBody() << endl;
      logprintf(LogConsumer::LogError, "%s",  message.str().c_str());
      return done(editor, "Error uploading... see Console or Log", false);
   }

   // The server responds with the DBID of the level we just uploaded
   mGame->setLevelDatabaseId(atoi(req.getResponseBody().c_str()));
   editor->saveLevel(false, false);    // Write databaseId to the level file

   return done(editor, "Uploaded successfully", true);
}


S32 LevelDatabaseUploadThread::done(EditorUserInterface* editor, const string &message, bool success)
{
   editor->setSaveMessage(message, success);

   if(!success)
      editor->clearSaveMessage();

   editor->clearLingeringMessage();
   editor->unlockQuit();

   delete this;
   return 0;
}

}
