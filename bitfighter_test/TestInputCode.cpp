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
   EXPECT_FALSE(InputCodeManager::isValidInputString("#"));                // Not a key
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
   EXPECT_EQ("Ctrl+G", normalized);     EXPECT_TRUE(InputCodeManager::isValidInputString(normalized));

   // Invalid inputStrings:
   normalized = InputCodeManager::normalizeInputString("Bullocks");   
   EXPECT_EQ("", normalized);                      EXPECT_FALSE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("Ctrl+OO");   
   EXPECT_EQ("", normalized);                      EXPECT_FALSE(InputCodeManager::isValidInputString(normalized));

   normalized = InputCodeManager::normalizeInputString("Ctrl+Ctrl");   
   EXPECT_EQ("", normalized);                      EXPECT_FALSE(InputCodeManager::isValidInputString(normalized));

}


}
