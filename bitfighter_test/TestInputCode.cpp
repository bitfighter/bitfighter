//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "InputCode.h"

#include "gtest/gtest.h"
#include <string>

namespace Zap
{

TEST(InputStringTest, validStrings)
{
   InputCodeManager::initializeKeyNames();

   // Single character
   EXPECT_TRUE(InputCodeManager::isValidInputString("V"));         

   // Single modifier
   EXPECT_TRUE(InputCodeManager::isValidInputString("Ctrl+H"));    
   EXPECT_TRUE(InputCodeManager::isValidInputString("Shift+T"));
   EXPECT_TRUE(InputCodeManager::isValidInputString("Alt+X"));

   // Double modifier
   EXPECT_TRUE(InputCodeManager::isValidInputString("Alt+Shift+Q"));

   // Other names
   EXPECT_TRUE(InputCodeManager::isValidInputString("Right Arrow"));
   EXPECT_TRUE(InputCodeManager::isValidInputString("Alt+Keypad Enter"));

   // Invalid keys
   EXPECT_FALSE(InputCodeManager::isValidInputString("123"));              // Garbage
   EXPECT_TRUE(InputCodeManager::isValidInputString("#"));                 // Not a key -- actually, it is... KEY_HASH
   EXPECT_FALSE(InputCodeManager::isValidInputString("|"));                // Not a key -- for real this time
   EXPECT_FALSE(InputCodeManager::isValidInputString("Flux+P"));           // Invalid modifier
   EXPECT_FALSE(InputCodeManager::isValidInputString("Shift+Shift+G"));    // Double modifier
   EXPECT_FALSE(InputCodeManager::isValidInputString("Shift+Alt+Q"));      // Out-of-order modifiers
   EXPECT_FALSE(InputCodeManager::isValidInputString("Alt-Shift+A"));      // Incorrect joiner
   EXPECT_FALSE(InputCodeManager::isValidInputString("Ctrl+shift+A"));     // Incorrect case
}

TEST(InputStringTest, normalizeStrings)
{
   InputCodeManager::initializeKeyNames();

   string normalized;

   // Valid inputStrings:
   normalized = InputCodeManager::normalizeInputString("C");   
   EXPECT_EQ("C", normalized);                     EXPECT_TRUE(InputCodeManager::isValidInputString(normalized));
   
   normalized = InputCodeManager::normalizeInputString("Alt+V");   
   EXPECT_EQ("Alt+V", normalized);                 EXPECT_TRUE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("Alt+Alt+X");   
   EXPECT_EQ("Alt+X", normalized);                 EXPECT_TRUE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("Alt+Ctrl+Up Arrow");   
   EXPECT_EQ("Ctrl+Alt+Up Arrow", normalized);     EXPECT_TRUE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("ctrl+g");   
   EXPECT_EQ("Ctrl+G", normalized);                EXPECT_TRUE(InputCodeManager::isValidInputString(normalized));

   // Invalid inputStrings:
   normalized = InputCodeManager::normalizeInputString("Bullocks");   
   EXPECT_EQ("", normalized);                      EXPECT_FALSE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("Ctrl+OO");   
   EXPECT_EQ("", normalized);                      EXPECT_FALSE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("Ctrl+Ctrl");
   EXPECT_EQ("", normalized);                      EXPECT_FALSE(InputCodeManager::isValidInputString(normalized));

}


// Loadout presets 1-10 use Ctrl/Alt with keys 1-9 and 0 (preset 10)
TEST(LoadoutPresetKeyTest, presetKeysAreModifiedDigits)
{
   InputCodeManager::initializeKeyNames();

   const InputCode ctrlKeys[] = { KEY_CTRL_1, KEY_CTRL_2, KEY_CTRL_3, KEY_CTRL_4, KEY_CTRL_5, KEY_CTRL_6,
                                  KEY_CTRL_7, KEY_CTRL_8, KEY_CTRL_9, KEY_CTRL_0 };
   const InputCode altKeys[]  = { KEY_ALT_1,  KEY_ALT_2,  KEY_ALT_3,  KEY_ALT_4,  KEY_ALT_5,  KEY_ALT_6,
                                  KEY_ALT_7,  KEY_ALT_8,  KEY_ALT_9,  KEY_ALT_0 };
   const char *ctrlNames[] = { "Ctrl+1", "Ctrl+2", "Ctrl+3", "Ctrl+4", "Ctrl+5", "Ctrl+6", "Ctrl+7", "Ctrl+8", "Ctrl+9", "Ctrl+0" };
   const char *altNames[]  = { "Alt+1",  "Alt+2",  "Alt+3",  "Alt+4",  "Alt+5",  "Alt+6",  "Alt+7",  "Alt+8",  "Alt+9",  "Alt+0" };
   const InputCode baseKeys[] = { KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0 };

   for(S32 i = 0; i < 10; i++)
   {
      EXPECT_TRUE(InputCodeManager::isCtrlKey(ctrlKeys[i]));
      EXPECT_TRUE(InputCodeManager::isAltKey(altKeys[i]));
      EXPECT_EQ(baseKeys[i], InputCodeManager::getBaseKey(ctrlKeys[i]));
      EXPECT_EQ(baseKeys[i], InputCodeManager::getBaseKey(altKeys[i]));
      EXPECT_EQ(string(ctrlNames[i]), string(InputCodeManager::inputCodeToString(ctrlKeys[i])));
      EXPECT_EQ(string(altNames[i]),  string(InputCodeManager::inputCodeToString(altKeys[i])));
      EXPECT_TRUE(InputCodeManager::isValidInputString(ctrlNames[i]));
      EXPECT_TRUE(InputCodeManager::isValidInputString(altNames[i]));
   }
}


}
