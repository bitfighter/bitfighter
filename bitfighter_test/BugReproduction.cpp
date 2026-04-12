#include "Color.h"
#include "stringUtils.h"
#include "Point.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(BugReproduction, ColorRounding)
{
   // 254.99 / 255.0 is approx 0.99996078
   // Truncation will result in 254
   // Rounding should result in 255
   Color c(254.999f / 255.0f, 254.999f / 255.0f, 254.999f / 255.0f);

   // Current implementation:
   // U32 ir = (U32)(CLAMP(r, 0.0f, 1.0f) * 255.0f);
   // (U32)(0.999996... * 255.0) = (U32)(254.999) = 254

   EXPECT_EQ(0xFFFFFF, c.toU32()) << "Color::toU32() should round to nearest integer";
   EXPECT_EQ("FFFFFF", c.toHexString()) << "Color::toHexString() should round to nearest integer";
}

TEST(BugReproduction, ParseStringNullAndEmpty)
{
   Vector<string> words;

   // NULL input - currently would crash
   parseString((const char*)NULL, words, ',');
   EXPECT_TRUE(words.empty());

   // Empty string input
   parseString("", words, ',');
   EXPECT_TRUE(words.empty()) << "parseString with empty input should return empty vector, but got size " << words.size();
}

TEST(BugReproduction, PointConstOperators)
{
   const Point p1(10.0f, 20.0f);
   const Point p2(2.0f, 4.0f);

   // The following lines would fail to compile if operators are not const
   Point res0 = p1 * 2.0f;
   Point res1 = p1 / 2.0f;
   Point res2 = p1 * p2;
   Point res3 = p1 / p2;

   EXPECT_FLOAT_EQ(20.0f, res0.x);
   EXPECT_FLOAT_EQ(40.0f, res0.y);
   EXPECT_FLOAT_EQ(5.0f, res1.x);
   EXPECT_FLOAT_EQ(10.0f, res1.y);
   EXPECT_FLOAT_EQ(20.0f, res2.x);
   EXPECT_FLOAT_EQ(80.0f, res2.y);
   EXPECT_FLOAT_EQ(5.0f, res3.x);
   EXPECT_FLOAT_EQ(5.0f, res3.y);
}

} // namespace Zap
