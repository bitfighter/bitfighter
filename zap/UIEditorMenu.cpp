//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIEditorMenu.h"

#include "ClientGame.h"
#include "EditorTeam.h"
#include "gameType.h"
#include "HttpRequest.h"
#include "LevelDatabase.h"
#include "LevelDatabaseUploadThread.h"

#include "UIEditor.h"
#include "UIEditorInstructions.h"
#include "UIErrorMessage.h"
#include "UIGameParameters.h"
#include "UIManager.h"
#include "UITeamDefMenu.h"

#include "Colors.h"
#include "Cursor.h"

namespace Zap
{

// Constructor
EditorMenuUserInterface::EditorMenuUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   mMenuTitle = "EDITOR MENU";
}


// Destructor
EditorMenuUserInterface::~EditorMenuUserInterface()
{
   // Do nothing
}


void EditorMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


extern MenuItem *getWindowModeMenuItem(U32 displayMode);

//////////
// Editor menu callbacks
//////////

void reactivatePrevUICallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->reactivatePrevUI();
}


static void testLevelCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->getUI<EditorUserInterface>()->testLevel();
}


void returnToEditorCallback(ClientGame *game, U32 unused)
{
   EditorUserInterface *ui = game->getUIManager()->getUI<EditorUserInterface>();

   ui->saveLevel(true, true);                                     // Save level
   ui->setSaveMessage("Saved " + ui->getLevelFileName(), true);   // Setup a message for the user
   game->getUIManager()->reactivatePrevUI();                      // Return to editor
}


static void activateHelpCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate<EditorInstructionsUserInterface>();
}


static void activateLevelParamsCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate<GameParamUserInterface>();
}


static void activateTeamDefCallback(ClientGame *game, U32 unused)
{
   game->getUIManager()->activate<TeamDefUserInterface>();
}


void uploadToDbCallback(ClientGame *game)
{
   game->getUIManager()->activate<EditorUserInterface>();

   EditorUserInterface* editor = game->getUIManager()->getUI<EditorUserInterface>();
   editor->createNormalizedScreenshot(game);

   if(game->getGameType()->getLevelName() == "")    
   {
      editor->setSaveMessage("Failed: Level name required", false);
      return;
   }


   if(strcmp(game->getClientInfo()->getName().getString(),
         game->getGameType()->getLevelCredits().c_str()) != 0)
   {
      editor->setSaveMessage("Failed: Level author must match your username", false);
      return;
   }

   RefPtr<LevelDatabaseUploadThread> uploadThread;
   uploadThread = new LevelDatabaseUploadThread(game);
   game->getSecondaryThread()->addEntry(uploadThread);
}


void uploadToDbPromptCallback(ClientGame *game, U32 unused)
{
   EditorUserInterface *editorUI = game->getUIManager()->getUI<EditorUserInterface>();

   if(editorUI->getNeedToSave())
   {
      game->getUIManager()->displayMessageBox("Error", "Press [[Esc]] to continue", "Level must be saved before uploading");
      return;
   }

   ErrorMessageUserInterface *ui = game->getUIManager()->getUI<ErrorMessageUserInterface>();

   ui->reset();
   ui->setTitle("UPLOAD LEVEL?");
   ui->setMessage("Do you want to upload your level to the online\n\n"
                  "level database?");
   ui->setInstr("Press [[Y]] to upload,  [[Esc]] to cancel");

   ui->registerKey(KEY_Y, uploadToDbCallback);
   ui->setRenderUnderlyingUi(false);

   game->getUIManager()->activate(ui);
}


static void backToMainMenuCallback(ClientGame *game)
{
   UIManager *uiManager = game->getUIManager();

   uiManager->getUI<EditorUserInterface>()->onQuitted();
   uiManager->reactivate(uiManager->getUI<MainMenuUserInterface>());
}


static void saveLevelCallback(ClientGame *game)
{
   UIManager *uiManager = game->getUIManager();

   if(uiManager->getUI<EditorUserInterface>()->saveLevel(true, true))
      backToMainMenuCallback(game);
   else
      uiManager->reactivate(uiManager->getUI<EditorUserInterface>());
}


