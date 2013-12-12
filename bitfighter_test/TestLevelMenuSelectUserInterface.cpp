//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ClientGame.h"
#include "UIMenus.h"
#include "UIManager.h"

#include "TestUtils.h"

#include "gtest/gtest.h"

namespace Zap
{

TEST(LevelMenuSelectUserInterfaceTests, GetIndexOfNext)
{
   ClientGame *game = newClientGame();

   // Want to test getIndexOfNext(), which is a slightly complex function.  Need to start by setting up a menu.
   LevelMenuSelectUserInterface *ui = game->getUIManager()->getUI<LevelMenuSelectUserInterface>();      // Cleaned up when game goes out of scope

   // These should be alphabetically sorted
   ui->addMenuItem(new MenuItem("Aardvark"));    //  0
   ui->addMenuItem(new MenuItem("Assinine"));    //  1
   ui->addMenuItem(new MenuItem("Bouy"));        //  2
   ui->addMenuItem(new MenuItem("Boy"));         //  3
   ui->addMenuItem(new MenuItem("C"));           //  4
   ui->addMenuItem(new MenuItem("Cat"));         //  5
   ui->addMenuItem(new MenuItem("Cc"));          //  6
   ui->addMenuItem(new MenuItem("Chop"));        //  7
   ui->addMenuItem(new MenuItem("Chump"));       //  8
   ui->addMenuItem(new MenuItem("Dog"));         //  9
   ui->addMenuItem(new MenuItem("Doug"));        // 10
   ui->addMenuItem(new MenuItem("Eat"));         // 11
   ui->addMenuItem(new MenuItem("Eating"));      // 12
   ui->addMenuItem(new MenuItem("Eel"));         // 13
   ui->addMenuItem(new MenuItem("Eels"));        // 14
   ui->addMenuItem(new MenuItem("Eggs"));        // 15


   // Some random checks
   ui->selectedIndex = 1;
   ASSERT_EQ(ui->getIndexOfNext("a"), 0);
   ASSERT_EQ(ui->getIndexOfNext("boy"), 3);
   ASSERT_EQ(ui->getIndexOfNext("c"), 4);
   ASSERT_EQ(ui->getIndexOfNext("ch"), 7);
   ASSERT_EQ(ui->getIndexOfNext("cho"), 7);
   ASSERT_EQ(ui->getIndexOfNext("chop"), 7);

   // Check cycling of the Cs
   ui->selectedIndex = 3;
   ASSERT_EQ(ui->getIndexOfNext("c"), 4);
   ui->selectedIndex = 4;
   ASSERT_EQ(ui->getIndexOfNext("c"), 5);
   ui->selectedIndex = 5;
   ASSERT_EQ(ui->getIndexOfNext("c"), 6);
   ui->selectedIndex = 6;
   ASSERT_EQ(ui->getIndexOfNext("c"), 7);
   ui->selectedIndex = 7;
   ASSERT_EQ(ui->getIndexOfNext("c"), 8);
   ui->selectedIndex = 8;
   ASSERT_EQ(ui->getIndexOfNext("c"), 4);

   // Check wrapping
   ui->selectedIndex = 9;
   ASSERT_EQ(ui->getIndexOfNext("a"), 0);
   ui->selectedIndex = 15;     // last item
   ASSERT_EQ(ui->getIndexOfNext("a"), 0);

   // Check repeated hammering on current item
   ui->selectedIndex = 12;
   ASSERT_EQ(ui->getIndexOfNext("e"), 13);    // Single letter advances to next of that letter
   ASSERT_EQ(ui->getIndexOfNext("ea"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eat"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eati"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eatin"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eating"), 12);

   // Check for not found items -- should return current index
   ASSERT_EQ(ui->getIndexOfNext("eatingx"), 12); 
   ASSERT_EQ(ui->getIndexOfNext("flummoxed"), 12); 

   ui->selectedIndex = 8;
   ASSERT_EQ(ui->getIndexOfNext("chop"), 7);

   delete game;
}
   
};
