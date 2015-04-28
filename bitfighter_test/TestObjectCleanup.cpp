//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "LevelFilesForTesting.h"

#include "ClientGame.h"
#include "GameManager.h"
#include "GameSettings.h"
#include "gameType.h"
#include "Level.h"
#include "loadoutZone.h"
#include "ServerGame.h"
#include "moveObject.h"
#include "Spawn.h"

#include "UIManager.h"
#include "UIEditor.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"

#include "gtest/gtest.h"

namespace Zap
{

   TEST(ObjectCleanupTests, Basics)
   {
      // Simple test to verify how to confirm object deletion
      TestItem *test = new TestItem();
      SafePtr<TestItem> testPtr = test;
      ASSERT_FALSE(testPtr.isNull());
      test->decRef();
      ASSERT_TRUE(testPtr.isNull());
   }


   TEST(ObjectCleanupTests, LevelCleanup)
   {
      GamePair gamePair;      // Do this to initialize NetObject things needed for code below

      Level *level = new Level(getLevelCode3());
      Vector<DatabaseObject *> objs;
      level->findObjects(ResourceItemTypeNumber, objs);
      ASSERT_EQ(1, objs.size());
      SafePtr<ResourceItem> r = static_cast<ResourceItem *>(objs[0]);
      level->findObjects(ResourceItemTypeNumber, objs);
      ASSERT_EQ(2, objs.size());
      SafePtr<Spawn> s = static_cast<Spawn *>(objs[1]);
      level->findObjects(ResourceItemTypeNumber, objs);
      ASSERT_EQ(3, objs.size());
      SafePtr<LoadoutZone> l = static_cast<LoadoutZone *>(objs[2]);

      SafePtr<GameType> gt = level->getGameType();

      ASSERT_FALSE(r.isNull() || s.isNull() || l.isNull() || gt.isNull());
      delete level;     // Should result in deletion of all game objects
      ASSERT_TRUE(r.isNull() && s.isNull() && l.isNull() && gt.isNull()) << "Expect level to clean up after itself";
   }


   // This one is a little more involved, as we will be simulating an entire editor session, including level testing
   TEST(ObjectCleanupTests, TestEditorObjectManagement)
   {
      GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
      settings->setSetting(IniKey::Nickname, string("Alfonso"));     // Set this to bypass startup screen
      settings->getFolderManager()->setLevelDir("levels");           // Need something for hosting to work
      settings->updatePlayerName("Alfonso");

      Address addr;
      ClientGame *clientGame = new ClientGame(addr, settings, new UIManager());    // ClientGame destructor will clean up UIManager


      GameManager::addClientGame(clientGame);
      UIManager *uiMgr = clientGame->getUIManager();

      uiMgr->activate<MainMenuUserInterface>();
      ASSERT_TRUE(uiMgr->isCurrentUI<MainMenuUserInterface>());
      // Cheat a little; go directly to editor
      uiMgr->getUI<EditorUserInterface>()->setLevelFileName("xyzzy");      // Reset this so we get the level entry screen
      uiMgr->activate<EditorUserInterface>();

      ASSERT_TRUE(uiMgr->isCurrentUI<EditorUserInterface>());
      EditorUserInterface *editor = uiMgr->getUI<EditorUserInterface>();
      ///// Test basic object deletion, undo system not activated
      SafePtr<TestItem> testItem = new TestItem();    // To track deletion... see "Basics" test above
      ASSERT_FALSE(testItem.isNull()) << "Just created this!";
      editor->addToEditor(testItem.getPointer());
      editor->deleteItem(0, false);                   // Low level method doesn't save undo state; object should be deleted
      ASSERT_TRUE(testItem.isNull()) << "Obj should be gone!";

      ///// Check deleting object with undo system -- do things get deleted as expected?
      testItem = new TestItem();
      ASSERT_FALSE(testItem.isNull()) << "Just created this -- shouldn't be gone yet!";
      editor->addToEditor(testItem.getPointer());
      testItem->setSelected(true);
      ASSERT_EQ(1, editor->getLevel()->findObjects_fast()->size()) << "Started with one object";
      editor->handleKeyPress(KEY_Z, "Del");     // Triggers deleteSelection(false)
      ASSERT_TRUE(testItem.isNull()) << "Obj should be gone!";
      ASSERT_EQ(0, editor->getLevel()->findObjects_fast()->size()) << "All objects deleted... none should be present";
      editor->handleKeyPress(KEY_Z, editor->getEditorBindingString(BINDING_UNDO_ACTION));  // Undo
      ASSERT_EQ(1, editor->getLevel()->findObjects_fast()->size()) << "Undeleted one object";
      ASSERT_FALSE(testItem.isValid());
      testItem = dynamic_cast<TestItem *>(editor->getLevel()->findObjects_fast()->get(0));
      ASSERT_TRUE(testItem.isValid());
      editor->handleKeyPress(KEY_Z, editor->getEditorBindingString(BINDING_REDO_ACTION));
      ASSERT_EQ(0, editor->getLevel()->findObjects_fast()->size()) << "Redid delete... none should be present";
      ASSERT_FALSE(testItem.isValid());

      ///// Test level
      Level *level = editor->getLevel();     // Keep track of the level we were editing
      editor->testLevelStart();
      ASSERT_TRUE(uiMgr->isCurrentUI<EditorUserInterface>()) << "Still in editor";
      ASSERT_TRUE(editor->getLevel()->getGameType()) << "Have valid GameType";
      GameType *gt = editor->getLevel()->getGameType();

      GameManager::getServerGame()->loadNextLevelInfo();
      ASSERT_EQ(GameManager::DoneLoadingLevels, GameManager::getHostingModePhase()) << "Only loading one level here...";
      ASSERT_TRUE(GameManager::hostGame()) << "Failure to host game!";
      ASSERT_TRUE(uiMgr->isCurrentUI<GameUserInterface>()) << "In game UI";
      GameManager::localClientQuits(GameManager::getClientGames()->get(0));
      ASSERT_TRUE(uiMgr->isCurrentUI<EditorUserInterface>()) << "Back to the editor";
      ASSERT_EQ(editor, uiMgr->getCurrentUI()) << "Expect same object";
      ASSERT_EQ(level, editor->getLevel()) << "Level should not have changed";
      ASSERT_TRUE(editor->getLevel()->getGameType()) << "Expect valid GameType";
      ASSERT_EQ(gt, editor->getLevel()->getGameType()) << "GameType should not have changed";
   }

}
