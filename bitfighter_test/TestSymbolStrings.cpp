//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "SymbolShape.h"
#include "gtest/gtest.h"

using namespace Zap::UI;

namespace Zap
{


TEST(SymbolStringTest, Height)
{
   S32 fontSize = 10;
   FontContext context = ErrorMsgContext;
   string msg = "Short Test Line";
   SymbolShapePtr one = SymbolShapePtr(new SymbolString(msg, NULL, context, fontSize, true));
   EXPECT_EQ(10, one->getHeight());

   string emptyMsg = "";
   SymbolShapePtr two = SymbolShapePtr(new SymbolString(emptyMsg, NULL, context, fontSize, true));
   EXPECT_EQ(10, one->getHeight());

   SymbolShapePtr three = SymbolShapePtr(new SymbolString(emptyMsg, NULL, context, fontSize, false));
   EXPECT_EQ(0, one->getHeight());

}

   
};
