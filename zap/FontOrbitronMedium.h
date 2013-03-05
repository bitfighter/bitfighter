#ifdef TNL_OS_MOBILE
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif

#include "freeglut_stroke.h"

// space
static const SFG_StrokeStrip chr_32_strip[] = { { 0, NULL} };

SFG_StrokeChar chr_32 = { 290 * fact, 0, chr_32_strip };


// exclam.glif
static const SFG_StrokeVertex chr_33_part_0[] = {
   { 166.0f * fact, 0.0f * fact },
   { 166.0f * fact, 108.0f * fact },
   { 58.0f * fact, 108.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 166.0f * fact, 0.0f * fact }
};

// exclam.glif
static const SFG_StrokeVertex chr_33_part_1[] = {
   { 58.0f * fact, 200.0f * fact },
   { 166.0f * fact, 200.0f * fact },
   { 166.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 200.0f * fact }
};

static const SFG_StrokeStrip chr_33_strip[] = { {5, chr_33_part_0}, {5, chr_33_part_1} };

SFG_StrokeChar chr_33 = { 220 * fact, 2, chr_33_strip };


// quotedbl.glif
static const SFG_StrokeVertex chr_34_part_0[] = {
   { 167.0f * fact, 719.0f * fact },
   { 59.0f * fact, 719.0f * fact },
   { 59.0f * fact, 557.0f * fact },
   { 167.0f * fact, 557.0f * fact },
   { 167.0f * fact, 719.0f * fact }
};

// quotedbl.glif
static const SFG_StrokeVertex chr_34_part_1[] = {
   { 314.0f * fact, 719.0f * fact },
   { 206.0f * fact, 719.0f * fact },
   { 206.0f * fact, 557.0f * fact },
   { 314.0f * fact, 557.0f * fact },
   { 314.0f * fact, 719.0f * fact }
};

static const SFG_StrokeStrip chr_34_strip[] = { {5, chr_34_part_0}, {5, chr_34_part_1} };

SFG_StrokeChar chr_34 = { 372 * fact, 2, chr_34_strip };


// numbersign.glif
static const SFG_StrokeVertex chr_35_part_0[] = {
   { 765.0f * fact, 558.0f * fact },
   { 680.0f * fact, 558.0f * fact },
   { 734.0f * fact, 717.0f * fact },
   { 626.0f * fact, 717.0f * fact },
   { 572.0f * fact, 558.0f * fact },
   { 357.0f * fact, 558.0f * fact },
   { 411.0f * fact, 717.0f * fact },
   { 303.0f * fact, 717.0f * fact },
   { 249.0f * fact, 558.0f * fact },
   { 63.0f * fact, 558.0f * fact },
   { 63.0f * fact, 450.0f * fact },
   { 215.0f * fact, 450.0f * fact },
   { 157.0f * fact, 272.0f * fact },
   { 32.0f * fact, 272.0f * fact },
   { 32.0f * fact, 164.0f * fact },
   { 123.0f * fact, 164.0f * fact },
   { 73.0f * fact, 0.0f * fact },
   { 181.0f * fact, 0.0f * fact },
   { 231.0f * fact, 164.0f * fact },
   { 446.0f * fact, 164.0f * fact },
   { 396.0f * fact, 0.0f * fact },
   { 504.0f * fact, 0.0f * fact },
   { 554.0f * fact, 164.0f * fact },
   { 734.0f * fact, 164.0f * fact },
   { 734.0f * fact, 272.0f * fact },
   { 588.0f * fact, 272.0f * fact },
   { 646.0f * fact, 450.0f * fact },
   { 765.0f * fact, 450.0f * fact },
   { 765.0f * fact, 558.0f * fact }
};

// numbersign.glif
static const SFG_StrokeVertex chr_35_part_1[] = {
   { 265.0f * fact, 272.0f * fact },
   { 323.0f * fact, 450.0f * fact },
   { 538.0f * fact, 450.0f * fact },
   { 480.0f * fact, 272.0f * fact },
   { 265.0f * fact, 272.0f * fact }
};

static const SFG_StrokeStrip chr_35_strip[] = { {29, chr_35_part_0}, {5, chr_35_part_1} };

SFG_StrokeChar chr_35 = { 797 * fact, 2, chr_35_strip };


// dollar.glif
static const SFG_StrokeVertex chr_36_part_0[] = {
   { 754.0f * fact, 590.0f * fact },
   { 754.0f * fact, 662.0f * fact },
   { 696.0f * fact, 720.0f * fact },
   { 624.0f * fact, 720.0f * fact },
   { 448.0f * fact, 720.0f * fact },
   { 448.0f * fact, 828.0f * fact },
   { 340.0f * fact, 828.0f * fact },
   { 340.0f * fact, 720.0f * fact },
   { 164.0f * fact, 720.0f * fact },
   { 92.0f * fact, 720.0f * fact },
   { 34.0f * fact, 662.0f * fact },
   { 34.0f * fact, 590.0f * fact },
   { 34.0f * fact, 436.0f * fact },
   { 34.0f * fact, 364.0f * fact },
   { 92.0f * fact, 306.0f * fact },
   { 164.0f * fact, 306.0f * fact },
   { 340.0f * fact, 306.0f * fact },
   { 340.0f * fact, 108.0f * fact },
   { 164.0f * fact, 108.0f * fact },
   { 152.0f * fact, 108.0f * fact },
   { 142.0f * fact, 118.0f * fact },
   { 142.0f * fact, 130.0f * fact },
   { 142.0f * fact, 172.0f * fact },
   { 34.0f * fact, 172.0f * fact },
   { 34.0f * fact, 130.0f * fact },
   { 34.0f * fact, 58.0f * fact },
   { 92.0f * fact, 0.0f * fact },
   { 164.0f * fact, 0.0f * fact },
   { 340.0f * fact, 0.0f * fact },
   { 340.0f * fact, -108.0f * fact },
   { 448.0f * fact, -108.0f * fact },
   { 448.0f * fact, 0.0f * fact },
   { 624.0f * fact, 0.0f * fact },
   { 696.0f * fact, 0.0f * fact },
   { 754.0f * fact, 58.0f * fact },
   { 754.0f * fact, 130.0f * fact },
   { 754.0f * fact, 284.0f * fact },
   { 754.0f * fact, 356.0f * fact },
   { 696.0f * fact, 414.0f * fact },
   { 624.0f * fact, 414.0f * fact },
   { 448.0f * fact, 414.0f * fact },
   { 448.0f * fact, 612.0f * fact },
   { 624.0f * fact, 612.0f * fact },
   { 636.0f * fact, 612.0f * fact },
   { 646.0f * fact, 602.0f * fact },
   { 646.0f * fact, 590.0f * fact },
   { 646.0f * fact, 548.0f * fact },
   { 754.0f * fact, 548.0f * fact },
   { 754.0f * fact, 590.0f * fact }
};

// dollar.glif
static const SFG_StrokeVertex chr_36_part_1[] = {
   { 624.0f * fact, 306.0f * fact },
   { 636.0f * fact, 306.0f * fact },
   { 646.0f * fact, 296.0f * fact },
   { 646.0f * fact, 284.0f * fact },
   { 646.0f * fact, 130.0f * fact },
   { 646.0f * fact, 118.0f * fact },
   { 636.0f * fact, 108.0f * fact },
   { 624.0f * fact, 108.0f * fact },
   { 448.0f * fact, 108.0f * fact },
   { 448.0f * fact, 306.0f * fact },
   { 624.0f * fact, 306.0f * fact }
};

// dollar.glif
static const SFG_StrokeVertex chr_36_part_2[] = {
   { 261.0f * fact, 414.0f * fact },
   { 164.0f * fact, 414.0f * fact },
   { 152.0f * fact, 414.0f * fact },
   { 142.0f * fact, 424.0f * fact },
   { 142.0f * fact, 436.0f * fact },
   { 142.0f * fact, 590.0f * fact },
   { 142.0f * fact, 602.0f * fact },
   { 152.0f * fact, 612.0f * fact },
   { 164.0f * fact, 612.0f * fact },
   { 340.0f * fact, 612.0f * fact },
   { 340.0f * fact, 414.0f * fact },
   { 261.0f * fact, 414.0f * fact }
};

static const SFG_StrokeStrip chr_36_strip[] = { {49, chr_36_part_0}, {11, chr_36_part_1}, {12, chr_36_part_2} };

SFG_StrokeChar chr_36 = { 788 * fact, 3, chr_36_strip };


