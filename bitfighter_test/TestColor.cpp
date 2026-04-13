//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Color.h"

#include "gtest/gtest.h"

namespace Zap
{

TEST(ColorTest, GrayscaleConstructors)
{
   // Defaults to white
   Color a;
   EXPECT_FLOAT_EQ(1.0f, a.r);
   EXPECT_FLOAT_EQ(1.0f, a.g);
   EXPECT_FLOAT_EQ(1.0f, a.b);

   Color b(0.25f);
   EXPECT_FLOAT_EQ(0.25f, b.r);
   EXPECT_FLOAT_EQ(0.25f, b.g);
   EXPECT_FLOAT_EQ(0.25f, b.b);

   Color c(0.125);
   EXPECT_FLOAT_EQ(0.125f, c.r);
   EXPECT_FLOAT_EQ(0.125f, c.g);
   EXPECT_FLOAT_EQ(0.125f, c.b);
}


TEST(ColorTest, CopyAndPointerConstructors)
{
   Color src(0.1f, 0.2f, 0.3f);

   Color fromCopy(src);
   EXPECT_FLOAT_EQ(src.r, fromCopy.r);
   EXPECT_FLOAT_EQ(src.g, fromCopy.g);
   EXPECT_FLOAT_EQ(src.b, fromCopy.b);

   Color fromPtr(&src);
   EXPECT_FLOAT_EQ(src.r, fromPtr.r);
   EXPECT_FLOAT_EQ(src.g, fromPtr.g);
   EXPECT_FLOAT_EQ(src.b, fromPtr.b);

   Color fromNull(static_cast<const Color *>(NULL));
   EXPECT_FLOAT_EQ(0.0f, fromNull.r);
   EXPECT_FLOAT_EQ(0.0f, fromNull.g);
   EXPECT_FLOAT_EQ(0.0f, fromNull.b);
}


TEST(ColorTest, IntegerAndHexConstructors)
{
   Color fromInt(U32(0x123456));
   EXPECT_FLOAT_EQ(0x56 / 255.0f, fromInt.r);
   EXPECT_FLOAT_EQ(0x34 / 255.0f, fromInt.g);
   EXPECT_FLOAT_EQ(0x12 / 255.0f, fromInt.b);
   EXPECT_EQ(0x123456u, fromInt.toU32());

   Color fromHex("123456");
   EXPECT_FLOAT_EQ(0x12 / 255.0f, fromHex.r);
   EXPECT_FLOAT_EQ(0x34 / 255.0f, fromHex.g);
   EXPECT_FLOAT_EQ(0x56 / 255.0f, fromHex.b);

   Color fromHexWithHash("#123456");
   EXPECT_FLOAT_EQ(0x12 / 255.0f, fromHexWithHash.r);
   EXPECT_FLOAT_EQ(0x34 / 255.0f, fromHexWithHash.g);
   EXPECT_FLOAT_EQ(0x56 / 255.0f, fromHexWithHash.b);

   Color invalidHex("12345");
   EXPECT_FLOAT_EQ(0.0f, invalidHex.r);
   EXPECT_FLOAT_EQ(0.0f, invalidHex.g);
   EXPECT_FLOAT_EQ(0.0f, invalidHex.b);
}


TEST(ColorTest, ReadAndSet)
{
   Color c(0.0f);

   const char *vals[] = { "0.2", "0.4", "0.6" };
   c.read(vals);
   EXPECT_FLOAT_EQ(0.2f, c.r);
   EXPECT_FLOAT_EQ(0.4f, c.g);
   EXPECT_FLOAT_EQ(0.6f, c.b);

   c.set("0.1 0.3 0.5");
   EXPECT_FLOAT_EQ(0.1f, c.r);
   EXPECT_FLOAT_EQ(0.3f, c.g);
   EXPECT_FLOAT_EQ(0.5f, c.b);

   c.set("0.2,0.4,0.6");
   EXPECT_FLOAT_EQ(0.2f, c.r);
   EXPECT_FLOAT_EQ(0.4f, c.g);
   EXPECT_FLOAT_EQ(0.6f, c.b);

   c.set(static_cast<const Color *>(NULL));
   EXPECT_FLOAT_EQ(0.0f, c.r);
   EXPECT_FLOAT_EQ(0.0f, c.g);
   EXPECT_FLOAT_EQ(0.0f, c.b);

   // Invalid strings should not overwrite an existing valid color.
   c.set("0.2,0.4,0.6");
   c.set("broken");
   EXPECT_FLOAT_EQ(0.2f, c.r);
   EXPECT_FLOAT_EQ(0.4f, c.g);
   EXPECT_FLOAT_EQ(0.6f, c.b);
}


TEST(ColorTest, Interpolation)
{
   Color c1(1.0f, 0.0f, 0.0f);
   Color c2(0.0f, 0.0f, 1.0f);
   Color out(0.0f);

   out.interp(0.0f, c1, c2);
   EXPECT_FLOAT_EQ(1.0f, out.r);
   EXPECT_FLOAT_EQ(0.0f, out.g);
   EXPECT_FLOAT_EQ(0.0f, out.b);

   out.interp(1.0f, c1, c2);
   EXPECT_FLOAT_EQ(0.0f, out.r);
   EXPECT_FLOAT_EQ(0.0f, out.g);
   EXPECT_FLOAT_EQ(1.0f, out.b);

   out.interp(0.25f, c1, c2);
   EXPECT_FLOAT_EQ(0.75f, out.r);
   EXPECT_FLOAT_EQ(0.0f, out.g);
   EXPECT_FLOAT_EQ(0.25f, out.b);
}


TEST(ColorTest, ConversionHelpers)
{
   Color c(1.0f, 0.5f, 0.0f);
   EXPECT_EQ(0x0080FFu, c.toU32());      // 0.5 * 255 = 127.5, rounded to 128 (0x80)
   EXPECT_EQ("FF8000", c.toHexString());
   EXPECT_EQ("1 0.5 0", c.toRGBString());
}


TEST(ColorTest, ClampingConversion)
{
   Color high(2.0f, 5.0f, 10.0f);
   EXPECT_EQ(0x00FFFFFFu, high.toU32());
   EXPECT_EQ("FFFFFF", high.toHexString());

   Color low(-1.0f, -0.5f, -2.0f);
   EXPECT_EQ(0x00000000u, low.toU32());
   EXPECT_EQ("000000", low.toHexString());
}


TEST(ColorTest, EnsureMinimumBrightness)
{
   Color dark(0.0f, 0.0f, 0.0f);
   dark.ensureMinimumBrightness();

   EXPECT_NEAR(0.2f, dark.r, 1e-4f);
   EXPECT_NEAR(0.2f, dark.g, 1e-4f);
   EXPECT_NEAR(0.2f, dark.b, 1e-4f);

   Color bright(1.0f, 0.0f, 0.0f);
   bright.ensureMinimumBrightness();
   EXPECT_FLOAT_EQ(1.0f, bright.r);
   EXPECT_FLOAT_EQ(0.0f, bright.g);
   EXPECT_FLOAT_EQ(0.0f, bright.b);
}


TEST(ColorTest, Desaturate)
{
   Color c(1.0f, 0.0f, 0.0f);
   c.desaturate(1.0f);

   EXPECT_FLOAT_EQ(0.3f, c.r);
   EXPECT_FLOAT_EQ(0.3f, c.g);
   EXPECT_FLOAT_EQ(0.3f, c.b);

   Color unchanged(0.7f, 0.1f, 0.2f);
   unchanged.desaturate(0.0f);
   EXPECT_FLOAT_EQ(0.7f, unchanged.r);
   EXPECT_FLOAT_EQ(0.1f, unchanged.g);
   EXPECT_FLOAT_EQ(0.2f, unchanged.b);
}


TEST(ColorTest, Operators)
{
   Color a(0.1f, 0.2f, 0.3f);
   Color b(0.4f, 0.5f, 0.6f);

   Color add = a + b;
   EXPECT_FLOAT_EQ(0.5f, add.r);
   EXPECT_FLOAT_EQ(0.7f, add.g);
   EXPECT_FLOAT_EQ(0.9f, add.b);

   Color sub = b - a;
   EXPECT_FLOAT_EQ(0.3f, sub.r);
   EXPECT_FLOAT_EQ(0.3f, sub.g);
   EXPECT_FLOAT_EQ(0.3f, sub.b);

   Color scaled = a * 2.0f;
   EXPECT_FLOAT_EQ(0.2f, scaled.r);
   EXPECT_FLOAT_EQ(0.4f, scaled.g);
   EXPECT_FLOAT_EQ(0.6f, scaled.b);

   a += b;
   EXPECT_FLOAT_EQ(0.5f, a.r);
   EXPECT_FLOAT_EQ(0.7f, a.g);
   EXPECT_FLOAT_EQ(0.9f, a.b);

   a -= b;
   EXPECT_FLOAT_EQ(0.1f, a.r);
   EXPECT_FLOAT_EQ(0.2f, a.g);
   EXPECT_FLOAT_EQ(0.3f, a.b);

   a *= 10.0f;
   EXPECT_FLOAT_EQ(1.0f, a.r);
   EXPECT_FLOAT_EQ(2.0f, a.g);
   EXPECT_FLOAT_EQ(3.0f, a.b);
}

}
