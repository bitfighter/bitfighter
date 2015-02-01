//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIEditor.h"

#include "ClientGame.h"
#include "UIManager.h"
#include "WallItem.h"

#include "TestUtils.h"
#include "gtest/gtest.h"

#include <cmath>

namespace Zap
{

TEST(EditorTest, findSnapVertexTest)
{
   ClientGame *clientGame = newClientGame();
   EditorUserInterface *editorUi = clientGame->getUIManager()->getUI<EditorUserInterface>();
   editorUi->setLevel(boost::shared_ptr<Level>(new Level()));

   ASSERT_EQ(0, editorUi->getLevel()->getObjectCount());     // Confirm level starts empty

   // 5 vertex wall in stair-step pattern, with middle vertex on 0,0
   editorUi->getLevel()->parseLevelLine("BarrierMaker 10 -100 -100  0 -100  0 0  100 0  100 100", "NoFile");

   ASSERT_EQ(1, editorUi->getLevel()->getObjectCount());     // Confirm object was added properly

   // Mark first 3 vertices as being selected
   WallItem *wall = static_cast<WallItem *>(editorUi->getLevel()->getObjectByIndex(0));
   wall->aselectVert(0);
   wall->aselectVert(1);
   wall->aselectVert(2);

   ASSERT_TRUE(wall->vertSelected(0));
   ASSERT_TRUE(wall->vertSelected(1));
   ASSERT_TRUE(wall->vertSelected(2));
   ASSERT_FALSE(wall->vertSelected(3));
   ASSERT_FALSE(wall->vertSelected(4));

   editorUi->mMousePos.set(3,3);          // In canvas coords; near vertex 2
   editorUi->findSnapVertex();
   EXPECT_EQ(2, editorUi->mSnapVertexIndex);

   editorUi->mMousePos.set(95,105);       // Near vertex 4, which is not selected --> closest selected is 2
   editorUi->findSnapVertex();
   EXPECT_EQ(2, editorUi->mSnapVertexIndex);

   editorUi->mMousePos.set(-88,-106);     // Near vertex 0, which is selected  
   editorUi->findSnapVertex();
   EXPECT_EQ(0, editorUi->mSnapVertexIndex);

   wall->unselectVert(0);
   ASSERT_FALSE(wall->vertSelected(0));
   ASSERT_TRUE(wall->vertSelected(1));
   ASSERT_TRUE(wall->vertSelected(2));
   ASSERT_FALSE(wall->vertSelected(3));
   ASSERT_FALSE(wall->vertSelected(4));

   editorUi->mMousePos.set(-88, -106);    // Near vertex 0, which is not selected   --> closest selected is 1
   editorUi->findSnapVertex();
   EXPECT_EQ(1, editorUi->mSnapVertexIndex);

   editorUi->mMousePos.set(3, 3);         // In canvas coords; near vertex 2
   editorUi->findSnapVertex();
   EXPECT_EQ(2, editorUi->mSnapVertexIndex);

   // Cleanup
   delete newClientGame();
}


TEST(EditorTest, panZoom)
{
   GamePair pair;
   EditorUserInterface editorUi(pair.getClient(0), NULL);

   // The basics
   F32 scale = 1.1f;
   editorUi.setDisplayScale(scale);
   ASSERT_EQ(scale, editorUi.getCurrentScale());

   Point center(33,44);
   editorUi.setDisplayCenter(center);
   ASSERT_FLOAT_EQ(center.x, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(center.y, editorUi.getDisplayCenter().y);

   // Changing center shouldn't change scale
   ASSERT_EQ(scale, editorUi.getCurrentScale());

   // Changing scale shouldn't change center
   scale = 1.4f;
   editorUi.setDisplayScale(scale);
   ASSERT_EQ(scale, editorUi.getCurrentScale());
   ASSERT_FLOAT_EQ(center.x, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(center.y, editorUi.getDisplayCenter().y);

   // Make sure center is what we expect after setting extents -- use non-proportionate size
   editorUi.setDisplayExtents(Rect(0,0,  1000,1000));
   ASSERT_FLOAT_EQ(0.6f, editorUi.getCurrentScale());    // Based on standard 800x600 window size
   ASSERT_FLOAT_EQ(500, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(500, editorUi.getDisplayCenter().y);

   Rect r;

   r = editorUi.getDisplayExtents();      
   ASSERT_FLOAT_EQ(-166.666667f, r.min.x);   // These depend on 800x600 display aspect ratio
   ASSERT_FLOAT_EQ(1166.666667f, r.max.x);
   ASSERT_TRUE(fabs(r.min.y) < .0001);        // We're getting errors here too great to use ASSERT_FLOAT_EQ
   ASSERT_FLOAT_EQ(1000       , r.max.y);

   ASSERT_FLOAT_EQ(r.getCenter().x, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(r.getCenter().y, editorUi.getDisplayCenter().y);

   editorUi.setDisplayExtents(Rect(0,0,  800,600));
   EXPECT_FLOAT_EQ(1, editorUi.getCurrentScale());    // Based on standard 800x600 window size
   ASSERT_FLOAT_EQ(400, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(300, editorUi.getDisplayCenter().y);

   r = editorUi.getDisplayExtents();
   ASSERT_FLOAT_EQ(r.getCenter().x, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(r.getCenter().y, editorUi.getDisplayCenter().y);


   // Zooming out should double the extents, but not change the center
   scale = editorUi.getCurrentScale() / 2;
   editorUi.setDisplayScale(scale);
   ASSERT_FLOAT_EQ(400, editorUi.getDisplayCenter().x);
   ASSERT_FLOAT_EQ(300, editorUi.getDisplayCenter().y);
   r = editorUi.getDisplayExtents();
   ASSERT_FLOAT_EQ(-400, r.min.x);
   ASSERT_FLOAT_EQ(-300, r.min.y);
   ASSERT_FLOAT_EQ(1200, r.max.x);
   ASSERT_FLOAT_EQ( 900, r.max.y);
}   

};
