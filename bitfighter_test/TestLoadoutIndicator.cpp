//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LoadoutIndicator.h"
#include "ClientGame.h"
#include "TestUtils.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(LoadoutIndicatorTest, RenderElementWidth)
{
   ClientGame *game = newClientGame();
   UI::LoadoutIndicator indicator;

   for (auto i = 0; i < static_cast<S32>(WeaponType::WeaponCount); ++i) {
      WeaponType wt = static_cast<WeaponType>(i);
      auto name = WeaponInfo::getWeaponInfo(wt).name.getString();
      auto w1 = UI::renderComponentIndicator(0, 0, name);
      auto w2 = UI::getComponentIndicatorWidth(name);
      ASSERT_EQ(w1, w2);
   }
   
   for (auto i = 0; i < static_cast<S32>(ShipModule::ModuleCount); ++i) {
      ShipModule sm = static_cast<ShipModule>(i);
      auto name = ModuleInfo::getModuleInfo(sm)->getName();
      auto w1 = UI::renderComponentIndicator(0, 0, name);
      auto w2 = UI::getComponentIndicatorWidth(name);
      ASSERT_EQ(w1, w2);
   }

   delete game;
}


TEST(LoadoutIndicatorTest, RenderWidth)
{
   ClientGame *game = newClientGame();

   UI::LoadoutIndicator indicator;
   indicator.newLoadoutHasArrived(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"));     // Sets the loadout

   // Make sure the calculated width matches the rendered width
   ASSERT_EQ(indicator.render(game), indicator.getWidth());

   delete game;
}
	
};
