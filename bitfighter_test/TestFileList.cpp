//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FileList.h"

#include "gtest/gtest.h"
#include <string>

namespace Zap
{

   TEST(FileListTest, basics)
   {
      FileList fileList;
      ASSERT_FALSE(fileList.isOk()) << "Needs to be initialized with valid files first!";
   }


}