void quitEditorCallback(ClientGame *game, U32 unused)
{
   EditorUserInterface *editorUI = game->getUIManager()->getUI<EditorUserInterface>();

   if(editorUI->getNeedToSave())
   {
      ErrorMessageUserInterface *ui = game->getUIManager()->getUI<ErrorMessageUserInterface>();

      ui->reset();
      ui->setTitle("SAVE YOUR EDITS?");
      ui->setMessage("You have not saved your changes to this level.\n\n"
                     "Do you want to?");
      ui->setInstr("Press [[Y]] to save,  [[N]] to quit,  [[Esc]] to cancel");

      ui->registerKey(KEY_Y, saveLevelCallback);
      ui->registerKey(KEY_N, backToMainMenuCallback);
      ui->setRenderUnderlyingUi(false);

      game->getUIManager()->activate(ui);
   }
   else
     backToMainMenuCallback(game);
}

//////////

void EditorMenuUserInterface::setupMenus()
{
   InputCode keyHelp = getInputCode(mGameSettings, BINDING_HELP);

   clearMenuItems();
   addMenuItem(new MenuItem("RETURN TO EDITOR", reactivatePrevUICallback,    "", KEY_R));
   addMenuItem(getWindowModeMenuItem((U32)mGameSettings->getSetting<DisplayMode>(IniKey::WindowMode)));
   addMenuItem(new MenuItem("TEST LEVEL",       testLevelCallback,           "", KEY_T));
   addMenuItem(new MenuItem("SAVE LEVEL",       returnToEditorCallback,      "", KEY_S));
   addMenuItem(new MenuItem("HOW TO EDIT",      activateHelpCallback,        "", KEY_H, keyHelp));
   addMenuItem(new MenuItem("LEVEL PARAMETERS", activateLevelParamsCallback, "", KEY_L, KEY_F3));
   addMenuItem(new MenuItem("MANAGE TEAMS",     activateTeamDefCallback,     "", KEY_M, KEY_F2));

   // Only show the upload to database option if authenticated
   if(getGame()->getClientInfo()->isAuthenticated())
   {
      string title = LevelDatabase::isLevelInDatabase(getUIManager()->getUI<EditorUserInterface>()->getLevel()->getLevelDatabaseId()) ?
         "UPDATE LEVEL IN DB" :
         "UPLOAD LEVEL TO DB";

      addMenuItem(new MenuItem(title, uploadToDbPromptCallback, "Levels posted at " + HttpRequest::LevelDatabaseBaseUrl, KEY_U));
   }
   else
      addMenuItem(new MessageMenuItem("MUST BE LOGGED IN TO UPLOAD LEVELS TO DB", Colors::gray40));

   if(getUIManager()->getUI<EditorUserInterface>()->isQuitLocked())
      addMenuItem(new MessageMenuItem(getUIManager()->getUI<EditorUserInterface>()->getQuitLockedMessage(), Colors::red));
   else
      addStandardQuitItem();
}


void EditorMenuUserInterface::addStandardQuitItem()
{
   addMenuItem(new MenuItem("QUIT", quitEditorCallback, "", KEY_Q, KEY_UNKNOWN));
}


extern TeamPreset TeamPresets[];

void EditorUserInterface::makeSureThereIsAtLeastOneTeam()
{
   if(getTeamCount() == 0)
   {
      EditorTeam *team = new EditorTeam(TeamPresets[0]);

      mLevel->addTeam(team);
   }
}


void EditorMenuUserInterface::unlockQuit()
{
   if(mMenuItems.size() > 0)
   {
      mMenuItems.erase(mMenuItems.size() - 1);     // Remove last item
      addStandardQuitItem();                       // Replace it with QUIT
   }
}


void EditorMenuUserInterface::onEscape()
{
   Cursor::disableCursor();
   getUIManager()->reactivatePrevUI();
}



};
