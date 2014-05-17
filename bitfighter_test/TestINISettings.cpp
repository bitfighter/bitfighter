//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "../zap/config.h"  // Multiple 'config.h' ??

namespace Zap
{

TEST(INISettingsTest, PackUnpack)
{
   static const S32 count = 5;
   bool items[count];

   // Set all bits to false
   IniSettings::clearbits(items, count);
   ASSERT_FALSE(items[0] || items[1] || items[2] || items[3] || items[4]);

   // Set from proper length string
   IniSettings::iniStringToBitArray("YYNNN", items, count);
   ASSERT_TRUE(items[0] && items[1]);
   ASSERT_FALSE(items[2] || items[3] || items[4]);

   // Set all bits to false
   IniSettings::clearbits(items, count);
   ASSERT_FALSE(items[0] || items[1] || items[2] || items[3] || items[4]);

   // Set from short string
   IniSettings::iniStringToBitArray("YNY", items, count);
   ASSERT_TRUE(items[0] && items[2]);
   ASSERT_FALSE(items[1] || items[3] || items[4]);

   // Set from long string
   IniSettings::clearbits(items, count);
   IniSettings::iniStringToBitArray("NNYYYYYYYYYY", items, count);
   ASSERT_TRUE(items[2] && items[3] && items[4]);
   ASSERT_FALSE(items[0] || items[1]);

   // Set from bogus string
   IniSettings::clearbits(items, count);
   IniSettings::iniStringToBitArray("yyYXE", items, count);
   ASSERT_TRUE(items[2]);
   ASSERT_FALSE(items[0] || items[1] || items[3] || items[4]);

   // Now we know unpack works, so testing pack should be easy
   IniSettings::clearbits(items, count);
   string vals = "YYNNY";

   IniSettings::iniStringToBitArray(vals, items, count);
   ASSERT_EQ(IniSettings::bitArrayToIniString(items, count), vals);
}   

};