// percent.glif
static const SFG_StrokeVertex chr_37_part_0[] = {
   { 837.0f * fact, 720.0f * fact },
   { 132.0f * fact, 128.0f * fact },
   { 132.0f * fact, 0.0f * fact },
   { 147.0f * fact, 0.0f * fact },
   { 852.0f * fact, 592.0f * fact },
   { 852.0f * fact, 720.0f * fact },
   { 837.0f * fact, 720.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_37_part_1[] = {
   { 178.0f * fact, 720.0f * fact },
   { 106.0f * fact, 720.0f * fact },
   { 48.0f * fact, 662.0f * fact },
   { 48.0f * fact, 590.0f * fact },
   { 48.0f * fact, 510.0f * fact },
   { 48.0f * fact, 438.0f * fact },
   { 106.0f * fact, 380.0f * fact },
   { 178.0f * fact, 380.0f * fact },
   { 268.0f * fact, 380.0f * fact },
   { 340.0f * fact, 380.0f * fact },
   { 398.0f * fact, 438.0f * fact },
   { 398.0f * fact, 510.0f * fact },
   { 398.0f * fact, 590.0f * fact },
   { 398.0f * fact, 662.0f * fact },
   { 340.0f * fact, 720.0f * fact },
   { 268.0f * fact, 720.0f * fact },
   { 178.0f * fact, 720.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_37_part_2[] = {
   { 158.0f * fact, 468.0f * fact },
   { 146.0f * fact, 468.0f * fact },
   { 136.0f * fact, 478.0f * fact },
   { 136.0f * fact, 490.0f * fact },
   { 136.0f * fact, 610.0f * fact },
   { 136.0f * fact, 622.0f * fact },
   { 146.0f * fact, 632.0f * fact },
   { 158.0f * fact, 632.0f * fact },
   { 288.0f * fact, 632.0f * fact },
   { 300.0f * fact, 632.0f * fact },
   { 310.0f * fact, 622.0f * fact },
   { 310.0f * fact, 610.0f * fact },
   { 310.0f * fact, 490.0f * fact },
   { 310.0f * fact, 478.0f * fact },
   { 300.0f * fact, 468.0f * fact },
   { 288.0f * fact, 468.0f * fact },
   { 158.0f * fact, 468.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_37_part_3[] = {
   { 696.0f * fact, 339.0f * fact },
   { 624.0f * fact, 339.0f * fact },
   { 566.0f * fact, 281.0f * fact },
   { 566.0f * fact, 209.0f * fact },
   { 566.0f * fact, 129.0f * fact },
   { 566.0f * fact, 57.0f * fact },
   { 624.0f * fact, -1.0f * fact },
   { 696.0f * fact, -1.0f * fact },
   { 786.0f * fact, -1.0f * fact },
   { 858.0f * fact, -1.0f * fact },
   { 916.0f * fact, 57.0f * fact },
   { 916.0f * fact, 129.0f * fact },
   { 916.0f * fact, 209.0f * fact },
   { 916.0f * fact, 281.0f * fact },
   { 858.0f * fact, 339.0f * fact },
   { 786.0f * fact, 339.0f * fact },
   { 696.0f * fact, 339.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_37_part_4[] = {
   { 676.0f * fact, 87.0f * fact },
   { 664.0f * fact, 87.0f * fact },
   { 654.0f * fact, 97.0f * fact },
   { 654.0f * fact, 109.0f * fact },
   { 654.0f * fact, 229.0f * fact },
   { 654.0f * fact, 241.0f * fact },
   { 664.0f * fact, 251.0f * fact },
   { 676.0f * fact, 251.0f * fact },
   { 806.0f * fact, 251.0f * fact },
   { 818.0f * fact, 251.0f * fact },
   { 828.0f * fact, 241.0f * fact },
   { 828.0f * fact, 229.0f * fact },
   { 828.0f * fact, 109.0f * fact },
   { 828.0f * fact, 97.0f * fact },
   { 818.0f * fact, 87.0f * fact },
   { 806.0f * fact, 87.0f * fact },
   { 676.0f * fact, 87.0f * fact }
};

static const SFG_StrokeStrip chr_37_strip[] = { {7, chr_37_part_0}, {17, chr_37_part_1}, {17, chr_37_part_2}, {17, chr_37_part_3}, {17, chr_37_part_4} };

SFG_StrokeChar chr_37 = { 966 * fact, 5, chr_37_strip };


// ampersand.glif
static const SFG_StrokeVertex chr_38_part_0[] = {
   { 773.0f * fact, 181.0f * fact },
   { 773.0f * fact, 346.0f * fact },
   { 665.0f * fact, 346.0f * fact },
   { 665.0f * fact, 223.0f * fact },
   { 201.0f * fact, 457.0f * fact },
   { 201.0f * fact, 589.0f * fact },
   { 201.0f * fact, 601.0f * fact },
   { 211.0f * fact, 611.0f * fact },
   { 223.0f * fact, 611.0f * fact },
   { 613.0f * fact, 611.0f * fact },
   { 625.0f * fact, 611.0f * fact },
   { 635.0f * fact, 601.0f * fact },
   { 635.0f * fact, 589.0f * fact },
   { 635.0f * fact, 541.0f * fact },
   { 743.0f * fact, 541.0f * fact },
   { 743.0f * fact, 612.0f * fact },
   { 732.0f * fact, 673.0f * fact },
   { 677.0f * fact, 720.0f * fact },
   { 613.0f * fact, 720.0f * fact },
   { 223.0f * fact, 720.0f * fact },
   { 151.0f * fact, 720.0f * fact },
   { 93.0f * fact, 662.0f * fact },
   { 93.0f * fact, 590.0f * fact },
   { 93.0f * fact, 486.0f * fact },
   { 93.0f * fact, 458.0f * fact },
   { 97.0f * fact, 426.0f * fact },
   { 118.0f * fact, 410.0f * fact },
   { 75.0f * fact, 410.0f * fact },
   { 53.0f * fact, 362.0f * fact },
   { 53.0f * fact, 334.0f * fact },
   { 53.0f * fact, 130.0f * fact },
   { 53.0f * fact, 58.0f * fact },
   { 111.0f * fact, 0.0f * fact },
   { 183.0f * fact, 0.0f * fact },
   { 643.0f * fact, 0.0f * fact },
   { 698.0f * fact, 0.0f * fact },
   { 744.0f * fact, 33.0f * fact },
   { 763.0f * fact, 81.0f * fact },
   { 900.0f * fact, 3.0f * fact },
   { 900.0f * fact, 107.0f * fact },
   { 773.0f * fact, 181.0f * fact }
};

// ampersand.glif
static const SFG_StrokeVertex chr_38_part_1[] = {
   { 183.0f * fact, 108.0f * fact },
   { 171.0f * fact, 108.0f * fact },
   { 161.0f * fact, 118.0f * fact },
   { 161.0f * fact, 130.0f * fact },
   { 161.0f * fact, 334.0f * fact },
   { 161.0f * fact, 346.0f * fact },
   { 171.0f * fact, 356.0f * fact },
   { 183.0f * fact, 356.0f * fact },
   { 194.0f * fact, 356.0f * fact },
   { 662.0f * fact, 120.0f * fact },
   { 659.0f * fact, 113.0f * fact },
   { 651.0f * fact, 108.0f * fact },
   { 643.0f * fact, 108.0f * fact },
   { 183.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_38_strip[] = { {41, chr_38_part_0}, {14, chr_38_part_1} };

SFG_StrokeChar chr_38 = { 938 * fact, 2, chr_38_strip };


// quotesingle.glif
static const SFG_StrokeVertex chr_39_part_0[] = {
   { 167.0f * fact, 719.0f * fact },
   { 59.0f * fact, 719.0f * fact },
   { 59.0f * fact, 557.0f * fact },
   { 167.0f * fact, 557.0f * fact },
   { 167.0f * fact, 719.0f * fact }
};

static const SFG_StrokeStrip chr_39_strip[] = { {5, chr_39_part_0} };

SFG_StrokeChar chr_39 = { 224 * fact, 1, chr_39_strip };


// parenleft.glif
static const SFG_StrokeVertex chr_40_part_0[] = {
   { 182.0f * fact, 108.0f * fact },
   { 170.0f * fact, 108.0f * fact },
   { 160.0f * fact, 118.0f * fact },
   { 160.0f * fact, 130.0f * fact },
   { 160.0f * fact, 590.0f * fact },
   { 160.0f * fact, 602.0f * fact },
   { 170.0f * fact, 612.0f * fact },
   { 182.0f * fact, 612.0f * fact },
   { 224.0f * fact, 612.0f * fact },
   { 224.0f * fact, 720.0f * fact },
   { 182.0f * fact, 720.0f * fact },
   { 110.0f * fact, 720.0f * fact },
   { 52.0f * fact, 662.0f * fact },
   { 52.0f * fact, 590.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 58.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 182.0f * fact, 0.0f * fact },
   { 224.0f * fact, 0.0f * fact },
   { 224.0f * fact, 108.0f * fact },
   { 182.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_40_strip[] = { {21, chr_40_part_0} };

SFG_StrokeChar chr_40 = { 277 * fact, 1, chr_40_strip };


// parenright.glif
static const SFG_StrokeVertex chr_41_part_0[] = {
   { 56.0f * fact, 0.0f * fact },
   { 98.0f * fact, 0.0f * fact },
   { 169.0f * fact, 0.0f * fact },
   { 228.0f * fact, 58.0f * fact },
   { 228.0f * fact, 130.0f * fact },
   { 228.0f * fact, 590.0f * fact },
   { 228.0f * fact, 662.0f * fact },
   { 169.0f * fact, 720.0f * fact },
   { 98.0f * fact, 720.0f * fact },
   { 56.0f * fact, 720.0f * fact },
   { 56.0f * fact, 612.0f * fact },
   { 98.0f * fact, 612.0f * fact },
   { 110.0f * fact, 612.0f * fact },
   { 120.0f * fact, 602.0f * fact },
   { 120.0f * fact, 590.0f * fact },
   { 120.0f * fact, 130.0f * fact },
   { 120.0f * fact, 118.0f * fact },
   { 110.0f * fact, 108.0f * fact },
   { 98.0f * fact, 108.0f * fact },
   { 56.0f * fact, 108.0f * fact },
   { 56.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_41_strip[] = { {21, chr_41_part_0} };

SFG_StrokeChar chr_41 = { 278 * fact, 1, chr_41_strip };


// asterisk.glif
static const SFG_StrokeVertex chr_42_part_0[] = {
   { 432.0f * fact, 623.0f * fact },
   { 299.0f * fact, 579.0f * fact },
   { 299.0f * fact, 719.0f * fact },
   { 191.0f * fact, 719.0f * fact },
   { 191.0f * fact, 579.0f * fact },
   { 58.0f * fact, 623.0f * fact },
   { 25.0f * fact, 520.0f * fact },
   { 158.0f * fact, 477.0f * fact },
   { 76.0f * fact, 363.0f * fact },
   { 163.0f * fact, 300.0f * fact },
   { 245.0f * fact, 413.0f * fact },
   { 328.0f * fact, 300.0f * fact },
   { 415.0f * fact, 363.0f * fact },
   { 333.0f * fact, 477.0f * fact },
   { 466.0f * fact, 520.0f * fact },
   { 432.0f * fact, 623.0f * fact }
};

static const SFG_StrokeStrip chr_42_strip[] = { {16, chr_42_part_0} };

SFG_StrokeChar chr_42 = { 491 * fact, 1, chr_42_strip };


// plus.glif
static const SFG_StrokeVertex chr_43_part_0[] = {
   { 161.0f * fact, 495.0f * fact },
   { 161.0f * fact, 350.0f * fact },
   { 17.0f * fact, 350.0f * fact },
   { 17.0f * fact, 242.0f * fact },
   { 161.0f * fact, 242.0f * fact },
   { 161.0f * fact, 95.0f * fact },
   { 269.0f * fact, 95.0f * fact },
   { 269.0f * fact, 242.0f * fact },
   { 417.0f * fact, 242.0f * fact },
   { 417.0f * fact, 350.0f * fact },
   { 269.0f * fact, 350.0f * fact },
   { 269.0f * fact, 495.0f * fact },
   { 161.0f * fact, 495.0f * fact }
};

static const SFG_StrokeStrip chr_43_strip[] = { {13, chr_43_part_0} };

SFG_StrokeChar chr_43 = { 433 * fact, 1, chr_43_strip };


// comma.glif
static const SFG_StrokeVertex chr_44_part_0[] = {
   { 54.0f * fact, 108.0f * fact },
   { 54.0f * fact, -128.0f * fact },
   { 115.0f * fact, -118.0f * fact },
   { 162.0f * fact, -65.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 108.0f * fact },
   { 54.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_44_strip[] = { {7, chr_44_part_0} };

SFG_StrokeChar chr_44 = { 193 * fact, 1, chr_44_strip };


// hyphen.glif
static const SFG_StrokeVertex chr_45_part_0[] = {
   { 459.0f * fact, 350.0f * fact },
   { 59.0f * fact, 350.0f * fact },
   { 59.0f * fact, 242.0f * fact },
   { 459.0f * fact, 242.0f * fact },
   { 459.0f * fact, 350.0f * fact }
};

static const SFG_StrokeStrip chr_45_strip[] = { {5, chr_45_part_0} };

SFG_StrokeChar chr_45 = { 517 * fact, 1, chr_45_strip };


// period.glif
static const SFG_StrokeVertex chr_46_part_0[] = {
   { 162.0f * fact, 108.0f * fact },
   { 54.0f * fact, 108.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_46_strip[] = { {5, chr_46_part_0} };

SFG_StrokeChar chr_46 = { 214 * fact, 1, chr_46_strip };


// slash.glif
static const SFG_StrokeVertex chr_47_part_0[] = {
   { 6.0f * fact, 128.0f * fact },
   { 6.0f * fact, 0.0f * fact },
   { 21.0f * fact, 0.0f * fact },
   { 516.0f * fact, 592.0f * fact },
   { 516.0f * fact, 720.0f * fact },
   { 501.0f * fact, 720.0f * fact },
   { 6.0f * fact, 128.0f * fact }
};

static const SFG_StrokeStrip chr_47_strip[] = { {7, chr_47_part_0} };

SFG_StrokeChar chr_47 = { 521 * fact, 1, chr_47_strip };


// zero.glif
static const SFG_StrokeVertex chr_48_part_0[] = {
   { 187.0f * fact, 720.0f * fact },
   { 115.0f * fact, 720.0f * fact },
   { 57.0f * fact, 662.0f * fact },
   { 57.0f * fact, 590.0f * fact },
   { 57.0f * fact, 130.0f * fact },
   { 57.0f * fact, 58.0f * fact },
   { 115.0f * fact, 0.0f * fact },
   { 187.0f * fact, 0.0f * fact },
   { 647.0f * fact, 0.0f * fact },
   { 719.0f * fact, 0.0f * fact },
   { 777.0f * fact, 58.0f * fact },
   { 777.0f * fact, 130.0f * fact },
   { 777.0f * fact, 590.0f * fact },
   { 777.0f * fact, 662.0f * fact },
   { 719.0f * fact, 720.0f * fact },
   { 647.0f * fact, 720.0f * fact },
   { 187.0f * fact, 720.0f * fact }
};

// zero.glif
static const SFG_StrokeVertex chr_48_part_1[] = {
   { 633.0f * fact, 612.0f * fact },
   { 165.0f * fact, 219.0f * fact },
   { 165.0f * fact, 590.0f * fact },
   { 165.0f * fact, 602.0f * fact },
   { 175.0f * fact, 612.0f * fact },
   { 187.0f * fact, 612.0f * fact },
   { 633.0f * fact, 612.0f * fact }
};

// zero.glif
static const SFG_StrokeVertex chr_48_part_2[] = {
   { 201.0f * fact, 108.0f * fact },
   { 669.0f * fact, 501.0f * fact },
   { 669.0f * fact, 130.0f * fact },
   { 669.0f * fact, 118.0f * fact },
   { 659.0f * fact, 108.0f * fact },
   { 647.0f * fact, 108.0f * fact },
   { 201.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_48_strip[] = { {17, chr_48_part_0}, {7, chr_48_part_1}, {7, chr_48_part_2} };

SFG_StrokeChar chr_48 = { 834 * fact, 3, chr_48_strip };


// one.glif
static const SFG_StrokeVertex chr_49_part_0[] = {
   { 1.0f * fact, 477.0f * fact },
   { 142.0f * fact, 477.0f * fact },
   { 225.0f * fact, 576.0f * fact },
   { 225.0f * fact, 0.0f * fact },
   { 333.0f * fact, 0.0f * fact },
   { 333.0f * fact, 720.0f * fact },
   { 204.0f * fact, 720.0f * fact },
   { 1.0f * fact, 477.0f * fact }
};

static const SFG_StrokeStrip chr_49_strip[] = { {8, chr_49_part_0} };

SFG_StrokeChar chr_49 = { 391 * fact, 1, chr_49_strip };


// two.glif
static const SFG_StrokeVertex chr_50_part_0[] = {
   { 187.0f * fact, 720.0f * fact },
   { 115.0f * fact, 720.0f * fact },
   { 57.0f * fact, 662.0f * fact },
   { 57.0f * fact, 590.0f * fact },
   { 57.0f * fact, 548.0f * fact },
   { 165.0f * fact, 548.0f * fact },
   { 165.0f * fact, 590.0f * fact },
   { 165.0f * fact, 602.0f * fact },
   { 175.0f * fact, 612.0f * fact },
   { 187.0f * fact, 612.0f * fact },
   { 647.0f * fact, 612.0f * fact },
   { 659.0f * fact, 612.0f * fact },
   { 669.0f * fact, 602.0f * fact },
   { 669.0f * fact, 590.0f * fact },
   { 669.0f * fact, 426.0f * fact },
   { 669.0f * fact, 414.0f * fact },
   { 659.0f * fact, 404.0f * fact },
   { 647.0f * fact, 404.0f * fact },
   { 187.0f * fact, 404.0f * fact },
   { 115.0f * fact, 404.0f * fact },
   { 57.0f * fact, 346.0f * fact },
   { 57.0f * fact, 274.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 777.0f * fact, 0.0f * fact },
   { 777.0f * fact, 108.0f * fact },
   { 187.0f * fact, 108.0f * fact },
   { 175.0f * fact, 108.0f * fact },
   { 165.0f * fact, 118.0f * fact },
   { 165.0f * fact, 130.0f * fact },
   { 165.0f * fact, 274.0f * fact },
   { 165.0f * fact, 286.0f * fact },
   { 175.0f * fact, 296.0f * fact },
   { 187.0f * fact, 296.0f * fact },
   { 647.0f * fact, 296.0f * fact },
   { 719.0f * fact, 296.0f * fact },
   { 777.0f * fact, 354.0f * fact },
   { 777.0f * fact, 426.0f * fact },
   { 777.0f * fact, 590.0f * fact },
   { 777.0f * fact, 662.0f * fact },
   { 719.0f * fact, 720.0f * fact },
   { 647.0f * fact, 720.0f * fact },
   { 187.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_50_strip[] = { {42, chr_50_part_0} };

SFG_StrokeChar chr_50 = { 830 * fact, 1, chr_50_strip };


// three.glif
static const SFG_StrokeVertex chr_51_part_0[] = {
   { 738.0f * fact, 404.0f * fact },
   { 743.0f * fact, 423.0f * fact },
   { 743.0f * fact, 443.0f * fact },
   { 743.0f * fact, 590.0f * fact },
   { 743.0f * fact, 662.0f * fact },
   { 685.0f * fact, 720.0f * fact },
   { 613.0f * fact, 720.0f * fact },
   { 183.0f * fact, 720.0f * fact },
   { 111.0f * fact, 720.0f * fact },
   { 53.0f * fact, 662.0f * fact },
   { 53.0f * fact, 590.0f * fact },
   { 53.0f * fact, 550.0f * fact },
   { 161.0f * fact, 550.0f * fact },
   { 161.0f * fact, 590.0f * fact },
   { 161.0f * fact, 602.0f * fact },
   { 171.0f * fact, 612.0f * fact },
   { 183.0f * fact, 612.0f * fact },
   { 613.0f * fact, 612.0f * fact },
   { 625.0f * fact, 612.0f * fact },
   { 635.0f * fact, 602.0f * fact },
   { 635.0f * fact, 590.0f * fact },
   { 635.0f * fact, 443.0f * fact },
   { 635.0f * fact, 431.0f * fact },
   { 625.0f * fact, 421.0f * fact },
   { 613.0f * fact, 421.0f * fact },
   { 183.0f * fact, 421.0f * fact },
   { 183.0f * fact, 313.0f * fact },
   { 643.0f * fact, 313.0f * fact },
   { 655.0f * fact, 313.0f * fact },
   { 665.0f * fact, 303.0f * fact },
   { 665.0f * fact, 291.0f * fact },
   { 665.0f * fact, 130.0f * fact },
   { 665.0f * fact, 118.0f * fact },
   { 655.0f * fact, 108.0f * fact },
   { 643.0f * fact, 108.0f * fact },
   { 183.0f * fact, 108.0f * fact },
   { 171.0f * fact, 108.0f * fact },
   { 161.0f * fact, 118.0f * fact },
   { 161.0f * fact, 130.0f * fact },
   { 161.0f * fact, 160.0f * fact },
   { 53.0f * fact, 160.0f * fact },
   { 53.0f * fact, 130.0f * fact },
   { 53.0f * fact, 58.0f * fact },
   { 111.0f * fact, 0.0f * fact },
   { 183.0f * fact, 0.0f * fact },
   { 643.0f * fact, 0.0f * fact },
   { 715.0f * fact, 0.0f * fact },
   { 773.0f * fact, 58.0f * fact },
   { 773.0f * fact, 130.0f * fact },
   { 773.0f * fact, 291.0f * fact },
   { 773.0f * fact, 329.0f * fact },
   { 756.0f * fact, 364.0f * fact },
   { 730.0f * fact, 387.0f * fact },
   { 738.0f * fact, 404.0f * fact }
};

static const SFG_StrokeStrip chr_51_strip[] = { {54, chr_51_part_0} };

SFG_StrokeChar chr_51 = { 826 * fact, 1, chr_51_strip };


// four.glif
static const SFG_StrokeVertex chr_52_part_0[] = {
   { 590.0f * fact, 291.0f * fact },
   { 590.0f * fact, 720.0f * fact },
   { 482.0f * fact, 720.0f * fact },
   { 6.0f * fact, 291.0f * fact },
   { 6.0f * fact, 183.0f * fact },
   { 482.0f * fact, 183.0f * fact },
   { 482.0f * fact, 0.0f * fact },
   { 590.0f * fact, 0.0f * fact },
   { 590.0f * fact, 183.0f * fact },
   { 696.0f * fact, 183.0f * fact },
   { 696.0f * fact, 291.0f * fact },
   { 590.0f * fact, 291.0f * fact }
};

// four.glif
static const SFG_StrokeVertex chr_52_part_1[] = {
   { 482.0f * fact, 550.0f * fact },
   { 482.0f * fact, 291.0f * fact },
   { 163.0f * fact, 291.0f * fact },
   { 482.0f * fact, 550.0f * fact }
};

static const SFG_StrokeStrip chr_52_strip[] = { {12, chr_52_part_0}, {4, chr_52_part_1} };

SFG_StrokeChar chr_52 = { 730 * fact, 2, chr_52_strip };


// five.glif
static const SFG_StrokeVertex chr_53_part_0[] = {
   { 165.0f * fact, 602.0f * fact },
   { 175.0f * fact, 612.0f * fact },
   { 187.0f * fact, 612.0f * fact },
   { 777.0f * fact, 612.0f * fact },
   { 777.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 446.0f * fact },
   { 57.0f * fact, 438.0f * fact },
   { 58.0f * fact, 431.0f * fact },
   { 59.0f * fact, 424.0f * fact },
   { 57.0f * fact, 424.0f * fact },
   { 57.0f * fact, 316.0f * fact },
   { 647.0f * fact, 316.0f * fact },
   { 659.0f * fact, 316.0f * fact },
   { 669.0f * fact, 306.0f * fact },
   { 669.0f * fact, 294.0f * fact },
   { 669.0f * fact, 130.0f * fact },
   { 669.0f * fact, 118.0f * fact },
   { 659.0f * fact, 108.0f * fact },
   { 647.0f * fact, 108.0f * fact },
   { 187.0f * fact, 108.0f * fact },
   { 175.0f * fact, 108.0f * fact },
   { 165.0f * fact, 118.0f * fact },
   { 165.0f * fact, 130.0f * fact },
   { 165.0f * fact, 172.0f * fact },
   { 57.0f * fact, 172.0f * fact },
   { 57.0f * fact, 130.0f * fact },
   { 57.0f * fact, 58.0f * fact },
   { 115.0f * fact, 0.0f * fact },
   { 187.0f * fact, 0.0f * fact },
   { 647.0f * fact, 0.0f * fact },
   { 719.0f * fact, 0.0f * fact },
   { 777.0f * fact, 58.0f * fact },
   { 777.0f * fact, 130.0f * fact },
   { 777.0f * fact, 294.0f * fact },
   { 777.0f * fact, 366.0f * fact },
   { 719.0f * fact, 424.0f * fact },
   { 647.0f * fact, 424.0f * fact },
   { 187.0f * fact, 424.0f * fact },
   { 175.0f * fact, 424.0f * fact },
   { 165.0f * fact, 434.0f * fact },
   { 165.0f * fact, 446.0f * fact },
   { 165.0f * fact, 590.0f * fact },
   { 165.0f * fact, 602.0f * fact }
};

static const SFG_StrokeStrip chr_53_strip[] = { {44, chr_53_part_0} };

SFG_StrokeChar chr_53 = { 830 * fact, 1, chr_53_strip };


// six.glif
static const SFG_StrokeVertex chr_54_part_0[] = {
   { 527.0f * fact, 424.0f * fact },
   { 187.0f * fact, 424.0f * fact },
   { 175.0f * fact, 424.0f * fact },
   { 165.0f * fact, 434.0f * fact },
   { 165.0f * fact, 446.0f * fact },
   { 165.0f * fact, 590.0f * fact },
   { 165.0f * fact, 602.0f * fact },
   { 175.0f * fact, 612.0f * fact },
   { 187.0f * fact, 612.0f * fact },
   { 657.0f * fact, 612.0f * fact },
   { 657.0f * fact, 720.0f * fact },
   { 654.0f * fact, 720.0f * fact },
   { 650.0f * fact, 720.0f * fact },
   { 187.0f * fact, 720.0f * fact },
   { 115.0f * fact, 720.0f * fact },
   { 57.0f * fact, 662.0f * fact },
   { 57.0f * fact, 590.0f * fact },
   { 57.0f * fact, 130.0f * fact },
   { 57.0f * fact, 58.0f * fact },
   { 115.0f * fact, 0.0f * fact },
   { 187.0f * fact, 0.0f * fact },
   { 647.0f * fact, 0.0f * fact },
   { 719.0f * fact, 0.0f * fact },
   { 777.0f * fact, 58.0f * fact },
   { 777.0f * fact, 130.0f * fact },
   { 777.0f * fact, 294.0f * fact },
   { 777.0f * fact, 366.0f * fact },
   { 719.0f * fact, 424.0f * fact },
   { 647.0f * fact, 424.0f * fact },
   { 527.0f * fact, 424.0f * fact }
};

// six.glif
static const SFG_StrokeVertex chr_54_part_1[] = {
   { 669.0f * fact, 130.0f * fact },
   { 669.0f * fact, 118.0f * fact },
   { 659.0f * fact, 108.0f * fact },
   { 647.0f * fact, 108.0f * fact },
   { 187.0f * fact, 108.0f * fact },
   { 175.0f * fact, 108.0f * fact },
   { 165.0f * fact, 118.0f * fact },
   { 165.0f * fact, 130.0f * fact },
   { 165.0f * fact, 316.0f * fact },
   { 647.0f * fact, 316.0f * fact },
   { 659.0f * fact, 316.0f * fact },
   { 669.0f * fact, 306.0f * fact },
   { 669.0f * fact, 294.0f * fact },
   { 669.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_54_strip[] = { {30, chr_54_part_0}, {14, chr_54_part_1} };

SFG_StrokeChar chr_54 = { 820 * fact, 2, chr_54_strip };


// seven.glif
static const SFG_StrokeVertex chr_55_part_0[] = {
   { 10.0f * fact, 720.0f * fact },
   { 6.0f * fact, 720.0f * fact },
   { 3.0f * fact, 720.0f * fact },
   { 3.0f * fact, 612.0f * fact },
   { 473.0f * fact, 612.0f * fact },
   { 485.0f * fact, 612.0f * fact },
   { 495.0f * fact, 602.0f * fact },
   { 495.0f * fact, 590.0f * fact },
   { 495.0f * fact, 0.0f * fact },
   { 603.0f * fact, 0.0f * fact },
   { 603.0f * fact, 590.0f * fact },
   { 603.0f * fact, 662.0f * fact },
   { 545.0f * fact, 720.0f * fact },
   { 473.0f * fact, 720.0f * fact },
   { 13.0f * fact, 720.0f * fact },
   { 10.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_55_strip[] = { {16, chr_55_part_0} };

SFG_StrokeChar chr_55 = { 660 * fact, 1, chr_55_strip };


// eight.glif
static const SFG_StrokeVertex chr_56_part_0[] = {
   { 777.0f * fact, 612.0f * fact },
   { 766.0f * fact, 673.0f * fact },
   { 711.0f * fact, 720.0f * fact },
   { 647.0f * fact, 720.0f * fact },
   { 187.0f * fact, 720.0f * fact },
   { 115.0f * fact, 720.0f * fact },
   { 57.0f * fact, 662.0f * fact },
   { 57.0f * fact, 590.0f * fact },
   { 57.0f * fact, 446.0f * fact },
   { 57.0f * fact, 418.0f * fact },
   { 66.0f * fact, 391.0f * fact },
   { 82.0f * fact, 370.0f * fact },
   { 66.0f * fact, 349.0f * fact },
   { 57.0f * fact, 322.0f * fact },
   { 57.0f * fact, 294.0f * fact },
   { 57.0f * fact, 130.0f * fact },
   { 57.0f * fact, 58.0f * fact },
   { 115.0f * fact, 0.0f * fact },
   { 187.0f * fact, 0.0f * fact },
   { 647.0f * fact, 0.0f * fact },
   { 719.0f * fact, 0.0f * fact },
   { 777.0f * fact, 58.0f * fact },
   { 777.0f * fact, 130.0f * fact },
   { 777.0f * fact, 294.0f * fact },
   { 777.0f * fact, 322.0f * fact },
   { 768.0f * fact, 349.0f * fact },
   { 752.0f * fact, 370.0f * fact },
   { 768.0f * fact, 391.0f * fact },
   { 777.0f * fact, 418.0f * fact },
   { 777.0f * fact, 446.0f * fact },
   { 777.0f * fact, 612.0f * fact }
};

// eight.glif
static const SFG_StrokeVertex chr_56_part_1[] = {
   { 669.0f * fact, 130.0f * fact },
   { 669.0f * fact, 118.0f * fact },
   { 659.0f * fact, 108.0f * fact },
   { 647.0f * fact, 108.0f * fact },
   { 187.0f * fact, 108.0f * fact },
   { 175.0f * fact, 108.0f * fact },
   { 165.0f * fact, 118.0f * fact },
   { 165.0f * fact, 130.0f * fact },
   { 165.0f * fact, 294.0f * fact },
   { 165.0f * fact, 306.0f * fact },
   { 175.0f * fact, 316.0f * fact },
   { 187.0f * fact, 316.0f * fact },
   { 647.0f * fact, 316.0f * fact },
   { 659.0f * fact, 316.0f * fact },
   { 669.0f * fact, 306.0f * fact },
   { 669.0f * fact, 294.0f * fact },
   { 669.0f * fact, 130.0f * fact }
};

// eight.glif
static const SFG_StrokeVertex chr_56_part_2[] = {
   { 669.0f * fact, 435.0f * fact },
   { 669.0f * fact, 423.0f * fact },
   { 659.0f * fact, 413.0f * fact },
   { 647.0f * fact, 413.0f * fact },
   { 187.0f * fact, 413.0f * fact },
   { 175.0f * fact, 413.0f * fact },
   { 165.0f * fact, 423.0f * fact },
   { 165.0f * fact, 435.0f * fact },
   { 165.0f * fact, 589.0f * fact },
   { 165.0f * fact, 601.0f * fact },
   { 175.0f * fact, 611.0f * fact },
   { 187.0f * fact, 611.0f * fact },
   { 647.0f * fact, 611.0f * fact },
   { 659.0f * fact, 611.0f * fact },
   { 669.0f * fact, 601.0f * fact },
   { 669.0f * fact, 589.0f * fact },
   { 669.0f * fact, 435.0f * fact }
};

static const SFG_StrokeStrip chr_56_strip[] = { {31, chr_56_part_0}, {17, chr_56_part_1}, {17, chr_56_part_2} };

SFG_StrokeChar chr_56 = { 834 * fact, 3, chr_56_strip };


// nine.glif
static const SFG_StrokeVertex chr_57_part_0[] = {
   { 301.0f * fact, 296.0f * fact },
   { 641.0f * fact, 296.0f * fact },
   { 653.0f * fact, 296.0f * fact },
   { 663.0f * fact, 286.0f * fact },
   { 663.0f * fact, 274.0f * fact },
   { 663.0f * fact, 130.0f * fact },
   { 663.0f * fact, 118.0f * fact },
   { 653.0f * fact, 108.0f * fact },
   { 641.0f * fact, 108.0f * fact },
   { 53.0f * fact, 108.0f * fact },
   { 63.0f * fact, 47.0f * fact },
   { 117.0f * fact, 0.0f * fact },
   { 181.0f * fact, 0.0f * fact },
   { 641.0f * fact, 0.0f * fact },
   { 713.0f * fact, 0.0f * fact },
   { 771.0f * fact, 58.0f * fact },
   { 771.0f * fact, 130.0f * fact },
   { 771.0f * fact, 590.0f * fact },
   { 771.0f * fact, 662.0f * fact },
   { 713.0f * fact, 720.0f * fact },
   { 641.0f * fact, 720.0f * fact },
   { 181.0f * fact, 720.0f * fact },
   { 109.0f * fact, 720.0f * fact },
   { 51.0f * fact, 662.0f * fact },
   { 51.0f * fact, 590.0f * fact },
   { 51.0f * fact, 426.0f * fact },
   { 51.0f * fact, 354.0f * fact },
   { 109.0f * fact, 296.0f * fact },
   { 181.0f * fact, 296.0f * fact },
   { 301.0f * fact, 296.0f * fact }
};

// nine.glif
static const SFG_StrokeVertex chr_57_part_1[] = {
   { 159.0f * fact, 590.0f * fact },
   { 159.0f * fact, 602.0f * fact },
   { 169.0f * fact, 612.0f * fact },
   { 181.0f * fact, 612.0f * fact },
   { 641.0f * fact, 612.0f * fact },
   { 653.0f * fact, 612.0f * fact },
   { 663.0f * fact, 602.0f * fact },
   { 663.0f * fact, 590.0f * fact },
   { 663.0f * fact, 404.0f * fact },
   { 181.0f * fact, 404.0f * fact },
   { 169.0f * fact, 404.0f * fact },
   { 159.0f * fact, 414.0f * fact },
   { 159.0f * fact, 426.0f * fact },
   { 159.0f * fact, 590.0f * fact }
};

static const SFG_StrokeStrip chr_57_strip[] = { {30, chr_57_part_0}, {14, chr_57_part_1} };

SFG_StrokeChar chr_57 = { 828 * fact, 2, chr_57_strip };


// colon.glif
static const SFG_StrokeVertex chr_58_part_0[] = {
   { 54.0f * fact, 108.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 108.0f * fact },
   { 54.0f * fact, 108.0f * fact }
};

// colon.glif
static const SFG_StrokeVertex chr_58_part_1[] = {
   { 162.0f * fact, 588.0f * fact },
   { 54.0f * fact, 588.0f * fact },
   { 54.0f * fact, 480.0f * fact },
   { 162.0f * fact, 480.0f * fact },
   { 162.0f * fact, 588.0f * fact }
};

static const SFG_StrokeStrip chr_58_strip[] = { {5, chr_58_part_0}, {5, chr_58_part_1} };

SFG_StrokeChar chr_58 = { 214 * fact, 2, chr_58_strip };


// semicolon.glif
static const SFG_StrokeVertex chr_59_part_0[] = {
   { 159.0f * fact, 588.0f * fact },
   { 51.0f * fact, 588.0f * fact },
   { 51.0f * fact, 480.0f * fact },
   { 159.0f * fact, 480.0f * fact },
   { 159.0f * fact, 588.0f * fact }
};

// semicolon.glif
static const SFG_StrokeVertex chr_59_part_1[] = {
   { 51.0f * fact, 108.0f * fact },
   { 51.0f * fact, -128.0f * fact },
   { 112.0f * fact, -118.0f * fact },
   { 159.0f * fact, -65.0f * fact },
   { 159.0f * fact, 0.0f * fact },
   { 159.0f * fact, 108.0f * fact },
   { 51.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_59_strip[] = { {5, chr_59_part_0}, {7, chr_59_part_1} };

SFG_StrokeChar chr_59 = { 193 * fact, 2, chr_59_strip };


// less.glif
static const SFG_StrokeVertex chr_60_part_0[] = {
   { 127.0f * fact, 296.0f * fact },
   { 416.0f * fact, 462.0f * fact },
   { 416.0f * fact, 587.0f * fact },
   { 5.0f * fact, 350.0f * fact },
   { 5.0f * fact, 242.0f * fact },
   { 416.0f * fact, 4.0f * fact },
   { 416.0f * fact, 129.0f * fact },
   { 127.0f * fact, 296.0f * fact }
};

static const SFG_StrokeStrip chr_60_strip[] = { {8, chr_60_part_0} };

SFG_StrokeChar chr_60 = { 473 * fact, 1, chr_60_strip };


// equal.glif
static const SFG_StrokeVertex chr_61_part_0[] = {
   { 579.0f * fact, 251.0f * fact },
   { 59.0f * fact, 251.0f * fact },
   { 59.0f * fact, 143.0f * fact },
   { 579.0f * fact, 143.0f * fact },
   { 579.0f * fact, 251.0f * fact }
};

// equal.glif
static const SFG_StrokeVertex chr_61_part_1[] = {
   { 579.0f * fact, 441.0f * fact },
   { 59.0f * fact, 441.0f * fact },
   { 59.0f * fact, 333.0f * fact },
   { 579.0f * fact, 333.0f * fact },
   { 579.0f * fact, 441.0f * fact }
};

static const SFG_StrokeStrip chr_61_strip[] = { {5, chr_61_part_0}, {5, chr_61_part_1} };

SFG_StrokeChar chr_61 = { 638 * fact, 2, chr_61_strip };


// greater.glif
static const SFG_StrokeVertex chr_62_part_0[] = {
   { 59.0f * fact, 2.0f * fact },
   { 470.0f * fact, 240.0f * fact },
   { 470.0f * fact, 348.0f * fact },
   { 59.0f * fact, 585.0f * fact },
   { 59.0f * fact, 460.0f * fact },
   { 348.0f * fact, 294.0f * fact },
   { 59.0f * fact, 127.0f * fact },
   { 59.0f * fact, 2.0f * fact }
};

static const SFG_StrokeStrip chr_62_strip[] = { {8, chr_62_part_0} };

SFG_StrokeChar chr_62 = { 475 * fact, 1, chr_62_strip };


// question.glif
static const SFG_StrokeVertex chr_63_part_0[] = {
   { 31.0f * fact, 720.0f * fact },
   { 31.0f * fact, 720.0f * fact },
   { 31.0f * fact, 673.0f * fact },
   { 31.0f * fact, 612.0f * fact },
   { 528.0f * fact, 612.0f * fact },
   { 540.0f * fact, 612.0f * fact },
   { 550.0f * fact, 602.0f * fact },
   { 550.0f * fact, 590.0f * fact },
   { 550.0f * fact, 406.0f * fact },
   { 550.0f * fact, 394.0f * fact },
   { 540.0f * fact, 384.0f * fact },
   { 528.0f * fact, 384.0f * fact },
   { 269.0f * fact, 384.0f * fact },
   { 197.0f * fact, 384.0f * fact },
   { 139.0f * fact, 326.0f * fact },
   { 139.0f * fact, 254.0f * fact },
   { 139.0f * fact, 199.0f * fact },
   { 247.0f * fact, 199.0f * fact },
   { 247.0f * fact, 254.0f * fact },
   { 247.0f * fact, 266.0f * fact },
   { 257.0f * fact, 276.0f * fact },
   { 269.0f * fact, 276.0f * fact },
   { 528.0f * fact, 276.0f * fact },
   { 600.0f * fact, 276.0f * fact },
   { 658.0f * fact, 334.0f * fact },
   { 658.0f * fact, 406.0f * fact },
   { 658.0f * fact, 590.0f * fact },
   { 658.0f * fact, 662.0f * fact },
   { 600.0f * fact, 720.0f * fact },
   { 528.0f * fact, 720.0f * fact },
   { 31.0f * fact, 720.0f * fact }
};

// question.glif
static const SFG_StrokeVertex chr_63_part_1[] = {
   { 247.0f * fact, 0.0f * fact },
   { 247.0f * fact, 108.0f * fact },
   { 139.0f * fact, 108.0f * fact },
   { 139.0f * fact, 0.0f * fact },
   { 247.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_63_strip[] = { {31, chr_63_part_0}, {5, chr_63_part_1} };

SFG_StrokeChar chr_63 = { 678 * fact, 2, chr_63_strip };


// at.glif
static const SFG_StrokeVertex chr_64_part_0[] = {
   { 372.0f * fact, 530.0f * fact },
   { 300.0f * fact, 530.0f * fact },
   { 242.0f * fact, 472.0f * fact },
   { 242.0f * fact, 400.0f * fact },
   { 242.0f * fact, 320.0f * fact },
   { 242.0f * fact, 248.0f * fact },
   { 300.0f * fact, 190.0f * fact },
   { 372.0f * fact, 190.0f * fact },
   { 777.0f * fact, 190.0f * fact },
   { 777.0f * fact, 590.0f * fact },
   { 777.0f * fact, 662.0f * fact },
   { 719.0f * fact, 720.0f * fact },
   { 647.0f * fact, 720.0f * fact },
   { 187.0f * fact, 720.0f * fact },
   { 115.0f * fact, 720.0f * fact },
   { 57.0f * fact, 662.0f * fact },
   { 57.0f * fact, 590.0f * fact },
   { 57.0f * fact, 130.0f * fact },
   { 57.0f * fact, 58.0f * fact },
   { 115.0f * fact, 0.0f * fact },
   { 187.0f * fact, 0.0f * fact },
   { 777.0f * fact, 0.0f * fact },
   { 777.0f * fact, 53.0f * fact },
   { 777.0f * fact, 59.0f * fact },
   { 777.0f * fact, 108.0f * fact },
   { 187.0f * fact, 108.0f * fact },
   { 175.0f * fact, 108.0f * fact },
   { 165.0f * fact, 118.0f * fact },
   { 165.0f * fact, 130.0f * fact },
   { 165.0f * fact, 590.0f * fact },
   { 165.0f * fact, 602.0f * fact },
   { 175.0f * fact, 612.0f * fact },
   { 187.0f * fact, 612.0f * fact },
   { 647.0f * fact, 612.0f * fact },
   { 659.0f * fact, 612.0f * fact },
   { 669.0f * fact, 602.0f * fact },
   { 669.0f * fact, 590.0f * fact },
   { 669.0f * fact, 278.0f * fact },
   { 592.0f * fact, 278.0f * fact },
   { 592.0f * fact, 400.0f * fact },
   { 592.0f * fact, 472.0f * fact },
   { 534.0f * fact, 530.0f * fact },
   { 462.0f * fact, 530.0f * fact },
   { 372.0f * fact, 530.0f * fact }
};

// at.glif
static const SFG_StrokeVertex chr_64_part_1[] = {
   { 352.0f * fact, 278.0f * fact },
   { 340.0f * fact, 278.0f * fact },
   { 330.0f * fact, 288.0f * fact },
   { 330.0f * fact, 300.0f * fact },
   { 330.0f * fact, 420.0f * fact },
   { 330.0f * fact, 432.0f * fact },
   { 340.0f * fact, 442.0f * fact },
   { 352.0f * fact, 442.0f * fact },
   { 482.0f * fact, 442.0f * fact },
   { 494.0f * fact, 442.0f * fact },
   { 504.0f * fact, 432.0f * fact },
   { 504.0f * fact, 420.0f * fact },
   { 504.0f * fact, 278.0f * fact },
   { 352.0f * fact, 278.0f * fact }
};

static const SFG_StrokeStrip chr_64_strip[] = { {44, chr_64_part_0}, {14, chr_64_part_1} };

SFG_StrokeChar chr_64 = { 831 * fact, 2, chr_64_strip };


// A_.glif
static const SFG_StrokeVertex chr_65_part_0[] = {
   { 188.0f * fact, 720.0f * fact },
   { 116.0f * fact, 720.0f * fact },
   { 58.0f * fact, 662.0f * fact },
   { 58.0f * fact, 590.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 166.0f * fact, 0.0f * fact },
   { 166.0f * fact, 252.0f * fact },
   { 670.0f * fact, 252.0f * fact },
   { 670.0f * fact, 0.0f * fact },
   { 778.0f * fact, 0.0f * fact },
   { 778.0f * fact, 590.0f * fact },
   { 778.0f * fact, 662.0f * fact },
   { 720.0f * fact, 720.0f * fact },
   { 648.0f * fact, 720.0f * fact },
   { 188.0f * fact, 720.0f * fact }
};

// A_.glif
static const SFG_StrokeVertex chr_65_part_1[] = {
   { 166.0f * fact, 360.0f * fact },
   { 166.0f * fact, 590.0f * fact },
   { 166.0f * fact, 602.0f * fact },
   { 176.0f * fact, 612.0f * fact },
   { 188.0f * fact, 612.0f * fact },
   { 648.0f * fact, 612.0f * fact },
   { 660.0f * fact, 612.0f * fact },
   { 670.0f * fact, 602.0f * fact },
   { 670.0f * fact, 590.0f * fact },
   { 670.0f * fact, 360.0f * fact },
   { 166.0f * fact, 360.0f * fact }
};

static const SFG_StrokeStrip chr_65_strip[] = { {15, chr_65_part_0}, {11, chr_65_part_1} };

SFG_StrokeChar chr_65 = { 836 * fact, 2, chr_65_strip };


// B_.glif
static const SFG_StrokeVertex chr_66_part_0[] = {
   { 744.0f * fact, 404.0f * fact },
   { 749.0f * fact, 423.0f * fact },
   { 749.0f * fact, 443.0f * fact },
   { 749.0f * fact, 590.0f * fact },
   { 749.0f * fact, 662.0f * fact },
   { 691.0f * fact, 720.0f * fact },
   { 619.0f * fact, 720.0f * fact },
   { 59.0f * fact, 720.0f * fact },
   { 59.0f * fact, 0.0f * fact },
   { 649.0f * fact, 0.0f * fact },
   { 721.0f * fact, 0.0f * fact },
   { 779.0f * fact, 58.0f * fact },
   { 779.0f * fact, 130.0f * fact },
   { 779.0f * fact, 291.0f * fact },
   { 779.0f * fact, 329.0f * fact },
   { 762.0f * fact, 364.0f * fact },
   { 736.0f * fact, 387.0f * fact },
   { 744.0f * fact, 404.0f * fact }
};

// B_.glif
static const SFG_StrokeVertex chr_66_part_1[] = {
   { 619.0f * fact, 612.0f * fact },
   { 631.0f * fact, 612.0f * fact },
   { 641.0f * fact, 602.0f * fact },
   { 641.0f * fact, 590.0f * fact },
   { 641.0f * fact, 443.0f * fact },
   { 641.0f * fact, 431.0f * fact },
   { 631.0f * fact, 421.0f * fact },
   { 619.0f * fact, 421.0f * fact },
   { 189.0f * fact, 421.0f * fact },
   { 177.0f * fact, 421.0f * fact },
   { 167.0f * fact, 431.0f * fact },
   { 167.0f * fact, 443.0f * fact },
   { 167.0f * fact, 590.0f * fact },
   { 167.0f * fact, 602.0f * fact },
   { 177.0f * fact, 612.0f * fact },
   { 189.0f * fact, 612.0f * fact },
   { 619.0f * fact, 612.0f * fact }
};

// B_.glif
static const SFG_StrokeVertex chr_66_part_2[] = {
   { 671.0f * fact, 130.0f * fact },
   { 671.0f * fact, 118.0f * fact },
   { 661.0f * fact, 108.0f * fact },
   { 649.0f * fact, 108.0f * fact },
   { 189.0f * fact, 108.0f * fact },
   { 177.0f * fact, 108.0f * fact },
   { 167.0f * fact, 118.0f * fact },
   { 167.0f * fact, 130.0f * fact },
   { 167.0f * fact, 291.0f * fact },
   { 167.0f * fact, 303.0f * fact },
   { 177.0f * fact, 313.0f * fact },
   { 189.0f * fact, 313.0f * fact },
   { 649.0f * fact, 313.0f * fact },
   { 661.0f * fact, 313.0f * fact },
   { 671.0f * fact, 303.0f * fact },
   { 671.0f * fact, 291.0f * fact },
   { 671.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_66_strip[] = { {18, chr_66_part_0}, {17, chr_66_part_1}, {17, chr_66_part_2} };

SFG_StrokeChar chr_66 = { 832 * fact, 3, chr_66_strip };


// C_.glif
static const SFG_StrokeVertex chr_67_part_0[] = {
   { 774.0f * fact, 612.0f * fact },
   { 774.0f * fact, 720.0f * fact },
   { 186.0f * fact, 720.0f * fact },
   { 114.0f * fact, 720.0f * fact },
   { 56.0f * fact, 662.0f * fact },
   { 56.0f * fact, 590.0f * fact },
   { 56.0f * fact, 130.0f * fact },
   { 56.0f * fact, 58.0f * fact },
   { 114.0f * fact, 0.0f * fact },
   { 186.0f * fact, 0.0f * fact },
   { 774.0f * fact, 0.0f * fact },
   { 774.0f * fact, 108.0f * fact },
   { 186.0f * fact, 108.0f * fact },
   { 174.0f * fact, 108.0f * fact },
   { 164.0f * fact, 118.0f * fact },
   { 164.0f * fact, 130.0f * fact },
   { 164.0f * fact, 590.0f * fact },
   { 164.0f * fact, 602.0f * fact },
   { 174.0f * fact, 612.0f * fact },
   { 186.0f * fact, 612.0f * fact },
   { 774.0f * fact, 612.0f * fact }
};

static const SFG_StrokeStrip chr_67_strip[] = { {21, chr_67_part_0} };

SFG_StrokeChar chr_67 = { 822 * fact, 1, chr_67_strip };


// D_.glif
static const SFG_StrokeVertex chr_68_part_0[] = {
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 648.0f * fact, 0.0f * fact },
   { 720.0f * fact, 0.0f * fact },
   { 778.0f * fact, 58.0f * fact },
   { 778.0f * fact, 130.0f * fact },
   { 778.0f * fact, 590.0f * fact },
   { 778.0f * fact, 662.0f * fact },
   { 720.0f * fact, 720.0f * fact },
   { 648.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact }
};

// D_.glif
static const SFG_StrokeVertex chr_68_part_1[] = {
   { 670.0f * fact, 130.0f * fact },
   { 670.0f * fact, 118.0f * fact },
   { 660.0f * fact, 108.0f * fact },
   { 648.0f * fact, 108.0f * fact },
   { 188.0f * fact, 108.0f * fact },
   { 176.0f * fact, 108.0f * fact },
   { 166.0f * fact, 118.0f * fact },
   { 166.0f * fact, 130.0f * fact },
   { 166.0f * fact, 590.0f * fact },
   { 166.0f * fact, 602.0f * fact },
   { 176.0f * fact, 612.0f * fact },
   { 188.0f * fact, 612.0f * fact },
   { 648.0f * fact, 612.0f * fact },
   { 660.0f * fact, 612.0f * fact },
   { 670.0f * fact, 602.0f * fact },
   { 670.0f * fact, 590.0f * fact },
   { 670.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_68_strip[] = { {11, chr_68_part_0}, {17, chr_68_part_1} };

SFG_StrokeChar chr_68 = { 834 * fact, 2, chr_68_strip };


// E_.glif
static const SFG_StrokeVertex chr_69_part_0[] = {
   { 718.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 718.0f * fact, 0.0f * fact },
   { 718.0f * fact, 108.0f * fact },
   { 166.0f * fact, 108.0f * fact },
   { 166.0f * fact, 306.0f * fact },
   { 610.0f * fact, 306.0f * fact },
   { 610.0f * fact, 414.0f * fact },
   { 166.0f * fact, 414.0f * fact },
   { 166.0f * fact, 612.0f * fact },
   { 718.0f * fact, 612.0f * fact },
   { 718.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_69_strip[] = { {13, chr_69_part_0} };

SFG_StrokeChar chr_69 = { 766 * fact, 1, chr_69_strip };


// F_.glif
static const SFG_StrokeVertex chr_70_part_0[] = {
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 166.0f * fact, 0.0f * fact },
   { 166.0f * fact, 306.0f * fact },
   { 610.0f * fact, 306.0f * fact },
   { 610.0f * fact, 414.0f * fact },
   { 166.0f * fact, 414.0f * fact },
   { 166.0f * fact, 612.0f * fact },
   { 718.0f * fact, 612.0f * fact },
   { 718.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_70_strip[] = { {11, chr_70_part_0} };

SFG_StrokeChar chr_70 = { 723 * fact, 1, chr_70_strip };


// G_.glif
static const SFG_StrokeVertex chr_71_part_0[] = {
   { 776.0f * fact, 590.0f * fact },
   { 776.0f * fact, 662.0f * fact },
   { 718.0f * fact, 720.0f * fact },
   { 646.0f * fact, 720.0f * fact },
   { 186.0f * fact, 720.0f * fact },
   { 114.0f * fact, 720.0f * fact },
   { 56.0f * fact, 662.0f * fact },
   { 56.0f * fact, 590.0f * fact },
   { 56.0f * fact, 130.0f * fact },
   { 56.0f * fact, 58.0f * fact },
   { 114.0f * fact, 0.0f * fact },
   { 186.0f * fact, 0.0f * fact },
   { 646.0f * fact, 0.0f * fact },
   { 718.0f * fact, 0.0f * fact },
   { 776.0f * fact, 58.0f * fact },
   { 776.0f * fact, 130.0f * fact },
   { 776.0f * fact, 394.0f * fact },
   { 498.0f * fact, 394.0f * fact },
   { 498.0f * fact, 286.0f * fact },
   { 668.0f * fact, 286.0f * fact },
   { 668.0f * fact, 130.0f * fact },
   { 668.0f * fact, 118.0f * fact },
   { 658.0f * fact, 108.0f * fact },
   { 646.0f * fact, 108.0f * fact },
   { 186.0f * fact, 108.0f * fact },
   { 174.0f * fact, 108.0f * fact },
   { 164.0f * fact, 118.0f * fact },
   { 164.0f * fact, 130.0f * fact },
   { 164.0f * fact, 590.0f * fact },
   { 164.0f * fact, 602.0f * fact },
   { 174.0f * fact, 612.0f * fact },
   { 186.0f * fact, 612.0f * fact },
   { 646.0f * fact, 612.0f * fact },
   { 658.0f * fact, 612.0f * fact },
   { 668.0f * fact, 602.0f * fact },
   { 668.0f * fact, 590.0f * fact },
   { 668.0f * fact, 547.0f * fact },
   { 776.0f * fact, 547.0f * fact },
   { 776.0f * fact, 590.0f * fact }
};

static const SFG_StrokeStrip chr_71_strip[] = { {39, chr_71_part_0} };

SFG_StrokeChar chr_71 = { 830 * fact, 1, chr_71_strip };


// H_.glif
static const SFG_StrokeVertex chr_72_part_0[] = {
   { 686.0f * fact, 720.0f * fact },
   { 686.0f * fact, 414.0f * fact },
   { 165.0f * fact, 414.0f * fact },
   { 165.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 165.0f * fact, 0.0f * fact },
   { 165.0f * fact, 306.0f * fact },
   { 686.0f * fact, 306.0f * fact },
   { 686.0f * fact, 0.0f * fact },
   { 794.0f * fact, 0.0f * fact },
   { 794.0f * fact, 720.0f * fact },
   { 686.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_72_strip[] = { {13, chr_72_part_0} };

SFG_StrokeChar chr_72 = { 851 * fact, 1, chr_72_strip };


// I_.glif
static const SFG_StrokeVertex chr_73_part_0[] = {
   { 57.0f * fact, 0.0f * fact },
   { 165.0f * fact, 0.0f * fact },
   { 165.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_73_strip[] = { {5, chr_73_part_0} };

SFG_StrokeChar chr_73 = { 220 * fact, 1, chr_73_strip };


// J_.glif
static const SFG_StrokeVertex chr_74_part_0[] = {
   { 616.0f * fact, 130.0f * fact },
   { 616.0f * fact, 118.0f * fact },
   { 606.0f * fact, 108.0f * fact },
   { 594.0f * fact, 108.0f * fact },
   { 134.0f * fact, 108.0f * fact },
   { 122.0f * fact, 108.0f * fact },
   { 112.0f * fact, 118.0f * fact },
   { 112.0f * fact, 130.0f * fact },
   { 112.0f * fact, 200.0f * fact },
   { 4.0f * fact, 200.0f * fact },
   { 4.0f * fact, 130.0f * fact },
   { 4.0f * fact, 58.0f * fact },
   { 62.0f * fact, 0.0f * fact },
   { 134.0f * fact, 0.0f * fact },
   { 594.0f * fact, 0.0f * fact },
   { 666.0f * fact, 0.0f * fact },
   { 724.0f * fact, 58.0f * fact },
   { 724.0f * fact, 130.0f * fact },
   { 724.0f * fact, 720.0f * fact },
   { 616.0f * fact, 720.0f * fact },
   { 616.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_74_strip[] = { {21, chr_74_part_0} };

SFG_StrokeChar chr_74 = { 780 * fact, 1, chr_74_strip };


// K_.glif
static const SFG_StrokeVertex chr_75_part_0[] = {
   { 622.0f * fact, 720.0f * fact },
   { 365.0f * fact, 414.0f * fact },
   { 165.0f * fact, 414.0f * fact },
   { 165.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 165.0f * fact, 0.0f * fact },
   { 165.0f * fact, 306.0f * fact },
   { 365.0f * fact, 306.0f * fact },
   { 622.0f * fact, 0.0f * fact },
   { 750.0f * fact, 0.0f * fact },
   { 750.0f * fact, 15.0f * fact },
   { 461.0f * fact, 360.0f * fact },
   { 750.0f * fact, 705.0f * fact },
   { 750.0f * fact, 720.0f * fact },
   { 622.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_75_strip[] = { {16, chr_75_part_0} };

SFG_StrokeChar chr_75 = { 797 * fact, 1, chr_75_strip };


// L_.glif
static const SFG_StrokeVertex chr_76_part_0[] = {
   { 57.0f * fact, 0.0f * fact },
   { 777.0f * fact, 0.0f * fact },
   { 777.0f * fact, 108.0f * fact },
   { 165.0f * fact, 108.0f * fact },
   { 165.0f * fact, 721.0f * fact },
   { 57.0f * fact, 721.0f * fact },
   { 57.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_76_strip[] = { {7, chr_76_part_0} };

SFG_StrokeChar chr_76 = { 779 * fact, 1, chr_76_strip };


// M_.glif
static const SFG_StrokeVertex chr_77_part_0[] = {
   { 464.0f * fact, 387.0f * fact },
   { 184.0f * fact, 720.0f * fact },
   { 56.0f * fact, 720.0f * fact },
   { 56.0f * fact, 0.0f * fact },
   { 164.0f * fact, 0.0f * fact },
   { 164.0f * fact, 576.0f * fact },
   { 464.0f * fact, 219.0f * fact },
   { 764.0f * fact, 576.0f * fact },
   { 764.0f * fact, 0.0f * fact },
   { 872.0f * fact, 0.0f * fact },
   { 872.0f * fact, 720.0f * fact },
   { 743.0f * fact, 720.0f * fact },
   { 464.0f * fact, 387.0f * fact }
};

static const SFG_StrokeStrip chr_77_strip[] = { {13, chr_77_part_0} };

SFG_StrokeChar chr_77 = { 928 * fact, 1, chr_77_strip };


// N_.glif
static const SFG_StrokeVertex chr_78_part_0[] = {
   { 668.0f * fact, 144.0f * fact },
   { 184.0f * fact, 720.0f * fact },
   { 56.0f * fact, 720.0f * fact },
   { 56.0f * fact, 0.0f * fact },
   { 164.0f * fact, 0.0f * fact },
   { 164.0f * fact, 576.0f * fact },
   { 648.0f * fact, 0.0f * fact },
   { 776.0f * fact, 0.0f * fact },
   { 776.0f * fact, 720.0f * fact },
   { 668.0f * fact, 720.0f * fact },
   { 668.0f * fact, 144.0f * fact }
};

static const SFG_StrokeStrip chr_78_strip[] = { {11, chr_78_part_0} };

SFG_StrokeChar chr_78 = { 832 * fact, 1, chr_78_strip };


// O_.glif
static const SFG_StrokeVertex chr_79_part_0[] = {
   { 184.0f * fact, 720.0f * fact },
   { 112.0f * fact, 720.0f * fact },
   { 54.0f * fact, 662.0f * fact },
   { 54.0f * fact, 590.0f * fact },
   { 54.0f * fact, 130.0f * fact },
   { 54.0f * fact, 58.0f * fact },
   { 112.0f * fact, 0.0f * fact },
   { 184.0f * fact, 0.0f * fact },
   { 644.0f * fact, 0.0f * fact },
   { 716.0f * fact, 0.0f * fact },
   { 774.0f * fact, 58.0f * fact },
   { 774.0f * fact, 130.0f * fact },
   { 774.0f * fact, 590.0f * fact },
   { 774.0f * fact, 662.0f * fact },
   { 716.0f * fact, 720.0f * fact },
   { 644.0f * fact, 720.0f * fact },
   { 184.0f * fact, 720.0f * fact }
};

// O_.glif
static const SFG_StrokeVertex chr_79_part_1[] = {
   { 184.0f * fact, 108.0f * fact },
   { 172.0f * fact, 108.0f * fact },
   { 162.0f * fact, 118.0f * fact },
   { 162.0f * fact, 130.0f * fact },
   { 162.0f * fact, 590.0f * fact },
   { 162.0f * fact, 602.0f * fact },
   { 172.0f * fact, 612.0f * fact },
   { 184.0f * fact, 612.0f * fact },
   { 644.0f * fact, 612.0f * fact },
   { 656.0f * fact, 612.0f * fact },
   { 666.0f * fact, 602.0f * fact },
   { 666.0f * fact, 590.0f * fact },
   { 666.0f * fact, 130.0f * fact },
   { 666.0f * fact, 118.0f * fact },
   { 656.0f * fact, 108.0f * fact },
   { 644.0f * fact, 108.0f * fact },
   { 184.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_79_strip[] = { {17, chr_79_part_0}, {17, chr_79_part_1} };

SFG_StrokeChar chr_79 = { 828 * fact, 2, chr_79_strip };


// P_.glif
static const SFG_StrokeVertex chr_80_part_0[] = {
   { 56.0f * fact, 719.0f * fact },
   { 56.0f * fact, 0.0f * fact },
   { 164.0f * fact, 0.0f * fact },
   { 164.0f * fact, 259.0f * fact },
   { 171.0f * fact, 258.0f * fact },
   { 178.0f * fact, 257.0f * fact },
   { 186.0f * fact, 257.0f * fact },
   { 646.0f * fact, 257.0f * fact },
   { 718.0f * fact, 257.0f * fact },
   { 776.0f * fact, 316.0f * fact },
   { 776.0f * fact, 387.0f * fact },
   { 776.0f * fact, 589.0f * fact },
   { 776.0f * fact, 660.0f * fact },
   { 718.0f * fact, 719.0f * fact },
   { 646.0f * fact, 719.0f * fact },
   { 56.0f * fact, 719.0f * fact }
};

// P_.glif
static const SFG_StrokeVertex chr_80_part_1[] = {
   { 668.0f * fact, 387.0f * fact },
   { 668.0f * fact, 375.0f * fact },
   { 658.0f * fact, 365.0f * fact },
   { 646.0f * fact, 365.0f * fact },
   { 186.0f * fact, 365.0f * fact },
   { 174.0f * fact, 365.0f * fact },
   { 164.0f * fact, 375.0f * fact },
   { 164.0f * fact, 387.0f * fact },
   { 164.0f * fact, 589.0f * fact },
   { 164.0f * fact, 601.0f * fact },
   { 174.0f * fact, 611.0f * fact },
   { 186.0f * fact, 611.0f * fact },
   { 646.0f * fact, 611.0f * fact },
   { 658.0f * fact, 611.0f * fact },
   { 668.0f * fact, 601.0f * fact },
   { 668.0f * fact, 589.0f * fact },
   { 668.0f * fact, 387.0f * fact }
};

static const SFG_StrokeStrip chr_80_strip[] = { {16, chr_80_part_0}, {17, chr_80_part_1} };

SFG_StrokeChar chr_80 = { 791 * fact, 2, chr_80_strip };


// Q_.glif
static const SFG_StrokeVertex chr_81_part_0[] = {
   { 772.0f * fact, 108.0f * fact },
   { 773.0f * fact, 115.0f * fact },
   { 774.0f * fact, 122.0f * fact },
   { 774.0f * fact, 130.0f * fact },
   { 774.0f * fact, 590.0f * fact },
   { 774.0f * fact, 662.0f * fact },
   { 716.0f * fact, 720.0f * fact },
   { 644.0f * fact, 720.0f * fact },
   { 184.0f * fact, 720.0f * fact },
   { 112.0f * fact, 720.0f * fact },
   { 54.0f * fact, 662.0f * fact },
   { 54.0f * fact, 590.0f * fact },
   { 54.0f * fact, 130.0f * fact },
   { 54.0f * fact, 58.0f * fact },
   { 112.0f * fact, 0.0f * fact },
   { 184.0f * fact, 0.0f * fact },
   { 864.0f * fact, 0.0f * fact },
   { 864.0f * fact, 108.0f * fact },
   { 772.0f * fact, 108.0f * fact }
};

// Q_.glif
static const SFG_StrokeVertex chr_81_part_1[] = {
   { 184.0f * fact, 108.0f * fact },
   { 172.0f * fact, 108.0f * fact },
   { 162.0f * fact, 118.0f * fact },
   { 162.0f * fact, 130.0f * fact },
   { 162.0f * fact, 590.0f * fact },
   { 162.0f * fact, 602.0f * fact },
   { 172.0f * fact, 612.0f * fact },
   { 184.0f * fact, 612.0f * fact },
   { 644.0f * fact, 612.0f * fact },
   { 656.0f * fact, 612.0f * fact },
   { 666.0f * fact, 602.0f * fact },
   { 666.0f * fact, 590.0f * fact },
   { 666.0f * fact, 130.0f * fact },
   { 666.0f * fact, 118.0f * fact },
   { 656.0f * fact, 108.0f * fact },
   { 644.0f * fact, 108.0f * fact },
   { 184.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_81_strip[] = { {19, chr_81_part_0}, {17, chr_81_part_1} };

SFG_StrokeChar chr_81 = { 884 * fact, 2, chr_81_strip };


// R_.glif
static const SFG_StrokeVertex chr_82_part_0[] = {
   { 775.0f * fact, 589.0f * fact },
   { 775.0f * fact, 660.0f * fact },
   { 717.0f * fact, 719.0f * fact },
   { 645.0f * fact, 719.0f * fact },
   { 55.0f * fact, 719.0f * fact },
   { 55.0f * fact, 0.0f * fact },
   { 163.0f * fact, 0.0f * fact },
   { 163.0f * fact, 259.0f * fact },
   { 170.0f * fact, 258.0f * fact },
   { 177.0f * fact, 257.0f * fact },
   { 185.0f * fact, 257.0f * fact },
   { 431.0f * fact, 257.0f * fact },
   { 647.0f * fact, 0.0f * fact },
   { 775.0f * fact, 0.0f * fact },
   { 775.0f * fact, 15.0f * fact },
   { 572.0f * fact, 257.0f * fact },
   { 645.0f * fact, 257.0f * fact },
   { 717.0f * fact, 257.0f * fact },
   { 775.0f * fact, 316.0f * fact },
   { 775.0f * fact, 387.0f * fact },
   { 775.0f * fact, 589.0f * fact }
};

// R_.glif
static const SFG_StrokeVertex chr_82_part_1[] = {
   { 185.0f * fact, 365.0f * fact },
   { 173.0f * fact, 365.0f * fact },
   { 163.0f * fact, 375.0f * fact },
   { 163.0f * fact, 387.0f * fact },
   { 163.0f * fact, 589.0f * fact },
   { 163.0f * fact, 601.0f * fact },
   { 173.0f * fact, 611.0f * fact },
   { 185.0f * fact, 611.0f * fact },
   { 645.0f * fact, 611.0f * fact },
   { 657.0f * fact, 611.0f * fact },
   { 667.0f * fact, 601.0f * fact },
   { 667.0f * fact, 589.0f * fact },
   { 667.0f * fact, 387.0f * fact },
   { 667.0f * fact, 375.0f * fact },
   { 657.0f * fact, 365.0f * fact },
   { 645.0f * fact, 365.0f * fact },
   { 185.0f * fact, 365.0f * fact }
};

static const SFG_StrokeStrip chr_82_strip[] = { {21, chr_82_part_0}, {17, chr_82_part_1} };

SFG_StrokeChar chr_82 = { 825 * fact, 2, chr_82_strip };


// S_.glif
static const SFG_StrokeVertex chr_83_part_0[] = {
   { 771.0f * fact, 590.0f * fact },
   { 771.0f * fact, 662.0f * fact },
   { 713.0f * fact, 720.0f * fact },
   { 641.0f * fact, 720.0f * fact },
   { 181.0f * fact, 720.0f * fact },
   { 109.0f * fact, 720.0f * fact },
   { 51.0f * fact, 662.0f * fact },
   { 51.0f * fact, 590.0f * fact },
   { 51.0f * fact, 436.0f * fact },
   { 51.0f * fact, 364.0f * fact },
   { 109.0f * fact, 306.0f * fact },
   { 181.0f * fact, 306.0f * fact },
   { 641.0f * fact, 306.0f * fact },
   { 653.0f * fact, 306.0f * fact },
   { 663.0f * fact, 296.0f * fact },
   { 663.0f * fact, 284.0f * fact },
   { 663.0f * fact, 130.0f * fact },
   { 663.0f * fact, 118.0f * fact },
   { 653.0f * fact, 108.0f * fact },
   { 641.0f * fact, 108.0f * fact },
   { 181.0f * fact, 108.0f * fact },
   { 169.0f * fact, 108.0f * fact },
   { 159.0f * fact, 118.0f * fact },
   { 159.0f * fact, 130.0f * fact },
   { 159.0f * fact, 172.0f * fact },
   { 51.0f * fact, 172.0f * fact },
   { 51.0f * fact, 130.0f * fact },
   { 51.0f * fact, 58.0f * fact },
   { 109.0f * fact, 0.0f * fact },
   { 181.0f * fact, 0.0f * fact },
   { 641.0f * fact, 0.0f * fact },
   { 713.0f * fact, 0.0f * fact },
   { 771.0f * fact, 58.0f * fact },
   { 771.0f * fact, 130.0f * fact },
   { 771.0f * fact, 284.0f * fact },
   { 771.0f * fact, 356.0f * fact },
   { 713.0f * fact, 414.0f * fact },
   { 641.0f * fact, 414.0f * fact },
   { 181.0f * fact, 414.0f * fact },
   { 169.0f * fact, 414.0f * fact },
   { 159.0f * fact, 424.0f * fact },
   { 159.0f * fact, 436.0f * fact },
   { 159.0f * fact, 590.0f * fact },
   { 159.0f * fact, 602.0f * fact },
   { 169.0f * fact, 612.0f * fact },
   { 181.0f * fact, 612.0f * fact },
   { 641.0f * fact, 612.0f * fact },
   { 653.0f * fact, 612.0f * fact },
   { 663.0f * fact, 602.0f * fact },
   { 663.0f * fact, 590.0f * fact },
   { 663.0f * fact, 548.0f * fact },
   { 771.0f * fact, 548.0f * fact },
   { 771.0f * fact, 590.0f * fact }
};

static const SFG_StrokeStrip chr_83_strip[] = { {53, chr_83_part_0} };

SFG_StrokeChar chr_83 = { 822 * fact, 1, chr_83_strip };


// T_.glif
static const SFG_StrokeVertex chr_84_part_0[] = {
   { 20.0f * fact, 720.0f * fact },
   { 20.0f * fact, 612.0f * fact },
   { 326.0f * fact, 612.0f * fact },
   { 326.0f * fact, 0.0f * fact },
   { 434.0f * fact, 0.0f * fact },
   { 434.0f * fact, 612.0f * fact },
   { 740.0f * fact, 612.0f * fact },
   { 740.0f * fact, 720.0f * fact },
   { 20.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_84_strip[] = { {9, chr_84_part_0} };

SFG_StrokeChar chr_84 = { 759 * fact, 1, chr_84_strip };


// U_.glif
static const SFG_StrokeVertex chr_85_part_0[] = {
   { 666.0f * fact, 130.0f * fact },
   { 666.0f * fact, 118.0f * fact },
   { 656.0f * fact, 108.0f * fact },
   { 644.0f * fact, 108.0f * fact },
   { 184.0f * fact, 108.0f * fact },
   { 172.0f * fact, 108.0f * fact },
   { 162.0f * fact, 118.0f * fact },
   { 162.0f * fact, 130.0f * fact },
   { 162.0f * fact, 720.0f * fact },
   { 54.0f * fact, 720.0f * fact },
   { 54.0f * fact, 130.0f * fact },
   { 54.0f * fact, 58.0f * fact },
   { 112.0f * fact, 0.0f * fact },
   { 184.0f * fact, 0.0f * fact },
   { 644.0f * fact, 0.0f * fact },
   { 716.0f * fact, 0.0f * fact },
   { 774.0f * fact, 58.0f * fact },
   { 774.0f * fact, 130.0f * fact },
   { 774.0f * fact, 720.0f * fact },
   { 666.0f * fact, 720.0f * fact },
   { 666.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_85_strip[] = { {21, chr_85_part_0} };

SFG_StrokeChar chr_85 = { 828 * fact, 1, chr_85_strip };


// V_.glif
static const SFG_StrokeVertex chr_86_part_0[] = {
   { 505.0f * fact, 122.0f * fact },
   { 160.0f * fact, 720.0f * fact },
   { 35.0f * fact, 720.0f * fact },
   { 451.0f * fact, 0.0f * fact },
   { 559.0f * fact, 0.0f * fact },
   { 974.0f * fact, 720.0f * fact },
   { 850.0f * fact, 720.0f * fact },
   { 505.0f * fact, 122.0f * fact }
};

static const SFG_StrokeStrip chr_86_strip[] = { {8, chr_86_part_0} };

SFG_StrokeChar chr_86 = { 1003 * fact, 1, chr_86_strip };


// W_.glif
static const SFG_StrokeVertex chr_87_part_0[] = {
   { 1029.0f * fact, 720.0f * fact },
   { 838.0f * fact, 195.0f * fact },
   { 647.0f * fact, 720.0f * fact },
   { 532.0f * fact, 720.0f * fact },
   { 341.0f * fact, 195.0f * fact },
   { 150.0f * fact, 720.0f * fact },
   { 35.0f * fact, 720.0f * fact },
   { 297.0f * fact, 0.0f * fact },
   { 384.0f * fact, 0.0f * fact },
   { 383.0f * fact, 1.0f * fact },
   { 385.0f * fact, 0.0f * fact },
   { 589.0f * fact, 561.0f * fact },
   { 794.0f * fact, 0.0f * fact },
   { 882.0f * fact, 0.0f * fact },
   { 1144.0f * fact, 720.0f * fact },
   { 1029.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_87_strip[] = { {16, chr_87_part_0} };

SFG_StrokeChar chr_87 = { 1179 * fact, 1, chr_87_strip };


// X_.glif
static const SFG_StrokeVertex chr_88_part_0[] = {
   { 638.0f * fact, 720.0f * fact },
   { 406.0f * fact, 444.0f * fact },
   { 174.0f * fact, 720.0f * fact },
   { 46.0f * fact, 720.0f * fact },
   { 46.0f * fact, 705.0f * fact },
   { 335.0f * fact, 360.0f * fact },
   { 46.0f * fact, 15.0f * fact },
   { 46.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 406.0f * fact, 276.0f * fact },
   { 638.0f * fact, 0.0f * fact },
   { 766.0f * fact, 0.0f * fact },
   { 766.0f * fact, 15.0f * fact },
   { 476.0f * fact, 360.0f * fact },
   { 766.0f * fact, 705.0f * fact },
   { 766.0f * fact, 720.0f * fact },
   { 638.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_88_strip[] = { {17, chr_88_part_0} };

SFG_StrokeChar chr_88 = { 812 * fact, 1, chr_88_strip };


// Y_.glif
static const SFG_StrokeVertex chr_89_part_0[] = {
   { 661.0f * fact, 720.0f * fact },
   { 403.0f * fact, 392.0f * fact },
   { 144.0f * fact, 720.0f * fact },
   { 17.0f * fact, 720.0f * fact },
   { 349.0f * fact, 270.0f * fact },
   { 349.0f * fact, 0.0f * fact },
   { 457.0f * fact, 0.0f * fact },
   { 457.0f * fact, 270.0f * fact },
   { 789.0f * fact, 720.0f * fact },
   { 661.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_89_strip[] = { {10, chr_89_part_0} };

SFG_StrokeChar chr_89 = { 806 * fact, 1, chr_89_strip };


// Z_.glif
static const SFG_StrokeVertex chr_90_part_0[] = {
   { 51.0f * fact, 612.0f * fact },
   { 627.0f * fact, 612.0f * fact },
   { 51.0f * fact, 128.0f * fact },
   { 51.0f * fact, 0.0f * fact },
   { 771.0f * fact, 0.0f * fact },
   { 771.0f * fact, 108.0f * fact },
   { 195.0f * fact, 108.0f * fact },
   { 771.0f * fact, 592.0f * fact },
   { 771.0f * fact, 720.0f * fact },
   { 51.0f * fact, 720.0f * fact },
   { 51.0f * fact, 612.0f * fact }
};

static const SFG_StrokeStrip chr_90_strip[] = { {11, chr_90_part_0} };

SFG_StrokeChar chr_90 = { 821 * fact, 1, chr_90_strip };


// bracketleft.glif
static const SFG_StrokeVertex chr_91_part_0[] = {
   { 54.0f * fact, 0.0f * fact },
   { 226.0f * fact, 0.0f * fact },
   { 226.0f * fact, 108.0f * fact },
   { 162.0f * fact, 108.0f * fact },
   { 162.0f * fact, 612.0f * fact },
   { 226.0f * fact, 612.0f * fact },
   { 226.0f * fact, 720.0f * fact },
   { 54.0f * fact, 720.0f * fact },
   { 54.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_91_strip[] = { {9, chr_91_part_0} };

SFG_StrokeChar chr_91 = { 275 * fact, 1, chr_91_strip };


// backslash.glif
static const SFG_StrokeVertex chr_92_part_0[] = {
   { 5.0f * fact, 720.0f * fact },
   { 5.0f * fact, 592.0f * fact },
   { 500.0f * fact, 0.0f * fact },
   { 515.0f * fact, 0.0f * fact },
   { 515.0f * fact, 128.0f * fact },
   { 20.0f * fact, 720.0f * fact },
   { 5.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_92_strip[] = { {7, chr_92_part_0} };

SFG_StrokeChar chr_92 = { 520 * fact, 1, chr_92_strip };


// bracketright.glif
static const SFG_StrokeVertex chr_93_part_0[] = {
   { 51.0f * fact, 612.0f * fact },
   { 115.0f * fact, 612.0f * fact },
   { 115.0f * fact, 108.0f * fact },
   { 51.0f * fact, 108.0f * fact },
   { 51.0f * fact, 0.0f * fact },
   { 223.0f * fact, 0.0f * fact },
   { 223.0f * fact, 720.0f * fact },
   { 51.0f * fact, 720.0f * fact },
   { 51.0f * fact, 612.0f * fact }
};

static const SFG_StrokeStrip chr_93_strip[] = { {9, chr_93_part_0} };

SFG_StrokeChar chr_93 = { 276 * fact, 1, chr_93_strip };


// underscore.glif
static const SFG_StrokeVertex chr_95_part_0[] = {
   { 774.0f * fact, 0.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 54.0f * fact, -108.0f * fact },
   { 774.0f * fact, -108.0f * fact },
   { 774.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_95_strip[] = { {5, chr_95_part_0} };

SFG_StrokeChar chr_95 = { 828 * fact, 1, chr_95_strip };


// grave.glif
static const SFG_StrokeVertex chr_96_part_0[] = {
   { 140.0f * fact, 989.0f * fact },
   { 32.0f * fact, 989.0f * fact },
   { 72.0f * fact, 827.0f * fact },
   { 180.0f * fact, 827.0f * fact },
   { 140.0f * fact, 989.0f * fact }
};

static const SFG_StrokeStrip chr_96_strip[] = { {5, chr_96_part_0} };

SFG_StrokeChar chr_96 = { 213 * fact, 1, chr_96_strip };


// a.glif
static const SFG_StrokeVertex chr_97_part_0[] = {
   { 52.0f * fact, 580.0f * fact },
   { 52.0f * fact, 472.0f * fact },
   { 512.0f * fact, 472.0f * fact },
   { 524.0f * fact, 472.0f * fact },
   { 534.0f * fact, 462.0f * fact },
   { 534.0f * fact, 450.0f * fact },
   { 534.0f * fact, 344.0f * fact },
   { 52.0f * fact, 344.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 58.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 182.0f * fact, 0.0f * fact },
   { 642.0f * fact, 0.0f * fact },
   { 642.0f * fact, 450.0f * fact },
   { 642.0f * fact, 522.0f * fact },
   { 584.0f * fact, 580.0f * fact },
   { 512.0f * fact, 580.0f * fact },
   { 52.0f * fact, 580.0f * fact }
};

// a.glif
static const SFG_StrokeVertex chr_97_part_1[] = {
   { 534.0f * fact, 108.0f * fact },
   { 182.0f * fact, 108.0f * fact },
   { 170.0f * fact, 108.0f * fact },
   { 160.0f * fact, 118.0f * fact },
   { 160.0f * fact, 130.0f * fact },
   { 160.0f * fact, 236.0f * fact },
   { 534.0f * fact, 236.0f * fact },
   { 534.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_97_strip[] = { {18, chr_97_part_0}, {8, chr_97_part_1} };

SFG_StrokeChar chr_97 = { 694 * fact, 2, chr_97_strip };


// b.glif
static const SFG_StrokeVertex chr_98_part_0[] = {
   { 162.0f * fact, 580.0f * fact },
   { 162.0f * fact, 770.0f * fact },
   { 54.0f * fact, 770.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 514.0f * fact, 0.0f * fact },
   { 586.0f * fact, 0.0f * fact },
   { 644.0f * fact, 58.0f * fact },
   { 644.0f * fact, 130.0f * fact },
   { 644.0f * fact, 450.0f * fact },
   { 644.0f * fact, 522.0f * fact },
   { 586.0f * fact, 580.0f * fact },
   { 514.0f * fact, 580.0f * fact },
   { 162.0f * fact, 580.0f * fact }
};

// b.glif
static const SFG_StrokeVertex chr_98_part_1[] = {
   { 536.0f * fact, 130.0f * fact },
   { 536.0f * fact, 118.0f * fact },
   { 526.0f * fact, 108.0f * fact },
   { 514.0f * fact, 108.0f * fact },
   { 184.0f * fact, 108.0f * fact },
   { 172.0f * fact, 108.0f * fact },
   { 162.0f * fact, 118.0f * fact },
   { 162.0f * fact, 130.0f * fact },
   { 162.0f * fact, 450.0f * fact },
   { 162.0f * fact, 462.0f * fact },
   { 172.0f * fact, 472.0f * fact },
   { 184.0f * fact, 472.0f * fact },
   { 514.0f * fact, 472.0f * fact },
   { 526.0f * fact, 472.0f * fact },
   { 536.0f * fact, 462.0f * fact },
   { 536.0f * fact, 450.0f * fact },
   { 536.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_98_strip[] = { {13, chr_98_part_0}, {17, chr_98_part_1} };

SFG_StrokeChar chr_98 = { 667 * fact, 2, chr_98_strip };


// c.glif
static const SFG_StrokeVertex chr_99_part_0[] = {
   { 181.0f * fact, 108.0f * fact },
   { 169.0f * fact, 108.0f * fact },
   { 159.0f * fact, 118.0f * fact },
   { 159.0f * fact, 130.0f * fact },
   { 159.0f * fact, 450.0f * fact },
   { 159.0f * fact, 462.0f * fact },
   { 169.0f * fact, 472.0f * fact },
   { 181.0f * fact, 472.0f * fact },
   { 639.0f * fact, 472.0f * fact },
   { 639.0f * fact, 580.0f * fact },
   { 181.0f * fact, 580.0f * fact },
   { 109.0f * fact, 580.0f * fact },
   { 51.0f * fact, 522.0f * fact },
   { 51.0f * fact, 450.0f * fact },
   { 51.0f * fact, 130.0f * fact },
   { 51.0f * fact, 58.0f * fact },
   { 109.0f * fact, 0.0f * fact },
   { 181.0f * fact, 0.0f * fact },
   { 641.0f * fact, 0.0f * fact },
   { 641.0f * fact, 108.0f * fact },
   { 181.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_99_strip[] = { {21, chr_99_part_0} };

SFG_StrokeChar chr_99 = { 695 * fact, 1, chr_99_strip };


// d.glif
static const SFG_StrokeVertex chr_100_part_0[] = {
   { 505.0f * fact, 770.0f * fact },
   { 505.0f * fact, 580.0f * fact },
   { 153.0f * fact, 580.0f * fact },
   { 81.0f * fact, 580.0f * fact },
   { 23.0f * fact, 522.0f * fact },
   { 23.0f * fact, 450.0f * fact },
   { 23.0f * fact, 130.0f * fact },
   { 23.0f * fact, 58.0f * fact },
   { 81.0f * fact, 0.0f * fact },
   { 153.0f * fact, 0.0f * fact },
   { 613.0f * fact, 0.0f * fact },
   { 613.0f * fact, 770.0f * fact },
   { 505.0f * fact, 770.0f * fact }
};

// d.glif
static const SFG_StrokeVertex chr_100_part_1[] = {
   { 153.0f * fact, 108.0f * fact },
   { 141.0f * fact, 108.0f * fact },
   { 131.0f * fact, 118.0f * fact },
   { 131.0f * fact, 130.0f * fact },
   { 131.0f * fact, 450.0f * fact },
   { 131.0f * fact, 462.0f * fact },
   { 141.0f * fact, 472.0f * fact },
   { 153.0f * fact, 472.0f * fact },
   { 483.0f * fact, 472.0f * fact },
   { 495.0f * fact, 472.0f * fact },
   { 505.0f * fact, 462.0f * fact },
   { 505.0f * fact, 450.0f * fact },
   { 505.0f * fact, 130.0f * fact },
   { 505.0f * fact, 118.0f * fact },
   { 495.0f * fact, 108.0f * fact },
   { 483.0f * fact, 108.0f * fact },
   { 153.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_100_strip[] = { {13, chr_100_part_0}, {17, chr_100_part_1} };

SFG_StrokeChar chr_100 = { 667 * fact, 2, chr_100_strip };


// e.glif
static const SFG_StrokeVertex chr_101_part_0[] = {
   { 181.0f * fact, 580.0f * fact },
   { 109.0f * fact, 580.0f * fact },
   { 51.0f * fact, 522.0f * fact },
   { 51.0f * fact, 450.0f * fact },
   { 51.0f * fact, 130.0f * fact },
   { 51.0f * fact, 58.0f * fact },
   { 109.0f * fact, 0.0f * fact },
   { 181.0f * fact, 0.0f * fact },
   { 641.0f * fact, 0.0f * fact },
   { 641.0f * fact, 108.0f * fact },
   { 181.0f * fact, 108.0f * fact },
   { 169.0f * fact, 108.0f * fact },
   { 159.0f * fact, 118.0f * fact },
   { 159.0f * fact, 130.0f * fact },
   { 159.0f * fact, 236.0f * fact },
   { 641.0f * fact, 236.0f * fact },
   { 641.0f * fact, 450.0f * fact },
   { 641.0f * fact, 522.0f * fact },
   { 583.0f * fact, 580.0f * fact },
   { 511.0f * fact, 580.0f * fact },
   { 181.0f * fact, 580.0f * fact }
};

// e.glif
static const SFG_StrokeVertex chr_101_part_1[] = {
   { 159.0f * fact, 344.0f * fact },
   { 159.0f * fact, 450.0f * fact },
   { 159.0f * fact, 462.0f * fact },
   { 169.0f * fact, 472.0f * fact },
   { 181.0f * fact, 472.0f * fact },
   { 511.0f * fact, 472.0f * fact },
   { 523.0f * fact, 472.0f * fact },
   { 533.0f * fact, 462.0f * fact },
   { 533.0f * fact, 450.0f * fact },
   { 533.0f * fact, 344.0f * fact },
   { 159.0f * fact, 344.0f * fact }
};

static const SFG_StrokeStrip chr_101_strip[] = { {21, chr_101_part_0}, {11, chr_101_part_1} };

SFG_StrokeChar chr_101 = { 692 * fact, 2, chr_101_strip };


// f.glif
static const SFG_StrokeVertex chr_102_part_0[] = {
   { 399.0f * fact, 662.0f * fact },
   { 399.0f * fact, 770.0f * fact },
   { 183.0f * fact, 770.0f * fact },
   { 111.0f * fact, 770.0f * fact },
   { 53.0f * fact, 712.0f * fact },
   { 53.0f * fact, 640.0f * fact },
   { 53.0f * fact, 320.0f * fact },
   { 53.0f * fact, 320.0f * fact },
   { 53.0f * fact, 320.0f * fact },
   { 53.0f * fact, 320.0f * fact },
   { 53.0f * fact, 0.0f * fact },
   { 161.0f * fact, 0.0f * fact },
   { 161.0f * fact, 472.0f * fact },
   { 399.0f * fact, 472.0f * fact },
   { 399.0f * fact, 580.0f * fact },
   { 161.0f * fact, 580.0f * fact },
   { 161.0f * fact, 640.0f * fact },
   { 161.0f * fact, 652.0f * fact },
   { 171.0f * fact, 662.0f * fact },
   { 183.0f * fact, 662.0f * fact },
   { 399.0f * fact, 662.0f * fact }
};

static const SFG_StrokeStrip chr_102_strip[] = { {21, chr_102_part_0} };

SFG_StrokeChar chr_102 = { 407 * fact, 1, chr_102_strip };


// g.glif
static const SFG_StrokeVertex chr_103_part_0[] = {
   { 631.0f * fact, 450.0f * fact },
   { 631.0f * fact, 522.0f * fact },
   { 573.0f * fact, 580.0f * fact },
   { 501.0f * fact, 580.0f * fact },
   { 171.0f * fact, 580.0f * fact },
   { 99.0f * fact, 580.0f * fact },
   { 41.0f * fact, 522.0f * fact },
   { 41.0f * fact, 450.0f * fact },
   { 41.0f * fact, 130.0f * fact },
   { 41.0f * fact, 58.0f * fact },
   { 99.0f * fact, 0.0f * fact },
   { 171.0f * fact, 0.0f * fact },
   { 523.0f * fact, 0.0f * fact },
   { 523.0f * fact, -100.0f * fact },
   { 523.0f * fact, -112.0f * fact },
   { 513.0f * fact, -122.0f * fact },
   { 501.0f * fact, -122.0f * fact },
   { 143.0f * fact, -122.0f * fact },
   { 143.0f * fact, -230.0f * fact },
   { 501.0f * fact, -230.0f * fact },
   { 573.0f * fact, -230.0f * fact },
   { 631.0f * fact, -172.0f * fact },
   { 631.0f * fact, -100.0f * fact },
   { 631.0f * fact, 220.0f * fact },
   { 631.0f * fact, 220.0f * fact },
   { 631.0f * fact, 221.0f * fact },
   { 631.0f * fact, 221.0f * fact },
   { 631.0f * fact, 450.0f * fact }
};

// g.glif
static const SFG_StrokeVertex chr_103_part_1[] = {
   { 171.0f * fact, 108.0f * fact },
   { 159.0f * fact, 108.0f * fact },
   { 149.0f * fact, 118.0f * fact },
   { 149.0f * fact, 130.0f * fact },
   { 149.0f * fact, 450.0f * fact },
   { 149.0f * fact, 462.0f * fact },
   { 159.0f * fact, 472.0f * fact },
   { 171.0f * fact, 472.0f * fact },
   { 501.0f * fact, 472.0f * fact },
   { 513.0f * fact, 472.0f * fact },
   { 523.0f * fact, 462.0f * fact },
   { 523.0f * fact, 450.0f * fact },
   { 523.0f * fact, 130.0f * fact },
   { 523.0f * fact, 118.0f * fact },
   { 513.0f * fact, 108.0f * fact },
   { 501.0f * fact, 108.0f * fact },
   { 171.0f * fact, 108.0f * fact }
};

static const SFG_StrokeStrip chr_103_strip[] = { {28, chr_103_part_0}, {17, chr_103_part_1} };

SFG_StrokeChar chr_103 = { 683 * fact, 2, chr_103_strip };


// h.glif
static const SFG_StrokeVertex chr_104_part_0[] = {
   { 162.0f * fact, 580.0f * fact },
   { 162.0f * fact, 770.0f * fact },
   { 54.0f * fact, 770.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 450.0f * fact },
   { 162.0f * fact, 462.0f * fact },
   { 172.0f * fact, 472.0f * fact },
   { 184.0f * fact, 472.0f * fact },
   { 514.0f * fact, 472.0f * fact },
   { 526.0f * fact, 472.0f * fact },
   { 536.0f * fact, 462.0f * fact },
   { 536.0f * fact, 450.0f * fact },
   { 536.0f * fact, 0.0f * fact },
   { 644.0f * fact, 0.0f * fact },
   { 644.0f * fact, 450.0f * fact },
   { 644.0f * fact, 522.0f * fact },
   { 585.0f * fact, 580.0f * fact },
   { 514.0f * fact, 580.0f * fact },
   { 162.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_104_strip[] = { {20, chr_104_part_0} };

SFG_StrokeChar chr_104 = { 668 * fact, 1, chr_104_strip };


// i.glif
static const SFG_StrokeVertex chr_105_part_0[] = {
   { 52.0f * fact, 0.0f * fact },
   { 160.0f * fact, 0.0f * fact },
   { 160.0f * fact, 580.0f * fact },
   { 52.0f * fact, 580.0f * fact },
   { 52.0f * fact, 0.0f * fact }
};

// i.glif
static const SFG_StrokeVertex chr_105_part_1[] = {
   { 52.0f * fact, 770.0f * fact },
   { 52.0f * fact, 662.0f * fact },
   { 160.0f * fact, 662.0f * fact },
   { 160.0f * fact, 770.0f * fact },
   { 52.0f * fact, 770.0f * fact }
};

static const SFG_StrokeStrip chr_105_strip[] = { {5, chr_105_part_0}, {5, chr_105_part_1} };

SFG_StrokeChar chr_105 = { 208 * fact, 2, chr_105_strip };


// j.glif
static const SFG_StrokeVertex chr_106_part_0[] = {
   { 83.0f * fact, 770.0f * fact },
   { 83.0f * fact, 662.0f * fact },
   { 191.0f * fact, 662.0f * fact },
   { 191.0f * fact, 770.0f * fact },
   { 83.0f * fact, 770.0f * fact }
};

// j.glif
static const SFG_StrokeVertex chr_106_part_1[] = {
   { 191.0f * fact, 578.0f * fact },
   { 83.0f * fact, 578.0f * fact },
   { 83.0f * fact, -100.0f * fact },
   { 83.0f * fact, -112.0f * fact },
   { 73.0f * fact, -122.0f * fact },
   { 61.0f * fact, -122.0f * fact },
   { -187.0f * fact, -122.0f * fact },
   { -187.0f * fact, -230.0f * fact },
   { 61.0f * fact, -230.0f * fact },
   { 133.0f * fact, -230.0f * fact },
   { 191.0f * fact, -172.0f * fact },
   { 191.0f * fact, -100.0f * fact },
   { 191.0f * fact, 220.0f * fact },
   { 191.0f * fact, 220.0f * fact },
   { 191.0f * fact, 220.0f * fact },
   { 191.0f * fact, 220.0f * fact },
   { 191.0f * fact, 578.0f * fact }
};

static const SFG_StrokeStrip chr_106_strip[] = { {5, chr_106_part_0}, {17, chr_106_part_1} };

SFG_StrokeChar chr_106 = { 239 * fact, 2, chr_106_strip };


// k.glif
static const SFG_StrokeVertex chr_107_part_0[] = {
   { 509.0f * fact, 580.0f * fact },
   { 292.0f * fact, 344.0f * fact },
   { 162.0f * fact, 344.0f * fact },
   { 162.0f * fact, 770.0f * fact },
   { 54.0f * fact, 770.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 236.0f * fact },
   { 292.0f * fact, 236.0f * fact },
   { 509.0f * fact, 0.0f * fact },
   { 637.0f * fact, 0.0f * fact },
   { 637.0f * fact, 15.0f * fact },
   { 388.0f * fact, 290.0f * fact },
   { 637.0f * fact, 565.0f * fact },
   { 637.0f * fact, 580.0f * fact },
   { 509.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_107_strip[] = { {16, chr_107_part_0} };

SFG_StrokeChar chr_107 = { 646 * fact, 1, chr_107_strip };


// l.glif
static const SFG_StrokeVertex chr_108_part_0[] = {
   { 52.0f * fact, 450.0f * fact },
   { 52.0f * fact, 450.0f * fact },
   { 52.0f * fact, 450.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 58.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 182.0f * fact, 0.0f * fact },
   { 290.0f * fact, 0.0f * fact },
   { 290.0f * fact, 108.0f * fact },
   { 182.0f * fact, 108.0f * fact },
   { 170.0f * fact, 108.0f * fact },
   { 160.0f * fact, 118.0f * fact },
   { 160.0f * fact, 130.0f * fact },
   { 160.0f * fact, 770.0f * fact },
   { 52.0f * fact, 770.0f * fact },
   { 52.0f * fact, 450.0f * fact },
   { 52.0f * fact, 450.0f * fact }
};

static const SFG_StrokeStrip chr_108_strip[] = { {17, chr_108_part_0} };

SFG_StrokeChar chr_108 = { 302 * fact, 1, chr_108_strip };


// m.glif
static const SFG_StrokeVertex chr_109_part_0[] = {
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 450.0f * fact },
   { 162.0f * fact, 462.0f * fact },
   { 172.0f * fact, 472.0f * fact },
   { 184.0f * fact, 472.0f * fact },
   { 414.0f * fact, 472.0f * fact },
   { 426.0f * fact, 472.0f * fact },
   { 436.0f * fact, 462.0f * fact },
   { 436.0f * fact, 450.0f * fact },
   { 436.0f * fact, 0.0f * fact },
   { 544.0f * fact, 0.0f * fact },
   { 544.0f * fact, 450.0f * fact },
   { 544.0f * fact, 462.0f * fact },
   { 554.0f * fact, 472.0f * fact },
   { 566.0f * fact, 472.0f * fact },
   { 795.0f * fact, 472.0f * fact },
   { 808.0f * fact, 472.0f * fact },
   { 818.0f * fact, 462.0f * fact },
   { 818.0f * fact, 450.0f * fact },
   { 818.0f * fact, 0.0f * fact },
   { 925.0f * fact, 0.0f * fact },
   { 925.0f * fact, 450.0f * fact },
   { 925.0f * fact, 522.0f * fact },
   { 867.0f * fact, 580.0f * fact },
   { 795.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_109_strip[] = { {28, chr_109_part_0} };

SFG_StrokeChar chr_109 = { 978 * fact, 1, chr_109_strip };


// n.glif
static const SFG_StrokeVertex chr_110_part_0[] = {
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 162.0f * fact, 450.0f * fact },
   { 162.0f * fact, 462.0f * fact },
   { 172.0f * fact, 472.0f * fact },
   { 184.0f * fact, 472.0f * fact },
   { 514.0f * fact, 472.0f * fact },
   { 526.0f * fact, 472.0f * fact },
   { 536.0f * fact, 462.0f * fact },
   { 536.0f * fact, 450.0f * fact },
   { 536.0f * fact, 0.0f * fact },
   { 644.0f * fact, 0.0f * fact },
   { 644.0f * fact, 450.0f * fact },
   { 644.0f * fact, 522.0f * fact },
   { 586.0f * fact, 580.0f * fact },
   { 514.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_110_strip[] = { {18, chr_110_part_0} };

SFG_StrokeChar chr_110 = { 696 * fact, 1, chr_110_strip };


// o.glif
static const SFG_StrokeVertex chr_111_part_0[] = {
   { 181.0f * fact, 580.0f * fact },
   { 109.0f * fact, 580.0f * fact },
   { 51.0f * fact, 522.0f * fact },
   { 51.0f * fact, 450.0f * fact },
   { 51.0f * fact, 130.0f * fact },
   { 51.0f * fact, 58.0f * fact },
   { 109.0f * fact, 0.0f * fact },
   { 181.0f * fact, 0.0f * fact },
   { 511.0f * fact, 0.0f * fact },
   { 583.0f * fact, 0.0f * fact },
   { 641.0f * fact, 58.0f * fact },
   { 641.0f * fact, 130.0f * fact },
   { 641.0f * fact, 450.0f * fact },
   { 641.0f * fact, 522.0f * fact },
   { 583.0f * fact, 580.0f * fact },
   { 511.0f * fact, 580.0f * fact },
   { 181.0f * fact, 580.0f * fact }
};

// o.glif
static const SFG_StrokeVertex chr_111_part_1[] = {
   { 511.0f * fact, 472.0f * fact },
   { 523.0f * fact, 472.0f * fact },
   { 533.0f * fact, 462.0f * fact },
   { 533.0f * fact, 450.0f * fact },
   { 533.0f * fact, 130.0f * fact },
   { 533.0f * fact, 118.0f * fact },
   { 523.0f * fact, 108.0f * fact },
   { 511.0f * fact, 108.0f * fact },
   { 181.0f * fact, 108.0f * fact },
   { 169.0f * fact, 108.0f * fact },
   { 159.0f * fact, 118.0f * fact },
   { 159.0f * fact, 130.0f * fact },
   { 159.0f * fact, 450.0f * fact },
   { 159.0f * fact, 462.0f * fact },
   { 169.0f * fact, 472.0f * fact },
   { 181.0f * fact, 472.0f * fact },
   { 511.0f * fact, 472.0f * fact }
};

static const SFG_StrokeStrip chr_111_strip[] = { {17, chr_111_part_0}, {17, chr_111_part_1} };

SFG_StrokeChar chr_111 = { 692 * fact, 2, chr_111_strip };


// p.glif
static const SFG_StrokeVertex chr_112_part_0[] = {
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, -230.0f * fact },
   { 162.0f * fact, -230.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 514.0f * fact, 0.0f * fact },
   { 586.0f * fact, 0.0f * fact },
   { 644.0f * fact, 58.0f * fact },
   { 644.0f * fact, 130.0f * fact },
   { 644.0f * fact, 450.0f * fact },
   { 644.0f * fact, 522.0f * fact },
   { 586.0f * fact, 580.0f * fact },
   { 514.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact }
};

// p.glif
static const SFG_StrokeVertex chr_112_part_1[] = {
   { 536.0f * fact, 130.0f * fact },
   { 536.0f * fact, 118.0f * fact },
   { 526.0f * fact, 108.0f * fact },
   { 514.0f * fact, 108.0f * fact },
   { 184.0f * fact, 108.0f * fact },
   { 172.0f * fact, 108.0f * fact },
   { 162.0f * fact, 118.0f * fact },
   { 162.0f * fact, 130.0f * fact },
   { 162.0f * fact, 450.0f * fact },
   { 162.0f * fact, 462.0f * fact },
   { 172.0f * fact, 472.0f * fact },
   { 184.0f * fact, 472.0f * fact },
   { 514.0f * fact, 472.0f * fact },
   { 526.0f * fact, 472.0f * fact },
   { 536.0f * fact, 462.0f * fact },
   { 536.0f * fact, 450.0f * fact },
   { 536.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_112_strip[] = { {13, chr_112_part_0}, {17, chr_112_part_1} };

SFG_StrokeChar chr_112 = { 664 * fact, 2, chr_112_strip };


// q.glif
static const SFG_StrokeVertex chr_113_part_0[] = {
   { 20.0f * fact, 130.0f * fact },
   { 20.0f * fact, 58.0f * fact },
   { 78.0f * fact, 0.0f * fact },
   { 150.0f * fact, 0.0f * fact },
   { 502.0f * fact, 0.0f * fact },
   { 502.0f * fact, -230.0f * fact },
   { 610.0f * fact, -230.0f * fact },
   { 610.0f * fact, 580.0f * fact },
   { 150.0f * fact, 580.0f * fact },
   { 78.0f * fact, 580.0f * fact },
   { 20.0f * fact, 522.0f * fact },
   { 20.0f * fact, 450.0f * fact },
   { 20.0f * fact, 130.0f * fact }
};

// q.glif
static const SFG_StrokeVertex chr_113_part_1[] = {
   { 128.0f * fact, 450.0f * fact },
   { 128.0f * fact, 462.0f * fact },
   { 138.0f * fact, 472.0f * fact },
   { 150.0f * fact, 472.0f * fact },
   { 480.0f * fact, 472.0f * fact },
   { 492.0f * fact, 472.0f * fact },
   { 502.0f * fact, 462.0f * fact },
   { 502.0f * fact, 450.0f * fact },
   { 502.0f * fact, 130.0f * fact },
   { 502.0f * fact, 118.0f * fact },
   { 492.0f * fact, 108.0f * fact },
   { 480.0f * fact, 108.0f * fact },
   { 150.0f * fact, 108.0f * fact },
   { 138.0f * fact, 108.0f * fact },
   { 128.0f * fact, 118.0f * fact },
   { 128.0f * fact, 130.0f * fact },
   { 128.0f * fact, 450.0f * fact }
};

static const SFG_StrokeStrip chr_113_strip[] = { {13, chr_113_part_0}, {17, chr_113_part_1} };

SFG_StrokeChar chr_113 = { 664 * fact, 2, chr_113_strip };


// r.glif
static const SFG_StrokeVertex chr_114_part_0[] = {
   { 182.0f * fact, 580.0f * fact },
   { 110.0f * fact, 580.0f * fact },
   { 52.0f * fact, 522.0f * fact },
   { 52.0f * fact, 450.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 130.0f * fact },
   { 52.0f * fact, 0.0f * fact },
   { 160.0f * fact, 0.0f * fact },
   { 160.0f * fact, 450.0f * fact },
   { 160.0f * fact, 462.0f * fact },
   { 170.0f * fact, 472.0f * fact },
   { 182.0f * fact, 472.0f * fact },
   { 506.0f * fact, 472.0f * fact },
   { 506.0f * fact, 580.0f * fact },
   { 182.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_114_strip[] = { {17, chr_114_part_0} };

SFG_StrokeChar chr_114 = { 512 * fact, 1, chr_114_strip };


// s.glif
static const SFG_StrokeVertex chr_115_part_0[] = {
   { 638.0f * fact, 450.0f * fact },
   { 638.0f * fact, 522.0f * fact },
   { 580.0f * fact, 580.0f * fact },
   { 508.0f * fact, 580.0f * fact },
   { 178.0f * fact, 580.0f * fact },
   { 106.0f * fact, 580.0f * fact },
   { 48.0f * fact, 522.0f * fact },
   { 48.0f * fact, 450.0f * fact },
   { 48.0f * fact, 366.0f * fact },
   { 48.0f * fact, 294.0f * fact },
   { 106.0f * fact, 236.0f * fact },
   { 178.0f * fact, 236.0f * fact },
   { 508.0f * fact, 236.0f * fact },
   { 520.0f * fact, 236.0f * fact },
   { 530.0f * fact, 226.0f * fact },
   { 530.0f * fact, 214.0f * fact },
   { 530.0f * fact, 130.0f * fact },
   { 530.0f * fact, 118.0f * fact },
   { 520.0f * fact, 108.0f * fact },
   { 508.0f * fact, 108.0f * fact },
   { 178.0f * fact, 108.0f * fact },
   { 166.0f * fact, 108.0f * fact },
   { 156.0f * fact, 118.0f * fact },
   { 156.0f * fact, 130.0f * fact },
   { 156.0f * fact, 152.0f * fact },
   { 48.0f * fact, 152.0f * fact },
   { 48.0f * fact, 130.0f * fact },
   { 48.0f * fact, 58.0f * fact },
   { 106.0f * fact, 0.0f * fact },
   { 178.0f * fact, 0.0f * fact },
   { 508.0f * fact, 0.0f * fact },
   { 580.0f * fact, 0.0f * fact },
   { 638.0f * fact, 58.0f * fact },
   { 638.0f * fact, 130.0f * fact },
   { 638.0f * fact, 214.0f * fact },
   { 638.0f * fact, 286.0f * fact },
   { 580.0f * fact, 344.0f * fact },
   { 508.0f * fact, 344.0f * fact },
   { 178.0f * fact, 344.0f * fact },
   { 166.0f * fact, 344.0f * fact },
   { 156.0f * fact, 354.0f * fact },
   { 156.0f * fact, 366.0f * fact },
   { 156.0f * fact, 450.0f * fact },
   { 156.0f * fact, 462.0f * fact },
   { 166.0f * fact, 472.0f * fact },
   { 178.0f * fact, 472.0f * fact },
   { 508.0f * fact, 472.0f * fact },
   { 520.0f * fact, 472.0f * fact },
   { 530.0f * fact, 462.0f * fact },
   { 530.0f * fact, 450.0f * fact },
   { 530.0f * fact, 428.0f * fact },
   { 638.0f * fact, 428.0f * fact },
   { 638.0f * fact, 450.0f * fact }
};

static const SFG_StrokeStrip chr_115_strip[] = { {53, chr_115_part_0} };

SFG_StrokeChar chr_115 = { 686 * fact, 1, chr_115_strip };


// t.glif
static const SFG_StrokeVertex chr_116_part_0[] = {
   { 399.0f * fact, 472.0f * fact },
   { 399.0f * fact, 580.0f * fact },
   { 161.0f * fact, 580.0f * fact },
   { 161.0f * fact, 770.0f * fact },
   { 53.0f * fact, 770.0f * fact },
   { 53.0f * fact, 450.0f * fact },
   { 53.0f * fact, 450.0f * fact },
   { 53.0f * fact, 450.0f * fact },
   { 53.0f * fact, 450.0f * fact },
   { 53.0f * fact, 130.0f * fact },
   { 53.0f * fact, 58.0f * fact },
   { 111.0f * fact, 0.0f * fact },
   { 183.0f * fact, 0.0f * fact },
   { 399.0f * fact, 0.0f * fact },
   { 399.0f * fact, 108.0f * fact },
   { 183.0f * fact, 108.0f * fact },
   { 171.0f * fact, 108.0f * fact },
   { 161.0f * fact, 118.0f * fact },
   { 161.0f * fact, 130.0f * fact },
   { 161.0f * fact, 472.0f * fact },
   { 399.0f * fact, 472.0f * fact }
};

static const SFG_StrokeStrip chr_116_strip[] = { {21, chr_116_part_0} };

SFG_StrokeChar chr_116 = { 410 * fact, 1, chr_116_strip };


// u.glif
static const SFG_StrokeVertex chr_117_part_0[] = {
   { 535.0f * fact, 580.0f * fact },
   { 535.0f * fact, 130.0f * fact },
   { 535.0f * fact, 118.0f * fact },
   { 525.0f * fact, 108.0f * fact },
   { 513.0f * fact, 108.0f * fact },
   { 183.0f * fact, 108.0f * fact },
   { 171.0f * fact, 108.0f * fact },
   { 161.0f * fact, 118.0f * fact },
   { 161.0f * fact, 130.0f * fact },
   { 161.0f * fact, 580.0f * fact },
   { 53.0f * fact, 580.0f * fact },
   { 53.0f * fact, 130.0f * fact },
   { 53.0f * fact, 58.0f * fact },
   { 111.0f * fact, 0.0f * fact },
   { 183.0f * fact, 0.0f * fact },
   { 513.0f * fact, 0.0f * fact },
   { 585.0f * fact, 0.0f * fact },
   { 643.0f * fact, 58.0f * fact },
   { 643.0f * fact, 130.0f * fact },
   { 643.0f * fact, 580.0f * fact },
   { 535.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_117_strip[] = { {21, chr_117_part_0} };

SFG_StrokeChar chr_117 = { 695 * fact, 1, chr_117_strip };


// v.glif
static const SFG_StrokeVertex chr_118_part_0[] = {
   { 645.0f * fact, 580.0f * fact },
   { 395.0f * fact, 122.0f * fact },
   { 146.0f * fact, 580.0f * fact },
   { 21.0f * fact, 580.0f * fact },
   { 341.0f * fact, 0.0f * fact },
   { 449.0f * fact, 0.0f * fact },
   { 769.0f * fact, 580.0f * fact },
   { 645.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_118_strip[] = { {8, chr_118_part_0} };

SFG_StrokeChar chr_118 = { 790 * fact, 1, chr_118_strip };


// w.glif
static const SFG_StrokeVertex chr_119_part_0[] = {
   { 921.0f * fact, 580.0f * fact },
   { 774.0f * fact, 195.0f * fact },
   { 597.0f * fact, 580.0f * fact },
   { 474.0f * fact, 580.0f * fact },
   { 307.0f * fact, 195.0f * fact },
   { 150.0f * fact, 580.0f * fact },
   { 35.0f * fact, 580.0f * fact },
   { 263.0f * fact, 0.0f * fact },
   { 350.0f * fact, 0.0f * fact },
   { 349.0f * fact, 1.0f * fact },
   { 351.0f * fact, 0.0f * fact },
   { 535.0f * fact, 431.0f * fact },
   { 730.0f * fact, 0.0f * fact },
   { 818.0f * fact, 0.0f * fact },
   { 1036.0f * fact, 580.0f * fact },
   { 921.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_119_strip[] = { {16, chr_119_part_0} };

SFG_StrokeChar chr_119 = { 1060 * fact, 1, chr_119_strip };


// x.glif
static const SFG_StrokeVertex chr_120_part_0[] = {
   { 518.0f * fact, 580.0f * fact },
   { 346.0f * fact, 379.0f * fact },
   { 174.0f * fact, 580.0f * fact },
   { 46.0f * fact, 580.0f * fact },
   { 46.0f * fact, 565.0f * fact },
   { 275.0f * fact, 295.0f * fact },
   { 46.0f * fact, 15.0f * fact },
   { 46.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 346.0f * fact, 211.0f * fact },
   { 518.0f * fact, 0.0f * fact },
   { 646.0f * fact, 0.0f * fact },
   { 646.0f * fact, 15.0f * fact },
   { 416.0f * fact, 295.0f * fact },
   { 646.0f * fact, 565.0f * fact },
   { 646.0f * fact, 580.0f * fact },
   { 518.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_120_strip[] = { {17, chr_120_part_0} };

SFG_StrokeChar chr_120 = { 692 * fact, 1, chr_120_strip };


// y.glif
static const SFG_StrokeVertex chr_121_part_0[] = {
   { 632.0f * fact, 578.0f * fact },
   { 524.0f * fact, 578.0f * fact },
   { 524.0f * fact, 130.0f * fact },
   { 524.0f * fact, 118.0f * fact },
   { 514.0f * fact, 108.0f * fact },
   { 502.0f * fact, 108.0f * fact },
   { 172.0f * fact, 108.0f * fact },
   { 160.0f * fact, 108.0f * fact },
   { 150.0f * fact, 118.0f * fact },
   { 150.0f * fact, 130.0f * fact },
   { 150.0f * fact, 578.0f * fact },
   { 42.0f * fact, 578.0f * fact },
   { 42.0f * fact, 130.0f * fact },
   { 42.0f * fact, 58.0f * fact },
   { 100.0f * fact, 0.0f * fact },
   { 172.0f * fact, 0.0f * fact },
   { 524.0f * fact, 0.0f * fact },
   { 524.0f * fact, -100.0f * fact },
   { 524.0f * fact, -112.0f * fact },
   { 514.0f * fact, -122.0f * fact },
   { 502.0f * fact, -122.0f * fact },
   { 144.0f * fact, -122.0f * fact },
   { 144.0f * fact, -230.0f * fact },
   { 502.0f * fact, -230.0f * fact },
   { 574.0f * fact, -230.0f * fact },
   { 632.0f * fact, -172.0f * fact },
   { 632.0f * fact, -100.0f * fact },
   { 632.0f * fact, 220.0f * fact },
   { 632.0f * fact, 220.0f * fact },
   { 632.0f * fact, 221.0f * fact },
   { 632.0f * fact, 222.0f * fact },
   { 632.0f * fact, 578.0f * fact }
};

static const SFG_StrokeStrip chr_121_strip[] = { {32, chr_121_part_0} };

SFG_StrokeChar chr_121 = { 685 * fact, 1, chr_121_strip };


// z.glif
static const SFG_StrokeVertex chr_122_part_0[] = {
   { 54.0f * fact, 472.0f * fact },
   { 500.0f * fact, 472.0f * fact },
   { 54.0f * fact, 128.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 644.0f * fact, 0.0f * fact },
   { 644.0f * fact, 108.0f * fact },
   { 198.0f * fact, 108.0f * fact },
   { 644.0f * fact, 452.0f * fact },
   { 644.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, 472.0f * fact }
};

static const SFG_StrokeStrip chr_122_strip[] = { {11, chr_122_part_0} };

SFG_StrokeChar chr_122 = { 698 * fact, 1, chr_122_strip };


// braceleft.glif
static const SFG_StrokeVertex chr_123_part_0[] = {
   { 175.0f * fact, 308.0f * fact },
   { 105.0f * fact, 366.0f * fact },
   { 175.0f * fact, 421.0f * fact },
   { 175.0f * fact, 590.0f * fact },
   { 175.0f * fact, 602.0f * fact },
   { 185.0f * fact, 612.0f * fact },
   { 197.0f * fact, 612.0f * fact },
   { 239.0f * fact, 612.0f * fact },
   { 239.0f * fact, 720.0f * fact },
   { 197.0f * fact, 720.0f * fact },
   { 125.0f * fact, 720.0f * fact },
   { 67.0f * fact, 662.0f * fact },
   { 67.0f * fact, 590.0f * fact },
   { 67.0f * fact, 445.0f * fact },
   { 23.0f * fact, 420.0f * fact },
   { 23.0f * fact, 312.0f * fact },
   { 67.0f * fact, 286.0f * fact },
   { 67.0f * fact, 130.0f * fact },
   { 67.0f * fact, 58.0f * fact },
   { 125.0f * fact, 0.0f * fact },
   { 197.0f * fact, 0.0f * fact },
   { 239.0f * fact, 0.0f * fact },
   { 239.0f * fact, 108.0f * fact },
   { 197.0f * fact, 108.0f * fact },
   { 185.0f * fact, 108.0f * fact },
   { 175.0f * fact, 118.0f * fact },
   { 175.0f * fact, 130.0f * fact },
   { 175.0f * fact, 308.0f * fact }
};

static const SFG_StrokeStrip chr_123_strip[] = { {28, chr_123_part_0} };

SFG_StrokeChar chr_123 = { 289 * fact, 1, chr_123_strip };


// bar.glif
static const SFG_StrokeVertex chr_124_part_0[] = {
   { 54.0f * fact, -109.0f * fact },
   { 162.0f * fact, -109.0f * fact },
   { 162.0f * fact, 828.0f * fact },
   { 54.0f * fact, 828.0f * fact },
   { 54.0f * fact, -109.0f * fact }
};

static const SFG_StrokeStrip chr_124_strip[] = { {5, chr_124_part_0} };

SFG_StrokeChar chr_124 = { 214 * fact, 1, chr_124_strip };


// braceright.glif
static const SFG_StrokeVertex chr_125_part_0[] = {
   { 115.0f * fact, 130.0f * fact },
   { 115.0f * fact, 118.0f * fact },
   { 105.0f * fact, 108.0f * fact },
   { 93.0f * fact, 108.0f * fact },
   { 51.0f * fact, 108.0f * fact },
   { 51.0f * fact, 0.0f * fact },
   { 93.0f * fact, 0.0f * fact },
   { 165.0f * fact, 0.0f * fact },
   { 223.0f * fact, 58.0f * fact },
   { 223.0f * fact, 130.0f * fact },
   { 223.0f * fact, 286.0f * fact },
   { 267.0f * fact, 312.0f * fact },
   { 267.0f * fact, 420.0f * fact },
   { 223.0f * fact, 445.0f * fact },
   { 223.0f * fact, 590.0f * fact },
   { 223.0f * fact, 662.0f * fact },
   { 165.0f * fact, 720.0f * fact },
   { 93.0f * fact, 720.0f * fact },
   { 51.0f * fact, 720.0f * fact },
   { 51.0f * fact, 612.0f * fact },
   { 93.0f * fact, 612.0f * fact },
   { 105.0f * fact, 612.0f * fact },
   { 115.0f * fact, 602.0f * fact },
   { 115.0f * fact, 590.0f * fact },
   { 115.0f * fact, 421.0f * fact },
   { 185.0f * fact, 366.0f * fact },
   { 115.0f * fact, 308.0f * fact },
   { 115.0f * fact, 130.0f * fact }
};

static const SFG_StrokeStrip chr_125_strip[] = { {28, chr_125_part_0} };

SFG_StrokeChar chr_125 = { 289 * fact, 1, chr_125_strip };


// asciitilde.glif
static const SFG_StrokeVertex chr_126_part_0[] = {
   { 224.0f * fact, 306.0f * fact },
   { 172.0f * fact, 366.0f * fact },
   { 98.0f * fact, 366.0f * fact },
   { 68.0f * fact, 366.0f * fact },
   { 43.0f * fact, 360.0f * fact },
   { 24.0f * fact, 352.0f * fact },
   { 24.0f * fact, 277.0f * fact },
   { 37.0f * fact, 298.0f * fact },
   { 76.0f * fact, 300.0f * fact },
   { 98.0f * fact, 300.0f * fact },
   { 174.0f * fact, 300.0f * fact },
   { 227.0f * fact, 232.0f * fact },
   { 298.0f * fact, 232.0f * fact },
   { 329.0f * fact, 232.0f * fact },
   { 355.0f * fact, 241.0f * fact },
   { 376.0f * fact, 250.0f * fact },
   { 376.0f * fact, 324.0f * fact },
   { 352.0f * fact, 308.0f * fact },
   { 327.0f * fact, 298.0f * fact },
   { 298.0f * fact, 298.0f * fact },
   { 224.0f * fact, 306.0f * fact }
};

static const SFG_StrokeStrip chr_126_strip[] = { {21, chr_126_part_0} };

SFG_StrokeChar chr_126 = { 404 * fact, 1, chr_126_strip };



static const SFG_StrokeChar *orbitronMediumChars[] = { 
   0,	0,	0,	0,	0,	0,	0,	0, 
   0,	0,	0,	0,	0,	0,	0,	0, 
   0,	0,	0,	0,	0,	0,	0,	0, 
   0,	0,	0,	0,	0,	0,	0,	0, 
   &chr_32,	&chr_33,	&chr_34,	&chr_35,	&chr_36,	&chr_37,	&chr_38,	&chr_39, 
   &chr_40,	&chr_41,	&chr_42,	&chr_43,	&chr_44,	&chr_45,	&chr_46,	&chr_47, 
   &chr_48,	&chr_49,	&chr_50,	&chr_51,	&chr_52,	&chr_53,	&chr_54,	&chr_55, 
   &chr_56,	&chr_57,	&chr_58,	&chr_59,	&chr_60,	&chr_61,	&chr_62,	&chr_63, 
   &chr_64,	&chr_65,	&chr_66,	&chr_67,	&chr_68,	&chr_69,	&chr_70,	&chr_71, 
   &chr_72,	&chr_73,	&chr_74,	&chr_75,	&chr_76,	&chr_77,	&chr_78,	&chr_79, 
   &chr_80,	&chr_81,	&chr_82,	&chr_83,	&chr_84,	&chr_85,	&chr_86,	&chr_87, 
   &chr_88,	&chr_89,	&chr_90,	&chr_91,	&chr_92,	&chr_93,	0,	&chr_95, 
   &chr_96,	&chr_97,	&chr_98,	&chr_99,	&chr_100,	&chr_101,	&chr_102,	&chr_103, 
   &chr_104,	&chr_105,	&chr_106,	&chr_107,	&chr_108,	&chr_109,	&chr_110,	&chr_111, 
   &chr_112,	&chr_113,	&chr_114,	&chr_115,	&chr_116,	&chr_117,	&chr_118,	&chr_119, 
   &chr_120,	&chr_121,	&chr_122,	&chr_123,	&chr_124,	&chr_125,	&chr_126,	0
};


const SFG_StrokeFont fgStrokeOrbitronMed = { (char*)"Orbitron-Medium", 128, 10, orbitronMediumChars };
