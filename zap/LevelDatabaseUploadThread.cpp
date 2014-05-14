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
   errorNumber = 0;
   EditorUserInterface* editor = mGame->getUIManager()->getUI<EditorUserInterface>();
   editor->lockQuit("CAN'T QUIT WHILE UPLOADING");

   if(mGame->getLevelDatabaseId())
      editor->queueSetLingeringMessage("Updating Level in Pleiades [[SPINNER]]");
   else
      editor->queueSetLingeringMessage("Uploading New Level to Pleiades [[SPINNER]]");

   string fileData;
   readFile(joindir(mGame->getSettings()->getFolderManager()->getScreenshotDir(), UploadScreenshotFilename + string(".png")), fileData);

   username = mGame->getPlayerName();
   user_password = mGame->getPlayerPassword();
   content = editor->getLevelText();
   screenshot = UploadScreenshotFilename + string(".png");
   screenshot2 = fileData;


   string levelgenFilename;
   levelgenFilename = mGame->getScriptName();

   if(levelgenFilename != "")
      readFile(mGame->getSettings()->getFolderManager()->findLevelGenScript(levelgenFilename), levelgen);

   uploadrequest = HttpRequest::LevelDatabaseBaseUrl + UploadRequest;
}


LevelDatabaseUploadThread::~LevelDatabaseUploadThread()
{
   // Do nothing
}


void LevelDatabaseUploadThread::run()
{

   HttpRequest req(uploadrequest);
   req.setMethod(HttpRequest::PostMethod);
   req.setData("data[User][username]",      username);
   req.setData("data[User][user_password]", user_password);
   req.setData("data[Level][content]",      content);
   req.addFile("data[Level][screenshot]",   screenshot, (U8*) screenshot2.c_str(), screenshot2.length());

   string levelgenFilename;
   levelgenFilename = mGame->getScriptName();

   if(levelgen != "")
      req.setData("data[Level][levelgen]", levelgen);

   if(!req.send())
   {
      errorNumber = 1;
      return;
   }

   responseCode = req.getResponseCode();
   responseBody = req.getResponseBody();
   if(responseCode != HttpRequest::OK && responseCode != HttpRequest::Found)
   {
      errorNumber = 2;
      return;
   }
}

void LevelDatabaseUploadThread::finish()
{
   EditorUserInterface* editor = mGame->getUIManager()->getUI<EditorUserInterface>();
   if(!editor)
      return;

   if(errorNumber == 0)
   {
      // The server responds with the DBID of the level we just uploaded
      mGame->setLevelDatabaseId(atoi(responseBody.c_str()));
      editor->saveLevel(false, false);    // Write databaseId to the level file

      done(editor, "Uploaded successfully", true);
   }
   else if(errorNumber == 1)
   {
      done(editor, "Error connecting to server", false);
   }
   else if(errorNumber == 2)
   {
      editor->showUploadErrorMessage(responseCode, responseBody);

      stringstream message;
      message << "Error " << responseCode << ": " << endl << responseBody << endl;
      logprintf(LogConsumer::LogError, "%s",  message.str().c_str());
      done(editor, "Error uploading... see Console or Log", false);
   }
   else
   {
      done(editor, "Unknown error", false);
   }
}

S32 LevelDatabaseUploadThread::done(EditorUserInterface* editor, const string &message, bool success)
{
   editor->setSaveMessage(message, success);

   if(!success)
      editor->clearSaveMessage();

   editor->clearLingeringMessage();
   editor->unlockQuit();

   return 0;
}

}
