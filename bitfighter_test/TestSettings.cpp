//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Settings.h"
#include "config.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(SettingsTest, SetValFromString)
{
   Settings settings;
   settings.add(new Setting<string>     ("strName",  "defval",              "SettingName1", "Section", "description"));
   settings.add(new Setting<S32>        ("S32Name",  123,                   "SettingName2", "Section", "description"));
   settings.add(new Setting<YesNo>      ("YesNoYes", Yes,                   "YesNoYes",     "Section", "description"));
   settings.add(new Setting<YesNo>      ("YesNoNo",  No,                    "YesNoNo",      "Section", "description"));
   settings.add(new Setting<DisplayMode>("DispMode", DISPLAY_MODE_WINDOWED, "DispMode",     "Section", "description"));
   settings.add(new Setting<RelAbs>     ("RelAbs",   Relative,              "RelAbs",       "Section", "description"));

   // Check default values
   // Get a string representation of the value
   ASSERT_EQ("defval", settings.getStrVal("strName"));
   ASSERT_EQ("123",    settings.getStrVal("S32Name"));

   // Get the raw values
   ASSERT_EQ("defval", settings.getVal<string>("strName"));
   ASSERT_EQ(123,      settings.getVal<S32>("S32Name"));

   // Set some new values
   settings.setVal("strName", string("newVal"));
   settings.setVal("S32Name", 321);
   ASSERT_EQ("newVal", settings.getVal<string>("strName"));
   ASSERT_EQ(321,      settings.getVal<S32>("S32Name"));

   // Check YesNo/DisplayMode conversions

   // Check setting YesNo by string
   settings.getSetting("YesNoYes")->setValFromString("No");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));
   settings.getSetting("YesNoYes")->setValFromString("Yes");
   ASSERT_EQ(Yes, settings.getVal<YesNo>("YesNoYes"));

   // Different cases
   settings.getSetting("YesNoYes")->setValFromString("NO");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));
   settings.getSetting("YesNoYes")->setValFromString("yes");
   ASSERT_EQ(Yes, settings.getVal<YesNo>("YesNoYes"));

   // Unknwon values - should go to No
   settings.getSetting("YesNoYes")->setValFromString("abcdefg");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));
   // Empty string
   settings.getSetting("YesNoYes")->setValFromString("");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));

   // Reset and check boolean equality
   settings.getSetting("YesNoYes")->setValFromString("Yes");
   settings.getSetting("YesNoNo")->setValFromString("No");
   ASSERT_TRUE(settings.getVal<YesNo>("YesNoYes"));
   ASSERT_FALSE(settings.getVal<YesNo>("YesNoNo"));

   // Relative/Absolute
   settings.getSetting("RelAbs")->setValFromString("absolute");
   ASSERT_EQ(Absolute, settings.getVal<RelAbs>("RelAbs"));
   settings.getSetting("RelAbs")->setValFromString("RELATIVE");
   ASSERT_EQ(Relative, settings.getVal<RelAbs>("RelAbs"));
}
   

};
