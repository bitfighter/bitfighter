#ifdef TNL_OS_MOBILE
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif

#include "freeglut_stroke.h"

F32 fact = 0.10f;

// space
static const SFG_StrokeStrip chr_ol_32_strip[] = { { 0, NULL} };

SFG_StrokeChar chr_ol_32 = { 290 * fact, 0, chr_ol_32_strip };


static const SFG_StrokeVertex chr_ol_33_part_0[] = {
   { 140.0f * fact, 0.0f * fact },
   { 140.0f * fact, 82.0f * fact },
   { 58.0f * fact, 82.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 140.0f * fact, 0.0f * fact }
};

// exclam.glif
static const SFG_StrokeVertex chr_ol_33_part_1[] = {
   { 58.0f * fact, 203.0f * fact },
   { 140.0f * fact, 203.0f * fact },
   { 140.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 203.0f * fact }
};

static const SFG_StrokeStrip chr_ol_33_strip[] = { {5, chr_ol_33_part_0}, {5, chr_ol_33_part_1} };

SFG_StrokeChar chr_ol_33 = { 220 * fact, 2, chr_ol_33_strip };


// quotedbl.glif
static const SFG_StrokeVertex chr_ol_34_part_0[] = {
   { 141.0f * fact, 720.0f * fact },
   { 59.0f * fact, 720.0f * fact },
   { 59.0f * fact, 580.0f * fact },
   { 141.0f * fact, 580.0f * fact },
   { 141.0f * fact, 720.0f * fact }
};

// quotedbl.glif
static const SFG_StrokeVertex chr_ol_34_part_1[] = {
   { 296.0f * fact, 720.0f * fact },
   { 214.0f * fact, 720.0f * fact },
   { 214.0f * fact, 580.0f * fact },
   { 296.0f * fact, 580.0f * fact },
   { 296.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_34_strip[] = { {5, chr_ol_34_part_0}, {5, chr_ol_34_part_1} };

SFG_StrokeChar chr_ol_34 = { 372 * fact, 2, chr_ol_34_strip };


// numbersign.glif
static const SFG_StrokeVertex chr_ol_35_part_0[] = {
   { 773.0f * fact, 555.0f * fact },
   { 677.0f * fact, 555.0f * fact },
   { 734.0f * fact, 719.0f * fact },
   { 653.0f * fact, 719.0f * fact },
   { 597.0f * fact, 555.0f * fact },
   { 336.0f * fact, 555.0f * fact },
   { 393.0f * fact, 719.0f * fact },
   { 312.0f * fact, 719.0f * fact },
   { 256.0f * fact, 555.0f * fact },
   { 64.0f * fact, 555.0f * fact },
   { 64.0f * fact, 473.0f * fact },
   { 230.0f * fact, 473.0f * fact },
   { 159.0f * fact, 253.0f * fact },
   { 32.0f * fact, 253.0f * fact },
   { 32.0f * fact, 171.0f * fact },
   { 133.0f * fact, 171.0f * fact },
   { 79.0f * fact, 0.0f * fact },
   { 161.0f * fact, 0.0f * fact },
   { 178.0f * fact, 55.0f * fact },
   { 198.0f * fact, 117.0f * fact },
   { 214.0f * fact, 171.0f * fact },
   { 474.0f * fact, 171.0f * fact },
   { 420.0f * fact, 0.0f * fact },
   { 502.0f * fact, 0.0f * fact },
   { 519.0f * fact, 55.0f * fact },
   { 539.0f * fact, 117.0f * fact },
   { 555.0f * fact, 171.0f * fact },
   { 740.0f * fact, 171.0f * fact },
   { 740.0f * fact, 253.0f * fact },
   { 581.0f * fact, 253.0f * fact },
   { 652.0f * fact, 473.0f * fact },
   { 773.0f * fact, 473.0f * fact },
   { 773.0f * fact, 555.0f * fact }
};

// numbersign.glif
static const SFG_StrokeVertex chr_ol_35_part_1[] = {
   { 241.0f * fact, 253.0f * fact },
   { 311.0f * fact, 473.0f * fact },
   { 571.0f * fact, 473.0f * fact },
   { 500.0f * fact, 253.0f * fact },
   { 241.0f * fact, 253.0f * fact }
};

static const SFG_StrokeStrip chr_ol_35_strip[] = { {33, chr_ol_35_part_0}, {5, chr_ol_35_part_1} };

SFG_StrokeChar chr_ol_35 = { 797 * fact, 2, chr_ol_35_strip };


// dollar.glif
static const SFG_StrokeVertex chr_ol_36_part_0[] = {
   { 762.0f * fact, 599.0f * fact },
   { 762.0f * fact, 667.0f * fact },
   { 708.0f * fact, 721.0f * fact },
   { 640.0f * fact, 721.0f * fact },
   { 439.0f * fact, 721.0f * fact },
   { 439.0f * fact, 835.0f * fact },
   { 357.0f * fact, 835.0f * fact },
   { 357.0f * fact, 721.0f * fact },
   { 155.0f * fact, 721.0f * fact },
   { 88.0f * fact, 721.0f * fact },
   { 34.0f * fact, 667.0f * fact },
   { 34.0f * fact, 599.0f * fact },
   { 34.0f * fact, 441.0f * fact },
   { 34.0f * fact, 373.0f * fact },
   { 88.0f * fact, 320.0f * fact },
   { 155.0f * fact, 320.0f * fact },
   { 357.0f * fact, 320.0f * fact },
   { 357.0f * fact, 82.0f * fact },
   { 155.0f * fact, 82.0f * fact },
   { 134.0f * fact, 82.0f * fact },
   { 116.0f * fact, 100.0f * fact },
   { 116.0f * fact, 121.0f * fact },
   { 116.0f * fact, 150.0f * fact },
   { 34.0f * fact, 150.0f * fact },
   { 34.0f * fact, 121.0f * fact },
   { 34.0f * fact, 54.0f * fact },
   { 88.0f * fact, 0.0f * fact },
   { 155.0f * fact, 0.0f * fact },
   { 357.0f * fact, 0.0f * fact },
   { 357.0f * fact, -114.0f * fact },
   { 439.0f * fact, -114.0f * fact },
   { 439.0f * fact, 0.0f * fact },
   { 640.0f * fact, 0.0f * fact },
   { 708.0f * fact, 0.0f * fact },
   { 762.0f * fact, 54.0f * fact },
   { 762.0f * fact, 121.0f * fact },
   { 762.0f * fact, 281.0f * fact },
   { 762.0f * fact, 348.0f * fact },
   { 708.0f * fact, 402.0f * fact },
   { 640.0f * fact, 402.0f * fact },
   { 439.0f * fact, 402.0f * fact },
   { 439.0f * fact, 638.0f * fact },
   { 640.0f * fact, 638.0f * fact },
   { 661.0f * fact, 638.0f * fact },
   { 679.0f * fact, 620.0f * fact },
   { 679.0f * fact, 599.0f * fact },
   { 679.0f * fact, 571.0f * fact },
   { 762.0f * fact, 571.0f * fact },
   { 762.0f * fact, 599.0f * fact }
};

// dollar.glif
static const SFG_StrokeVertex chr_ol_36_part_1[] = {
   { 640.0f * fact, 320.0f * fact },
   { 661.0f * fact, 320.0f * fact },
   { 679.0f * fact, 302.0f * fact },
   { 679.0f * fact, 281.0f * fact },
   { 679.0f * fact, 121.0f * fact },
   { 679.0f * fact, 100.0f * fact },
   { 661.0f * fact, 82.0f * fact },
   { 640.0f * fact, 82.0f * fact },
   { 439.0f * fact, 82.0f * fact },
   { 439.0f * fact, 320.0f * fact },
   { 640.0f * fact, 320.0f * fact }
};

// dollar.glif
static const SFG_StrokeVertex chr_ol_36_part_2[] = {
   { 357.0f * fact, 402.0f * fact },
   { 155.0f * fact, 402.0f * fact },
   { 134.0f * fact, 402.0f * fact },
   { 116.0f * fact, 420.0f * fact },
   { 116.0f * fact, 441.0f * fact },
   { 116.0f * fact, 599.0f * fact },
   { 116.0f * fact, 620.0f * fact },
   { 134.0f * fact, 638.0f * fact },
   { 155.0f * fact, 638.0f * fact },
   { 357.0f * fact, 638.0f * fact },
   { 357.0f * fact, 402.0f * fact }
};

static const SFG_StrokeStrip chr_ol_36_strip[] = { {49, chr_ol_36_part_0}, {11, chr_ol_36_part_1}, {11, chr_ol_36_part_2} };

SFG_StrokeChar chr_ol_36 = { 788 * fact, 3, chr_ol_36_strip };


// percent.glif
static const SFG_StrokeVertex chr_ol_37_part_0[] = {
   { 137.0f * fact, 0.0f * fact },
   { 865.0f * fact, 614.0f * fact },
   { 865.0f * fact, 721.0f * fact },
   { 137.0f * fact, 108.0f * fact },
   { 137.0f * fact, 0.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_ol_37_part_1[] = {
   { 170.0f * fact, 720.0f * fact },
   { 102.0f * fact, 720.0f * fact },
   { 48.0f * fact, 666.0f * fact },
   { 48.0f * fact, 598.0f * fact },
   { 48.0f * fact, 514.0f * fact },
   { 48.0f * fact, 447.0f * fact },
   { 102.0f * fact, 393.0f * fact },
   { 170.0f * fact, 393.0f * fact },
   { 264.0f * fact, 393.0f * fact },
   { 332.0f * fact, 393.0f * fact },
   { 386.0f * fact, 447.0f * fact },
   { 386.0f * fact, 514.0f * fact },
   { 386.0f * fact, 598.0f * fact },
   { 386.0f * fact, 666.0f * fact },
   { 332.0f * fact, 720.0f * fact },
   { 264.0f * fact, 720.0f * fact },
   { 170.0f * fact, 720.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_ol_37_part_2[] = {
   { 148.0f * fact, 454.0f * fact },
   { 127.0f * fact, 454.0f * fact },
   { 109.0f * fact, 472.0f * fact },
   { 109.0f * fact, 493.0f * fact },
   { 109.0f * fact, 619.0f * fact },
   { 109.0f * fact, 641.0f * fact },
   { 127.0f * fact, 658.0f * fact },
   { 148.0f * fact, 658.0f * fact },
   { 286.0f * fact, 658.0f * fact },
   { 307.0f * fact, 658.0f * fact },
   { 325.0f * fact, 641.0f * fact },
   { 325.0f * fact, 619.0f * fact },
   { 325.0f * fact, 493.0f * fact },
   { 325.0f * fact, 472.0f * fact },
   { 307.0f * fact, 454.0f * fact },
   { 286.0f * fact, 454.0f * fact },
   { 148.0f * fact, 454.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_ol_37_part_3[] = {
   { 716.0f * fact, 326.0f * fact },
   { 648.0f * fact, 326.0f * fact },
   { 595.0f * fact, 272.0f * fact },
   { 595.0f * fact, 205.0f * fact },
   { 595.0f * fact, 120.0f * fact },
   { 595.0f * fact, 53.0f * fact },
   { 648.0f * fact, -1.0f * fact },
   { 716.0f * fact, -1.0f * fact },
   { 811.0f * fact, -1.0f * fact },
   { 878.0f * fact, -1.0f * fact },
   { 932.0f * fact, 53.0f * fact },
   { 932.0f * fact, 120.0f * fact },
   { 932.0f * fact, 205.0f * fact },
   { 932.0f * fact, 272.0f * fact },
   { 878.0f * fact, 326.0f * fact },
   { 811.0f * fact, 326.0f * fact },
   { 716.0f * fact, 326.0f * fact }
};

// percent.glif
static const SFG_StrokeVertex chr_ol_37_part_4[] = {
   { 695.0f * fact, 60.0f * fact },
   { 674.0f * fact, 60.0f * fact },
   { 656.0f * fact, 78.0f * fact },
   { 656.0f * fact, 99.0f * fact },
   { 656.0f * fact, 226.0f * fact },
   { 656.0f * fact, 247.0f * fact },
   { 674.0f * fact, 265.0f * fact },
   { 695.0f * fact, 265.0f * fact },
   { 832.0f * fact, 265.0f * fact },
   { 853.0f * fact, 265.0f * fact },
   { 871.0f * fact, 247.0f * fact },
   { 871.0f * fact, 226.0f * fact },
   { 871.0f * fact, 99.0f * fact },
   { 871.0f * fact, 78.0f * fact },
   { 853.0f * fact, 60.0f * fact },
   { 832.0f * fact, 60.0f * fact },
   { 695.0f * fact, 60.0f * fact }
};

static const SFG_StrokeStrip chr_ol_37_strip[] = { {5, chr_ol_37_part_0}, {17, chr_ol_37_part_1}, {17, chr_ol_37_part_2}, {17, chr_ol_37_part_3}, {17, chr_ol_37_part_4} };

SFG_StrokeChar chr_ol_37 = { 966 * fact, 5, chr_ol_37_strip };


// ampersand.glif
static const SFG_StrokeVertex chr_ol_38_part_0[] = {
   { 780.0f * fact, 167.0f * fact },
   { 780.0f * fact, 333.0f * fact },
   { 698.0f * fact, 333.0f * fact },
   { 698.0f * fact, 194.0f * fact },
   { 177.0f * fact, 457.0f * fact },
   { 177.0f * fact, 596.0f * fact },
   { 177.0f * fact, 617.0f * fact },
   { 195.0f * fact, 635.0f * fact },
   { 216.0f * fact, 635.0f * fact },
   { 627.0f * fact, 635.0f * fact },
   { 649.0f * fact, 635.0f * fact },
   { 666.0f * fact, 617.0f * fact },
   { 666.0f * fact, 596.0f * fact },
   { 666.0f * fact, 562.0f * fact },
   { 749.0f * fact, 562.0f * fact },
   { 749.0f * fact, 617.0f * fact },
   { 734.0f * fact, 669.0f * fact },
   { 683.0f * fact, 719.0f * fact },
   { 627.0f * fact, 719.0f * fact },
   { 216.0f * fact, 719.0f * fact },
   { 149.0f * fact, 719.0f * fact },
   { 95.0f * fact, 665.0f * fact },
   { 95.0f * fact, 597.0f * fact },
   { 95.0f * fact, 497.0f * fact },
   { 95.0f * fact, 470.0f * fact },
   { 104.0f * fact, 437.0f * fact },
   { 152.0f * fact, 401.0f * fact },
   { 86.0f * fact, 401.0f * fact },
   { 53.0f * fact, 360.0f * fact },
   { 53.0f * fact, 336.0f * fact },
   { 53.0f * fact, 121.0f * fact },
   { 53.0f * fact, 54.0f * fact },
   { 107.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 659.0f * fact, 0.0f * fact },
   { 710.0f * fact, 0.0f * fact },
   { 755.0f * fact, 38.0f * fact },
   { 777.0f * fact, 93.0f * fact },
   { 914.0f * fact, 16.0f * fact },
   { 914.0f * fact, 88.0f * fact },
   { 780.0f * fact, 167.0f * fact }
};

// ampersand.glif
static const SFG_StrokeVertex chr_ol_38_part_1[] = {
   { 174.0f * fact, 82.0f * fact },
   { 153.0f * fact, 82.0f * fact },
   { 135.0f * fact, 100.0f * fact },
   { 135.0f * fact, 121.0f * fact },
   { 135.0f * fact, 336.0f * fact },
   { 135.0f * fact, 357.0f * fact },
   { 153.0f * fact, 375.0f * fact },
   { 174.0f * fact, 375.0f * fact },
   { 189.0f * fact, 375.0f * fact },
   { 699.0f * fact, 118.0f * fact },
   { 690.0f * fact, 97.0f * fact },
   { 673.0f * fact, 82.0f * fact },
   { 659.0f * fact, 82.0f * fact },
   { 174.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_38_strip[] = { {41, chr_ol_38_part_0}, {14, chr_ol_38_part_1} };

SFG_StrokeChar chr_ol_38 = { 938 * fact, 2, chr_ol_38_strip };


// quotesingle.glif
static const SFG_StrokeVertex chr_ol_39_part_0[] = {
   { 141.0f * fact, 720.0f * fact },
   { 59.0f * fact, 720.0f * fact },
   { 59.0f * fact, 580.0f * fact },
   { 141.0f * fact, 580.0f * fact },
   { 141.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_39_strip[] = { {5, chr_ol_39_part_0} };

SFG_StrokeChar chr_ol_39 = { 224 * fact, 1, chr_ol_39_strip };


// parenleft.glif
static const SFG_StrokeVertex chr_ol_40_part_0[] = {
   { 173.0f * fact, 82.0f * fact },
   { 152.0f * fact, 82.0f * fact },
   { 134.0f * fact, 100.0f * fact },
   { 134.0f * fact, 121.0f * fact },
   { 134.0f * fact, 598.0f * fact },
   { 134.0f * fact, 619.0f * fact },
   { 152.0f * fact, 637.0f * fact },
   { 173.0f * fact, 637.0f * fact },
   { 202.0f * fact, 637.0f * fact },
   { 202.0f * fact, 720.0f * fact },
   { 173.0f * fact, 720.0f * fact },
   { 106.0f * fact, 720.0f * fact },
   { 52.0f * fact, 666.0f * fact },
   { 52.0f * fact, 598.0f * fact },
   { 52.0f * fact, 121.0f * fact },
   { 52.0f * fact, 54.0f * fact },
   { 106.0f * fact, 0.0f * fact },
   { 173.0f * fact, 0.0f * fact },
   { 202.0f * fact, 0.0f * fact },
   { 202.0f * fact, 82.0f * fact },
   { 173.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_40_strip[] = { {21, chr_ol_40_part_0} };

SFG_StrokeChar chr_ol_40 = { 277 * fact, 1, chr_ol_40_strip };


// parenright.glif
static const SFG_StrokeVertex chr_ol_41_part_0[] = {
   { 56.0f * fact, 0.0f * fact },
   { 84.0f * fact, 0.0f * fact },
   { 151.0f * fact, 0.0f * fact },
   { 206.0f * fact, 54.0f * fact },
   { 206.0f * fact, 121.0f * fact },
   { 206.0f * fact, 598.0f * fact },
   { 206.0f * fact, 666.0f * fact },
   { 151.0f * fact, 720.0f * fact },
   { 84.0f * fact, 720.0f * fact },
   { 56.0f * fact, 720.0f * fact },
   { 56.0f * fact, 637.0f * fact },
   { 84.0f * fact, 637.0f * fact },
   { 106.0f * fact, 637.0f * fact },
   { 123.0f * fact, 619.0f * fact },
   { 123.0f * fact, 598.0f * fact },
   { 123.0f * fact, 121.0f * fact },
   { 123.0f * fact, 100.0f * fact },
   { 106.0f * fact, 82.0f * fact },
   { 84.0f * fact, 82.0f * fact },
   { 56.0f * fact, 82.0f * fact },
   { 56.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_ol_41_strip[] = { {21, chr_ol_41_part_0} };

SFG_StrokeChar chr_ol_41 = { 278 * fact, 1, chr_ol_41_strip };


// asterisk.glif
static const SFG_StrokeVertex chr_ol_42_part_0[] = {
   { 423.0f * fact, 614.0f * fact },
   { 278.0f * fact, 566.0f * fact },
   { 278.0f * fact, 719.0f * fact },
   { 196.0f * fact, 719.0f * fact },
   { 196.0f * fact, 565.0f * fact },
   { 49.0f * fact, 614.0f * fact },
   { 25.0f * fact, 535.0f * fact },
   { 170.0f * fact, 488.0f * fact },
   { 80.0f * fact, 362.0f * fact },
   { 105.0f * fact, 344.0f * fact },
   { 120.0f * fact, 335.0f * fact },
   { 147.0f * fact, 315.0f * fact },
   { 237.0f * fact, 438.0f * fact },
   { 328.0f * fact, 315.0f * fact },
   { 353.0f * fact, 332.0f * fact },
   { 367.0f * fact, 344.0f * fact },
   { 394.0f * fact, 363.0f * fact },
   { 366.0f * fact, 401.0f * fact },
   { 328.0f * fact, 453.0f * fact },
   { 304.0f * fact, 488.0f * fact },
   { 450.0f * fact, 535.0f * fact },
   { 423.0f * fact, 614.0f * fact }
};

static const SFG_StrokeStrip chr_ol_42_strip[] = { {22, chr_ol_42_part_0} };

SFG_StrokeChar chr_ol_42 = { 491 * fact, 1, chr_ol_42_strip };


// plus.glif
static const SFG_StrokeVertex chr_ol_43_part_0[] = {
   { 169.0f * fact, 490.0f * fact },
   { 169.0f * fact, 337.0f * fact },
   { 17.0f * fact, 337.0f * fact },
   { 17.0f * fact, 255.0f * fact },
   { 169.0f * fact, 255.0f * fact },
   { 169.0f * fact, 100.0f * fact },
   { 251.0f * fact, 100.0f * fact },
   { 251.0f * fact, 255.0f * fact },
   { 407.0f * fact, 255.0f * fact },
   { 407.0f * fact, 337.0f * fact },
   { 251.0f * fact, 337.0f * fact },
   { 251.0f * fact, 490.0f * fact },
   { 169.0f * fact, 490.0f * fact }
};

static const SFG_StrokeStrip chr_ol_43_strip[] = { {13, chr_ol_43_part_0} };

SFG_StrokeChar chr_ol_43 = { 433 * fact, 1, chr_ol_43_strip };


// comma.glif
static const SFG_StrokeVertex chr_ol_44_part_0[] = {
   { 54.0f * fact, 82.0f * fact },
   { 54.0f * fact, -131.0f * fact },
   { 102.0f * fact, -115.0f * fact },
   { 136.0f * fact, -70.0f * fact },
   { 136.0f * fact, -16.0f * fact },
   { 136.0f * fact, 82.0f * fact },
   { 54.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_44_strip[] = { {7, chr_ol_44_part_0} };

SFG_StrokeChar chr_ol_44 = { 193 * fact, 1, chr_ol_44_strip };


// hyphen.glif
static const SFG_StrokeVertex chr_ol_45_part_0[] = {
   { 449.0f * fact, 337.0f * fact },
   { 59.0f * fact, 337.0f * fact },
   { 59.0f * fact, 255.0f * fact },
   { 449.0f * fact, 255.0f * fact },
   { 449.0f * fact, 337.0f * fact }
};

static const SFG_StrokeStrip chr_ol_45_strip[] = { {5, chr_ol_45_part_0} };

SFG_StrokeChar chr_ol_45 = { 517 * fact, 1, chr_ol_45_strip };


// period.glif
static const SFG_StrokeVertex chr_ol_46_part_0[] = {
   { 136.0f * fact, 82.0f * fact },
   { 54.0f * fact, 82.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 136.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_46_strip[] = { {5, chr_ol_46_part_0} };

SFG_StrokeChar chr_ol_46 = { 214 * fact, 1, chr_ol_46_strip };


// slash.glif
static const SFG_StrokeVertex chr_ol_47_part_0[] = {
   { 6.0f * fact, 106.0f * fact },
   { 6.0f * fact, 0.0f * fact },
   { 512.0f * fact, 616.0f * fact },
   { 512.0f * fact, 720.0f * fact },
   { 6.0f * fact, 106.0f * fact }
};

static const SFG_StrokeStrip chr_ol_47_strip[] = { {5, chr_ol_47_part_0} };

SFG_StrokeChar chr_ol_47 = { 521 * fact, 1, chr_ol_47_strip };


// zero.glif
static const SFG_StrokeVertex chr_ol_48_part_0[] = {
   { 178.0f * fact, 720.0f * fact },
   { 110.0f * fact, 720.0f * fact },
   { 57.0f * fact, 666.0f * fact },
   { 57.0f * fact, 598.0f * fact },
   { 57.0f * fact, 121.0f * fact },
   { 57.0f * fact, 54.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 178.0f * fact, 0.0f * fact },
   { 663.0f * fact, 0.0f * fact },
   { 730.0f * fact, 0.0f * fact },
   { 784.0f * fact, 54.0f * fact },
   { 784.0f * fact, 121.0f * fact },
   { 784.0f * fact, 598.0f * fact },
   { 784.0f * fact, 666.0f * fact },
   { 730.0f * fact, 720.0f * fact },
   { 663.0f * fact, 720.0f * fact },
   { 178.0f * fact, 720.0f * fact }
};

// zero.glif
static const SFG_StrokeVertex chr_ol_48_part_1[] = {
   { 691.0f * fact, 637.0f * fact },
   { 139.0f * fact, 181.0f * fact },
   { 139.0f * fact, 598.0f * fact },
   { 139.0f * fact, 619.0f * fact },
   { 157.0f * fact, 637.0f * fact },
   { 178.0f * fact, 637.0f * fact },
   { 691.0f * fact, 637.0f * fact }
};

// zero.glif
static const SFG_StrokeVertex chr_ol_48_part_2[] = {
   { 149.0f * fact, 82.0f * fact },
   { 702.0f * fact, 538.0f * fact },
   { 702.0f * fact, 121.0f * fact },
   { 702.0f * fact, 100.0f * fact },
   { 684.0f * fact, 82.0f * fact },
   { 663.0f * fact, 82.0f * fact },
   { 149.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_48_strip[] = { {17, chr_ol_48_part_0}, {7, chr_ol_48_part_1}, {7, chr_ol_48_part_2} };

SFG_StrokeChar chr_ol_48 = { 834 * fact, 3, chr_ol_48_strip };


// one.glif
static const SFG_StrokeVertex chr_ol_49_part_0[] = {
   { 1.0f * fact, 495.0f * fact },
   { 108.0f * fact, 495.0f * fact },
   { 137.0f * fact, 531.0f * fact },
   { 192.0f * fact, 594.0f * fact },
   { 219.0f * fact, 628.0f * fact },
   { 219.0f * fact, 0.0f * fact },
   { 302.0f * fact, 0.0f * fact },
   { 302.0f * fact, 720.0f * fact },
   { 190.0f * fact, 720.0f * fact },
   { 1.0f * fact, 495.0f * fact }
};

static const SFG_StrokeStrip chr_ol_49_strip[] = { {10, chr_ol_49_part_0} };

SFG_StrokeChar chr_ol_49 = { 391 * fact, 1, chr_ol_49_strip };


// two.glif
static const SFG_StrokeVertex chr_ol_50_part_0[] = {
   { 178.0f * fact, 721.0f * fact },
   { 110.0f * fact, 721.0f * fact },
   { 57.0f * fact, 667.0f * fact },
   { 57.0f * fact, 599.0f * fact },
   { 57.0f * fact, 571.0f * fact },
   { 139.0f * fact, 571.0f * fact },
   { 139.0f * fact, 599.0f * fact },
   { 139.0f * fact, 620.0f * fact },
   { 157.0f * fact, 638.0f * fact },
   { 178.0f * fact, 638.0f * fact },
   { 663.0f * fact, 638.0f * fact },
   { 684.0f * fact, 638.0f * fact },
   { 702.0f * fact, 620.0f * fact },
   { 702.0f * fact, 599.0f * fact },
   { 702.0f * fact, 430.0f * fact },
   { 702.0f * fact, 409.0f * fact },
   { 684.0f * fact, 391.0f * fact },
   { 663.0f * fact, 391.0f * fact },
   { 178.0f * fact, 391.0f * fact },
   { 110.0f * fact, 391.0f * fact },
   { 57.0f * fact, 338.0f * fact },
   { 57.0f * fact, 270.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 784.0f * fact, 0.0f * fact },
   { 784.0f * fact, 82.0f * fact },
   { 178.0f * fact, 82.0f * fact },
   { 157.0f * fact, 82.0f * fact },
   { 139.0f * fact, 100.0f * fact },
   { 139.0f * fact, 121.0f * fact },
   { 139.0f * fact, 270.0f * fact },
   { 139.0f * fact, 291.0f * fact },
   { 157.0f * fact, 309.0f * fact },
   { 178.0f * fact, 309.0f * fact },
   { 663.0f * fact, 309.0f * fact },
   { 730.0f * fact, 309.0f * fact },
   { 784.0f * fact, 363.0f * fact },
   { 784.0f * fact, 430.0f * fact },
   { 784.0f * fact, 599.0f * fact },
   { 784.0f * fact, 667.0f * fact },
   { 730.0f * fact, 721.0f * fact },
   { 663.0f * fact, 721.0f * fact },
   { 178.0f * fact, 721.0f * fact }
};

static const SFG_StrokeStrip chr_ol_50_strip[] = { {42, chr_ol_50_part_0} };

SFG_StrokeChar chr_ol_50 = { 830 * fact, 1, chr_ol_50_strip };


// three.glif
static const SFG_StrokeVertex chr_ol_51_part_0[] = {
   { 737.0f * fact, 396.0f * fact },
   { 744.0f * fact, 411.0f * fact },
   { 749.0f * fact, 429.0f * fact },
   { 749.0f * fact, 448.0f * fact },
   { 749.0f * fact, 599.0f * fact },
   { 749.0f * fact, 667.0f * fact },
   { 696.0f * fact, 721.0f * fact },
   { 628.0f * fact, 721.0f * fact },
   { 175.0f * fact, 721.0f * fact },
   { 107.0f * fact, 721.0f * fact },
   { 53.0f * fact, 667.0f * fact },
   { 53.0f * fact, 599.0f * fact },
   { 53.0f * fact, 573.0f * fact },
   { 136.0f * fact, 573.0f * fact },
   { 136.0f * fact, 599.0f * fact },
   { 136.0f * fact, 620.0f * fact },
   { 154.0f * fact, 638.0f * fact },
   { 175.0f * fact, 638.0f * fact },
   { 628.0f * fact, 638.0f * fact },
   { 649.0f * fact, 638.0f * fact },
   { 667.0f * fact, 620.0f * fact },
   { 667.0f * fact, 599.0f * fact },
   { 667.0f * fact, 448.0f * fact },
   { 667.0f * fact, 427.0f * fact },
   { 649.0f * fact, 409.0f * fact },
   { 628.0f * fact, 409.0f * fact },
   { 191.0f * fact, 409.0f * fact },
   { 191.0f * fact, 327.0f * fact },
   { 660.0f * fact, 327.0f * fact },
   { 681.0f * fact, 327.0f * fact },
   { 699.0f * fact, 309.0f * fact },
   { 699.0f * fact, 288.0f * fact },
   { 699.0f * fact, 121.0f * fact },
   { 699.0f * fact, 100.0f * fact },
   { 681.0f * fact, 82.0f * fact },
   { 660.0f * fact, 82.0f * fact },
   { 175.0f * fact, 82.0f * fact },
   { 154.0f * fact, 82.0f * fact },
   { 136.0f * fact, 100.0f * fact },
   { 136.0f * fact, 121.0f * fact },
   { 136.0f * fact, 137.0f * fact },
   { 53.0f * fact, 137.0f * fact },
   { 53.0f * fact, 121.0f * fact },
   { 53.0f * fact, 54.0f * fact },
   { 107.0f * fact, 0.0f * fact },
   { 175.0f * fact, 0.0f * fact },
   { 660.0f * fact, 0.0f * fact },
   { 727.0f * fact, 0.0f * fact },
   { 781.0f * fact, 54.0f * fact },
   { 781.0f * fact, 121.0f * fact },
   { 781.0f * fact, 288.0f * fact },
   { 781.0f * fact, 323.0f * fact },
   { 765.0f * fact, 356.0f * fact },
   { 741.0f * fact, 378.0f * fact },
   { 737.0f * fact, 396.0f * fact }
};

static const SFG_StrokeStrip chr_ol_51_strip[] = { {55, chr_ol_51_part_0} };

SFG_StrokeChar chr_ol_51 = { 826 * fact, 1, chr_ol_51_strip };


// four.glif
static const SFG_StrokeVertex chr_ol_52_part_0[] = {
   { 590.0f * fact, 268.0f * fact },
   { 590.0f * fact, 721.0f * fact },
   { 498.0f * fact, 721.0f * fact },
   { 334.0f * fact, 571.0f * fact },
   { 171.0f * fact, 427.0f * fact },
   { 6.0f * fact, 278.0f * fact },
   { 6.0f * fact, 186.0f * fact },
   { 508.0f * fact, 186.0f * fact },
   { 508.0f * fact, 0.0f * fact },
   { 590.0f * fact, 0.0f * fact },
   { 590.0f * fact, 186.0f * fact },
   { 702.0f * fact, 186.0f * fact },
   { 702.0f * fact, 268.0f * fact },
   { 590.0f * fact, 268.0f * fact }
};

// four.glif
static const SFG_StrokeVertex chr_ol_52_part_1[] = {
   { 508.0f * fact, 590.0f * fact },
   { 508.0f * fact, 268.0f * fact },
   { 111.0f * fact, 268.0f * fact },
   { 508.0f * fact, 590.0f * fact }
};

static const SFG_StrokeStrip chr_ol_52_strip[] = { {14, chr_ol_52_part_0}, {4, chr_ol_52_part_1} };

SFG_StrokeChar chr_ol_52 = { 730 * fact, 2, chr_ol_52_strip };


// five.glif
static const SFG_StrokeVertex chr_ol_53_part_0[] = {
   { 139.0f * fact, 599.0f * fact },
   { 139.0f * fact, 620.0f * fact },
   { 157.0f * fact, 638.0f * fact },
   { 178.0f * fact, 638.0f * fact },
   { 784.0f * fact, 638.0f * fact },
   { 784.0f * fact, 721.0f * fact },
   { 57.0f * fact, 721.0f * fact },
   { 57.0f * fact, 330.0f * fact },
   { 663.0f * fact, 330.0f * fact },
   { 684.0f * fact, 330.0f * fact },
   { 702.0f * fact, 312.0f * fact },
   { 702.0f * fact, 291.0f * fact },
   { 702.0f * fact, 121.0f * fact },
   { 702.0f * fact, 100.0f * fact },
   { 684.0f * fact, 82.0f * fact },
   { 663.0f * fact, 82.0f * fact },
   { 178.0f * fact, 82.0f * fact },
   { 157.0f * fact, 82.0f * fact },
   { 139.0f * fact, 100.0f * fact },
   { 139.0f * fact, 121.0f * fact },
   { 139.0f * fact, 150.0f * fact },
   { 57.0f * fact, 150.0f * fact },
   { 57.0f * fact, 121.0f * fact },
   { 57.0f * fact, 54.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 178.0f * fact, 0.0f * fact },
   { 663.0f * fact, 0.0f * fact },
   { 730.0f * fact, 0.0f * fact },
   { 784.0f * fact, 54.0f * fact },
   { 784.0f * fact, 121.0f * fact },
   { 784.0f * fact, 291.0f * fact },
   { 784.0f * fact, 359.0f * fact },
   { 730.0f * fact, 412.0f * fact },
   { 663.0f * fact, 412.0f * fact },
   { 178.0f * fact, 412.0f * fact },
   { 157.0f * fact, 412.0f * fact },
   { 139.0f * fact, 430.0f * fact },
   { 139.0f * fact, 452.0f * fact },
   { 139.0f * fact, 599.0f * fact }
};

static const SFG_StrokeStrip chr_ol_53_strip[] = { {39, chr_ol_53_part_0} };

SFG_StrokeChar chr_ol_53 = { 830 * fact, 1, chr_ol_53_strip };


// six.glif
static const SFG_StrokeVertex chr_ol_54_part_0[] = {
   { 178.0f * fact, 415.0f * fact },
   { 157.0f * fact, 415.0f * fact },
   { 139.0f * fact, 433.0f * fact },
   { 139.0f * fact, 455.0f * fact },
   { 139.0f * fact, 599.0f * fact },
   { 139.0f * fact, 620.0f * fact },
   { 157.0f * fact, 638.0f * fact },
   { 178.0f * fact, 638.0f * fact },
   { 658.0f * fact, 638.0f * fact },
   { 658.0f * fact, 721.0f * fact },
   { 178.0f * fact, 721.0f * fact },
   { 110.0f * fact, 721.0f * fact },
   { 57.0f * fact, 667.0f * fact },
   { 57.0f * fact, 599.0f * fact },
   { 57.0f * fact, 121.0f * fact },
   { 57.0f * fact, 54.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 178.0f * fact, 0.0f * fact },
   { 663.0f * fact, 0.0f * fact },
   { 730.0f * fact, 0.0f * fact },
   { 784.0f * fact, 54.0f * fact },
   { 784.0f * fact, 121.0f * fact },
   { 784.0f * fact, 294.0f * fact },
   { 784.0f * fact, 362.0f * fact },
   { 730.0f * fact, 415.0f * fact },
   { 663.0f * fact, 415.0f * fact },
   { 178.0f * fact, 415.0f * fact }
};

// six.glif
static const SFG_StrokeVertex chr_ol_54_part_1[] = {
   { 702.0f * fact, 121.0f * fact },
   { 702.0f * fact, 100.0f * fact },
   { 684.0f * fact, 82.0f * fact },
   { 663.0f * fact, 82.0f * fact },
   { 178.0f * fact, 82.0f * fact },
   { 157.0f * fact, 82.0f * fact },
   { 139.0f * fact, 100.0f * fact },
   { 139.0f * fact, 121.0f * fact },
   { 139.0f * fact, 333.0f * fact },
   { 663.0f * fact, 333.0f * fact },
   { 684.0f * fact, 333.0f * fact },
   { 702.0f * fact, 315.0f * fact },
   { 702.0f * fact, 294.0f * fact },
   { 702.0f * fact, 121.0f * fact }
};

static const SFG_StrokeStrip chr_ol_54_strip[] = { {27, chr_ol_54_part_0}, {14, chr_ol_54_part_1} };

SFG_StrokeChar chr_ol_54 = { 820 * fact, 2, chr_ol_54_strip };


// seven.glif
static const SFG_StrokeVertex chr_ol_55_part_0[] = {
   { 483.0f * fact, 638.0f * fact },
   { 504.0f * fact, 638.0f * fact },
   { 522.0f * fact, 620.0f * fact },
   { 522.0f * fact, 599.0f * fact },
   { 522.0f * fact, 0.0f * fact },
   { 604.0f * fact, 0.0f * fact },
   { 604.0f * fact, 599.0f * fact },
   { 604.0f * fact, 667.0f * fact },
   { 550.0f * fact, 721.0f * fact },
   { 483.0f * fact, 721.0f * fact },
   { 3.0f * fact, 721.0f * fact },
   { 3.0f * fact, 638.0f * fact },
   { 483.0f * fact, 638.0f * fact }
};

static const SFG_StrokeStrip chr_ol_55_strip[] = { {13, chr_ol_55_part_0} };

SFG_StrokeChar chr_ol_55 = { 660 * fact, 1, chr_ol_55_strip };


// eight.glif
static const SFG_StrokeVertex chr_ol_56_part_0[] = {
   { 784.0f * fact, 619.0f * fact },
   { 769.0f * fact, 671.0f * fact },
   { 719.0f * fact, 721.0f * fact },
   { 663.0f * fact, 721.0f * fact },
   { 178.0f * fact, 721.0f * fact },
   { 110.0f * fact, 721.0f * fact },
   { 57.0f * fact, 667.0f * fact },
   { 57.0f * fact, 599.0f * fact },
   { 57.0f * fact, 452.0f * fact },
   { 57.0f * fact, 425.0f * fact },
   { 68.0f * fact, 397.0f * fact },
   { 87.0f * fact, 371.0f * fact },
   { 68.0f * fact, 346.0f * fact },
   { 57.0f * fact, 318.0f * fact },
   { 57.0f * fact, 291.0f * fact },
   { 57.0f * fact, 121.0f * fact },
   { 57.0f * fact, 54.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 178.0f * fact, 0.0f * fact },
   { 663.0f * fact, 0.0f * fact },
   { 730.0f * fact, 0.0f * fact },
   { 784.0f * fact, 54.0f * fact },
   { 784.0f * fact, 121.0f * fact },
   { 784.0f * fact, 291.0f * fact },
   { 784.0f * fact, 318.0f * fact },
   { 773.0f * fact, 346.0f * fact },
   { 754.0f * fact, 371.0f * fact },
   { 773.0f * fact, 397.0f * fact },
   { 784.0f * fact, 425.0f * fact },
   { 784.0f * fact, 452.0f * fact },
   { 784.0f * fact, 619.0f * fact }
};

// eight.glif
static const SFG_StrokeVertex chr_ol_56_part_1[] = {
   { 702.0f * fact, 121.0f * fact },
   { 702.0f * fact, 100.0f * fact },
   { 684.0f * fact, 82.0f * fact },
   { 663.0f * fact, 82.0f * fact },
   { 178.0f * fact, 82.0f * fact },
   { 157.0f * fact, 82.0f * fact },
   { 139.0f * fact, 100.0f * fact },
   { 139.0f * fact, 121.0f * fact },
   { 139.0f * fact, 291.0f * fact },
   { 139.0f * fact, 312.0f * fact },
   { 157.0f * fact, 330.0f * fact },
   { 178.0f * fact, 330.0f * fact },
   { 663.0f * fact, 330.0f * fact },
   { 684.0f * fact, 330.0f * fact },
   { 702.0f * fact, 312.0f * fact },
   { 702.0f * fact, 291.0f * fact },
   { 702.0f * fact, 121.0f * fact }
};

// eight.glif
static const SFG_StrokeVertex chr_ol_56_part_2[] = {
   { 702.0f * fact, 440.0f * fact },
   { 702.0f * fact, 419.0f * fact },
   { 684.0f * fact, 401.0f * fact },
   { 663.0f * fact, 401.0f * fact },
   { 178.0f * fact, 401.0f * fact },
   { 157.0f * fact, 401.0f * fact },
   { 139.0f * fact, 419.0f * fact },
   { 139.0f * fact, 440.0f * fact },
   { 139.0f * fact, 598.0f * fact },
   { 139.0f * fact, 619.0f * fact },
   { 157.0f * fact, 637.0f * fact },
   { 178.0f * fact, 637.0f * fact },
   { 663.0f * fact, 637.0f * fact },
   { 684.0f * fact, 637.0f * fact },
   { 702.0f * fact, 619.0f * fact },
   { 702.0f * fact, 598.0f * fact },
   { 702.0f * fact, 440.0f * fact }
};

static const SFG_StrokeStrip chr_ol_56_strip[] = { {31, chr_ol_56_part_0}, {17, chr_ol_56_part_1}, {17, chr_ol_56_part_2} };

SFG_StrokeChar chr_ol_56 = { 834 * fact, 3, chr_ol_56_strip };


// nine.glif
static const SFG_StrokeVertex chr_ol_57_part_0[] = {
   { 658.0f * fact, 305.0f * fact },
   { 679.0f * fact, 305.0f * fact },
   { 697.0f * fact, 287.0f * fact },
   { 697.0f * fact, 266.0f * fact },
   { 697.0f * fact, 121.0f * fact },
   { 697.0f * fact, 100.0f * fact },
   { 679.0f * fact, 82.0f * fact },
   { 658.0f * fact, 82.0f * fact },
   { 58.0f * fact, 82.0f * fact },
   { 74.0f * fact, 34.0f * fact },
   { 120.0f * fact, 0.0f * fact },
   { 173.0f * fact, 0.0f * fact },
   { 658.0f * fact, 0.0f * fact },
   { 725.0f * fact, 0.0f * fact },
   { 779.0f * fact, 54.0f * fact },
   { 779.0f * fact, 121.0f * fact },
   { 779.0f * fact, 599.0f * fact },
   { 779.0f * fact, 667.0f * fact },
   { 725.0f * fact, 721.0f * fact },
   { 658.0f * fact, 721.0f * fact },
   { 173.0f * fact, 721.0f * fact },
   { 105.0f * fact, 721.0f * fact },
   { 51.0f * fact, 667.0f * fact },
   { 51.0f * fact, 599.0f * fact },
   { 51.0f * fact, 426.0f * fact },
   { 51.0f * fact, 359.0f * fact },
   { 105.0f * fact, 305.0f * fact },
   { 173.0f * fact, 305.0f * fact },
   { 658.0f * fact, 305.0f * fact }
};

// nine.glif
static const SFG_StrokeVertex chr_ol_57_part_1[] = {
   { 134.0f * fact, 599.0f * fact },
   { 134.0f * fact, 620.0f * fact },
   { 152.0f * fact, 638.0f * fact },
   { 173.0f * fact, 638.0f * fact },
   { 658.0f * fact, 638.0f * fact },
   { 679.0f * fact, 638.0f * fact },
   { 697.0f * fact, 620.0f * fact },
   { 697.0f * fact, 599.0f * fact },
   { 697.0f * fact, 387.0f * fact },
   { 173.0f * fact, 387.0f * fact },
   { 152.0f * fact, 387.0f * fact },
   { 134.0f * fact, 405.0f * fact },
   { 134.0f * fact, 426.0f * fact },
   { 134.0f * fact, 599.0f * fact }
};

static const SFG_StrokeStrip chr_ol_57_strip[] = { {29, chr_ol_57_part_0}, {14, chr_ol_57_part_1} };

SFG_StrokeChar chr_ol_57 = { 828 * fact, 2, chr_ol_57_strip };


// colon.glif
static const SFG_StrokeVertex chr_ol_58_part_0[] = {
   { 54.0f * fact, 82.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 136.0f * fact, 82.0f * fact },
   { 54.0f * fact, 82.0f * fact }
};

// colon.glif
static const SFG_StrokeVertex chr_ol_58_part_1[] = {
   { 136.0f * fact, 588.0f * fact },
   { 54.0f * fact, 588.0f * fact },
   { 54.0f * fact, 506.0f * fact },
   { 136.0f * fact, 506.0f * fact },
   { 136.0f * fact, 588.0f * fact }
};

static const SFG_StrokeStrip chr_ol_58_strip[] = { {5, chr_ol_58_part_0}, {5, chr_ol_58_part_1} };

SFG_StrokeChar chr_ol_58 = { 214 * fact, 2, chr_ol_58_strip };


// semicolon.glif
static const SFG_StrokeVertex chr_ol_59_part_0[] = {
   { 133.0f * fact, 588.0f * fact },
   { 51.0f * fact, 588.0f * fact },
   { 51.0f * fact, 506.0f * fact },
   { 133.0f * fact, 506.0f * fact },
   { 133.0f * fact, 588.0f * fact }
};

// semicolon.glif
static const SFG_StrokeVertex chr_ol_59_part_1[] = {
   { 51.0f * fact, 82.0f * fact },
   { 51.0f * fact, -131.0f * fact },
   { 99.0f * fact, -115.0f * fact },
   { 133.0f * fact, -70.0f * fact },
   { 133.0f * fact, -16.0f * fact },
   { 133.0f * fact, 82.0f * fact },
   { 51.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_59_strip[] = { {5, chr_ol_59_part_0}, {7, chr_ol_59_part_1} };

SFG_StrokeChar chr_ol_59 = { 193 * fact, 2, chr_ol_59_strip };


// less.glif
static const SFG_StrokeVertex chr_ol_60_part_0[] = {
   { 87.0f * fact, 296.0f * fact },
   { 407.0f * fact, 481.0f * fact },
   { 407.0f * fact, 576.0f * fact },
   { 5.0f * fact, 345.0f * fact },
   { 5.0f * fact, 248.0f * fact },
   { 407.0f * fact, 16.0f * fact },
   { 407.0f * fact, 111.0f * fact },
   { 87.0f * fact, 296.0f * fact }
};

static const SFG_StrokeStrip chr_ol_60_strip[] = { {8, chr_ol_60_part_0} };

SFG_StrokeChar chr_ol_60 = { 473 * fact, 1, chr_ol_60_strip };


// equal.glif
static const SFG_StrokeVertex chr_ol_61_part_0[] = {
   { 576.0f * fact, 233.0f * fact },
   { 59.0f * fact, 233.0f * fact },
   { 59.0f * fact, 151.0f * fact },
   { 576.0f * fact, 151.0f * fact },
   { 576.0f * fact, 233.0f * fact }
};

// equal.glif
static const SFG_StrokeVertex chr_ol_61_part_1[] = {
   { 576.0f * fact, 433.0f * fact },
   { 59.0f * fact, 433.0f * fact },
   { 59.0f * fact, 351.0f * fact },
   { 576.0f * fact, 351.0f * fact },
   { 576.0f * fact, 433.0f * fact }
};

static const SFG_StrokeStrip chr_ol_61_strip[] = { {5, chr_ol_61_part_0}, {5, chr_ol_61_part_1} };

SFG_StrokeChar chr_ol_61 = { 638 * fact, 2, chr_ol_61_strip };


// greater.glif
static const SFG_StrokeVertex chr_ol_62_part_0[] = {
   { 59.0f * fact, 14.0f * fact },
   { 196.0f * fact, 93.0f * fact },
   { 322.0f * fact, 167.0f * fact },
   { 461.0f * fact, 246.0f * fact },
   { 461.0f * fact, 343.0f * fact },
   { 59.0f * fact, 574.0f * fact },
   { 59.0f * fact, 479.0f * fact },
   { 379.0f * fact, 294.0f * fact },
   { 59.0f * fact, 109.0f * fact },
   { 59.0f * fact, 14.0f * fact }
};

static const SFG_StrokeStrip chr_ol_62_strip[] = { {10, chr_ol_62_part_0} };

SFG_StrokeChar chr_ol_62 = { 475 * fact, 1, chr_ol_62_strip };


// question.glif
static const SFG_StrokeVertex chr_ol_63_part_0[] = {
   { 31.0f * fact, 720.0f * fact },
   { 31.0f * fact, 720.0f * fact },
   { 31.0f * fact, 701.0f * fact },
   { 31.0f * fact, 636.0f * fact },
   { 548.0f * fact, 636.0f * fact },
   { 569.0f * fact, 636.0f * fact },
   { 587.0f * fact, 618.0f * fact },
   { 587.0f * fact, 597.0f * fact },
   { 587.0f * fact, 403.0f * fact },
   { 587.0f * fact, 382.0f * fact },
   { 569.0f * fact, 364.0f * fact },
   { 548.0f * fact, 364.0f * fact },
   { 274.0f * fact, 364.0f * fact },
   { 207.0f * fact, 364.0f * fact },
   { 153.0f * fact, 311.0f * fact },
   { 153.0f * fact, 243.0f * fact },
   { 153.0f * fact, 201.0f * fact },
   { 235.0f * fact, 201.0f * fact },
   { 235.0f * fact, 243.0f * fact },
   { 235.0f * fact, 264.0f * fact },
   { 253.0f * fact, 282.0f * fact },
   { 274.0f * fact, 282.0f * fact },
   { 548.0f * fact, 282.0f * fact },
   { 615.0f * fact, 282.0f * fact },
   { 669.0f * fact, 336.0f * fact },
   { 669.0f * fact, 403.0f * fact },
   { 669.0f * fact, 597.0f * fact },
   { 669.0f * fact, 665.0f * fact },
   { 615.0f * fact, 719.0f * fact },
   { 548.0f * fact, 719.0f * fact },
   { 31.0f * fact, 720.0f * fact }
};

// question.glif
static const SFG_StrokeVertex chr_ol_63_part_1[] = {
   { 235.0f * fact, 0.0f * fact },
   { 235.0f * fact, 82.0f * fact },
   { 153.0f * fact, 82.0f * fact },
   { 153.0f * fact, 0.0f * fact },
   { 235.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_ol_63_strip[] = { {31, chr_ol_63_part_0}, {5, chr_ol_63_part_1} };

SFG_StrokeChar chr_ol_63 = { 678 * fact, 2, chr_ol_63_strip };


// at.glif
static const SFG_StrokeVertex chr_ol_64_part_0[] = {
   { 373.0f * fact, 519.0f * fact },
   { 305.0f * fact, 519.0f * fact },
   { 252.0f * fact, 465.0f * fact },
   { 252.0f * fact, 398.0f * fact },
   { 252.0f * fact, 314.0f * fact },
   { 252.0f * fact, 246.0f * fact },
   { 305.0f * fact, 192.0f * fact },
   { 373.0f * fact, 192.0f * fact },
   { 784.0f * fact, 192.0f * fact },
   { 784.0f * fact, 598.0f * fact },
   { 784.0f * fact, 666.0f * fact },
   { 730.0f * fact, 720.0f * fact },
   { 663.0f * fact, 720.0f * fact },
   { 178.0f * fact, 720.0f * fact },
   { 110.0f * fact, 720.0f * fact },
   { 57.0f * fact, 666.0f * fact },
   { 57.0f * fact, 598.0f * fact },
   { 57.0f * fact, 121.0f * fact },
   { 57.0f * fact, 54.0f * fact },
   { 110.0f * fact, 0.0f * fact },
   { 178.0f * fact, 0.0f * fact },
   { 784.0f * fact, 0.0f * fact },
   { 784.0f * fact, 82.0f * fact },
   { 178.0f * fact, 82.0f * fact },
   { 157.0f * fact, 82.0f * fact },
   { 139.0f * fact, 100.0f * fact },
   { 139.0f * fact, 121.0f * fact },
   { 139.0f * fact, 598.0f * fact },
   { 139.0f * fact, 619.0f * fact },
   { 157.0f * fact, 637.0f * fact },
   { 178.0f * fact, 637.0f * fact },
   { 663.0f * fact, 637.0f * fact },
   { 684.0f * fact, 637.0f * fact },
   { 702.0f * fact, 619.0f * fact },
   { 702.0f * fact, 598.0f * fact },
   { 702.0f * fact, 254.0f * fact },
   { 589.0f * fact, 254.0f * fact },
   { 589.0f * fact, 398.0f * fact },
   { 589.0f * fact, 465.0f * fact },
   { 535.0f * fact, 519.0f * fact },
   { 468.0f * fact, 519.0f * fact },
   { 373.0f * fact, 519.0f * fact }
};

// at.glif
static const SFG_StrokeVertex chr_ol_64_part_1[] = {
   { 352.0f * fact, 254.0f * fact },
   { 331.0f * fact, 254.0f * fact },
   { 313.0f * fact, 271.0f * fact },
   { 313.0f * fact, 293.0f * fact },
   { 313.0f * fact, 419.0f * fact },
   { 313.0f * fact, 440.0f * fact },
   { 331.0f * fact, 458.0f * fact },
   { 352.0f * fact, 458.0f * fact },
   { 489.0f * fact, 458.0f * fact },
   { 510.0f * fact, 458.0f * fact },
   { 528.0f * fact, 440.0f * fact },
   { 528.0f * fact, 419.0f * fact },
   { 528.0f * fact, 254.0f * fact },
   { 352.0f * fact, 254.0f * fact }
};

static const SFG_StrokeStrip chr_ol_64_strip[] = { {42, chr_ol_64_part_0}, {14, chr_ol_64_part_1} };

SFG_StrokeChar chr_ol_64 = { 831 * fact, 2, chr_ol_64_strip };


// A_.glif
static const SFG_StrokeVertex chr_ol_65_part_0[] = {
   { 178.0f * fact, 720.0f * fact },
   { 111.0f * fact, 720.0f * fact },
   { 58.0f * fact, 667.0f * fact },
   { 58.0f * fact, 600.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 139.0f * fact, 0.0f * fact },
   { 139.0f * fact, 263.0f * fact },
   { 697.0f * fact, 263.0f * fact },
   { 697.0f * fact, 0.0f * fact },
   { 778.0f * fact, 0.0f * fact },
   { 778.0f * fact, 600.0f * fact },
   { 778.0f * fact, 667.0f * fact },
   { 725.0f * fact, 720.0f * fact },
   { 658.0f * fact, 720.0f * fact },
   { 178.0f * fact, 720.0f * fact }
};

// A_.glif
static const SFG_StrokeVertex chr_ol_65_part_1[] = {
   { 139.0f * fact, 344.0f * fact },
   { 139.0f * fact, 600.0f * fact },
   { 139.0f * fact, 621.0f * fact },
   { 157.0f * fact, 639.0f * fact },
   { 178.0f * fact, 639.0f * fact },
   { 658.0f * fact, 639.0f * fact },
   { 679.0f * fact, 639.0f * fact },
   { 697.0f * fact, 621.0f * fact },
   { 697.0f * fact, 600.0f * fact },
   { 697.0f * fact, 344.0f * fact },
   { 139.0f * fact, 344.0f * fact }
};

static const SFG_StrokeStrip chr_ol_65_strip[] = { {15, chr_ol_65_part_0}, {11, chr_ol_65_part_1} };

SFG_StrokeChar chr_ol_65 = { 836 * fact, 2, chr_ol_65_strip };


// B_.glif
static const SFG_StrokeVertex chr_ol_66_part_0[] = {
   { 735.0f * fact, 394.0f * fact },
   { 743.0f * fact, 410.0f * fact },
   { 748.0f * fact, 428.0f * fact },
   { 748.0f * fact, 447.0f * fact },
   { 748.0f * fact, 600.0f * fact },
   { 748.0f * fact, 667.0f * fact },
   { 695.0f * fact, 720.0f * fact },
   { 628.0f * fact, 720.0f * fact },
   { 59.0f * fact, 720.0f * fact },
   { 59.0f * fact, 0.0f * fact },
   { 659.0f * fact, 0.0f * fact },
   { 726.0f * fact, 0.0f * fact },
   { 779.0f * fact, 53.0f * fact },
   { 779.0f * fact, 120.0f * fact },
   { 779.0f * fact, 288.0f * fact },
   { 779.0f * fact, 322.0f * fact },
   { 763.0f * fact, 355.0f * fact },
   { 739.0f * fact, 377.0f * fact },
   { 735.0f * fact, 394.0f * fact }
};

// B_.glif
static const SFG_StrokeVertex chr_ol_66_part_1[] = {
   { 628.0f * fact, 639.0f * fact },
   { 649.0f * fact, 639.0f * fact },
   { 666.0f * fact, 621.0f * fact },
   { 666.0f * fact, 600.0f * fact },
   { 666.0f * fact, 447.0f * fact },
   { 666.0f * fact, 426.0f * fact },
   { 649.0f * fact, 408.0f * fact },
   { 628.0f * fact, 408.0f * fact },
   { 179.0f * fact, 408.0f * fact },
   { 158.0f * fact, 408.0f * fact },
   { 140.0f * fact, 426.0f * fact },
   { 140.0f * fact, 447.0f * fact },
   { 140.0f * fact, 600.0f * fact },
   { 140.0f * fact, 621.0f * fact },
   { 158.0f * fact, 639.0f * fact },
   { 179.0f * fact, 639.0f * fact },
   { 628.0f * fact, 639.0f * fact }
};

// B_.glif
static const SFG_StrokeVertex chr_ol_66_part_2[] = {
   { 698.0f * fact, 120.0f * fact },
   { 698.0f * fact, 99.0f * fact },
   { 680.0f * fact, 81.0f * fact },
   { 659.0f * fact, 81.0f * fact },
   { 179.0f * fact, 81.0f * fact },
   { 158.0f * fact, 81.0f * fact },
   { 140.0f * fact, 99.0f * fact },
   { 140.0f * fact, 120.0f * fact },
   { 140.0f * fact, 288.0f * fact },
   { 140.0f * fact, 309.0f * fact },
   { 158.0f * fact, 327.0f * fact },
   { 179.0f * fact, 327.0f * fact },
   { 659.0f * fact, 327.0f * fact },
   { 680.0f * fact, 327.0f * fact },
   { 698.0f * fact, 309.0f * fact },
   { 698.0f * fact, 288.0f * fact },
   { 698.0f * fact, 120.0f * fact }
};

static const SFG_StrokeStrip chr_ol_66_strip[] = { {19, chr_ol_66_part_0}, {17, chr_ol_66_part_1}, {17, chr_ol_66_part_2} };

SFG_StrokeChar chr_ol_66 = { 832 * fact, 3, chr_ol_66_strip };


// C_.glif
static const SFG_StrokeVertex chr_ol_67_part_0[] = {
   { 774.0f * fact, 639.0f * fact },
   { 774.0f * fact, 720.0f * fact },
   { 176.0f * fact, 720.0f * fact },
   { 109.0f * fact, 720.0f * fact },
   { 56.0f * fact, 667.0f * fact },
   { 56.0f * fact, 600.0f * fact },
   { 56.0f * fact, 120.0f * fact },
   { 56.0f * fact, 53.0f * fact },
   { 109.0f * fact, 0.0f * fact },
   { 176.0f * fact, 0.0f * fact },
   { 774.0f * fact, 0.0f * fact },
   { 774.0f * fact, 81.0f * fact },
   { 176.0f * fact, 81.0f * fact },
   { 155.0f * fact, 81.0f * fact },
   { 137.0f * fact, 99.0f * fact },
   { 137.0f * fact, 120.0f * fact },
   { 137.0f * fact, 600.0f * fact },
   { 137.0f * fact, 621.0f * fact },
   { 155.0f * fact, 639.0f * fact },
   { 176.0f * fact, 639.0f * fact },
   { 774.0f * fact, 639.0f * fact }
};

static const SFG_StrokeStrip chr_ol_67_strip[] = { {21, chr_ol_67_part_0} };

SFG_StrokeChar chr_ol_67 = { 822 * fact, 1, chr_ol_67_strip };


// D_.glif
static const SFG_StrokeVertex chr_ol_68_part_0[] = {
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 658.0f * fact, 0.0f * fact },
   { 725.0f * fact, 0.0f * fact },
   { 778.0f * fact, 53.0f * fact },
   { 778.0f * fact, 120.0f * fact },
   { 778.0f * fact, 600.0f * fact },
   { 778.0f * fact, 667.0f * fact },
   { 725.0f * fact, 720.0f * fact },
   { 658.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact }
};

// D_.glif
static const SFG_StrokeVertex chr_ol_68_part_1[] = {
   { 697.0f * fact, 120.0f * fact },
   { 697.0f * fact, 99.0f * fact },
   { 679.0f * fact, 81.0f * fact },
   { 658.0f * fact, 81.0f * fact },
   { 178.0f * fact, 81.0f * fact },
   { 157.0f * fact, 81.0f * fact },
   { 139.0f * fact, 99.0f * fact },
   { 139.0f * fact, 120.0f * fact },
   { 139.0f * fact, 600.0f * fact },
   { 139.0f * fact, 621.0f * fact },
   { 157.0f * fact, 639.0f * fact },
   { 178.0f * fact, 639.0f * fact },
   { 658.0f * fact, 639.0f * fact },
   { 679.0f * fact, 639.0f * fact },
   { 697.0f * fact, 621.0f * fact },
   { 697.0f * fact, 600.0f * fact },
   { 697.0f * fact, 120.0f * fact }
};

static const SFG_StrokeStrip chr_ol_68_strip[] = { {11, chr_ol_68_part_0}, {17, chr_ol_68_part_1} };

SFG_StrokeChar chr_ol_68 = { 834 * fact, 2, chr_ol_68_strip };


// E_.glif
static const SFG_StrokeVertex chr_ol_69_part_0[] = {
   { 715.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 715.0f * fact, 0.0f * fact },
   { 715.0f * fact, 81.0f * fact },
   { 139.0f * fact, 81.0f * fact },
   { 139.0f * fact, 319.0f * fact },
   { 602.0f * fact, 319.0f * fact },
   { 602.0f * fact, 401.0f * fact },
   { 139.0f * fact, 401.0f * fact },
   { 139.0f * fact, 639.0f * fact },
   { 715.0f * fact, 639.0f * fact },
   { 715.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_69_strip[] = { {13, chr_ol_69_part_0} };

SFG_StrokeChar chr_ol_69 = { 766 * fact, 1, chr_ol_69_strip };


// F_.glif
static const SFG_StrokeVertex chr_ol_70_part_0[] = {
   { 58.0f * fact, 720.0f * fact },
   { 58.0f * fact, 0.0f * fact },
   { 139.0f * fact, 0.0f * fact },
   { 139.0f * fact, 319.0f * fact },
   { 602.0f * fact, 319.0f * fact },
   { 602.0f * fact, 401.0f * fact },
   { 139.0f * fact, 401.0f * fact },
   { 139.0f * fact, 639.0f * fact },
   { 715.0f * fact, 639.0f * fact },
   { 715.0f * fact, 720.0f * fact },
   { 58.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_70_strip[] = { {11, chr_ol_70_part_0} };

SFG_StrokeChar chr_ol_70 = { 723 * fact, 1, chr_ol_70_strip };


// G_.glif
static const SFG_StrokeVertex chr_ol_71_part_0[] = {
   { 776.0f * fact, 600.0f * fact },
   { 776.0f * fact, 667.0f * fact },
   { 723.0f * fact, 720.0f * fact },
   { 656.0f * fact, 720.0f * fact },
   { 176.0f * fact, 720.0f * fact },
   { 109.0f * fact, 720.0f * fact },
   { 56.0f * fact, 667.0f * fact },
   { 56.0f * fact, 600.0f * fact },
   { 56.0f * fact, 120.0f * fact },
   { 56.0f * fact, 53.0f * fact },
   { 109.0f * fact, 0.0f * fact },
   { 176.0f * fact, 0.0f * fact },
   { 656.0f * fact, 0.0f * fact },
   { 723.0f * fact, 0.0f * fact },
   { 776.0f * fact, 53.0f * fact },
   { 776.0f * fact, 120.0f * fact },
   { 776.0f * fact, 380.0f * fact },
   { 517.0f * fact, 380.0f * fact },
   { 517.0f * fact, 298.0f * fact },
   { 695.0f * fact, 298.0f * fact },
   { 695.0f * fact, 120.0f * fact },
   { 695.0f * fact, 99.0f * fact },
   { 677.0f * fact, 81.0f * fact },
   { 656.0f * fact, 81.0f * fact },
   { 176.0f * fact, 81.0f * fact },
   { 155.0f * fact, 81.0f * fact },
   { 137.0f * fact, 99.0f * fact },
   { 137.0f * fact, 120.0f * fact },
   { 137.0f * fact, 600.0f * fact },
   { 137.0f * fact, 621.0f * fact },
   { 155.0f * fact, 639.0f * fact },
   { 176.0f * fact, 639.0f * fact },
   { 656.0f * fact, 639.0f * fact },
   { 677.0f * fact, 639.0f * fact },
   { 695.0f * fact, 621.0f * fact },
   { 695.0f * fact, 600.0f * fact },
   { 695.0f * fact, 571.0f * fact },
   { 776.0f * fact, 571.0f * fact },
   { 776.0f * fact, 600.0f * fact }
};

static const SFG_StrokeStrip chr_ol_71_strip[] = { {39, chr_ol_71_part_0} };

SFG_StrokeChar chr_ol_71 = { 830 * fact, 1, chr_ol_71_strip };


// H_.glif
static const SFG_StrokeVertex chr_ol_72_part_0[] = {
   { 713.0f * fact, 720.0f * fact },
   { 713.0f * fact, 401.0f * fact },
   { 138.0f * fact, 401.0f * fact },
   { 138.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 138.0f * fact, 0.0f * fact },
   { 138.0f * fact, 319.0f * fact },
   { 713.0f * fact, 319.0f * fact },
   { 713.0f * fact, 0.0f * fact },
   { 795.0f * fact, 0.0f * fact },
   { 795.0f * fact, 720.0f * fact },
   { 713.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_72_strip[] = { {13, chr_ol_72_part_0} };

SFG_StrokeChar chr_ol_72 = { 851 * fact, 1, chr_ol_72_strip };


// I_.glif
static const SFG_StrokeVertex chr_ol_73_part_0[] = {
   { 57.0f * fact, 0.0f * fact },
   { 138.0f * fact, 0.0f * fact },
   { 138.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_ol_73_strip[] = { {5, chr_ol_73_part_0} };

SFG_StrokeChar chr_ol_73 = { 220 * fact, 1, chr_ol_73_strip };


// J_.glif
static const SFG_StrokeVertex chr_ol_74_part_0[] = {
   { 643.0f * fact, 120.0f * fact },
   { 643.0f * fact, 99.0f * fact },
   { 625.0f * fact, 81.0f * fact },
   { 604.0f * fact, 81.0f * fact },
   { 124.0f * fact, 81.0f * fact },
   { 103.0f * fact, 81.0f * fact },
   { 85.0f * fact, 99.0f * fact },
   { 85.0f * fact, 120.0f * fact },
   { 85.0f * fact, 177.0f * fact },
   { 4.0f * fact, 177.0f * fact },
   { 4.0f * fact, 120.0f * fact },
   { 4.0f * fact, 53.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 124.0f * fact, 0.0f * fact },
   { 604.0f * fact, 0.0f * fact },
   { 671.0f * fact, 0.0f * fact },
   { 724.0f * fact, 53.0f * fact },
   { 724.0f * fact, 120.0f * fact },
   { 724.0f * fact, 720.0f * fact },
   { 643.0f * fact, 720.0f * fact },
   { 643.0f * fact, 120.0f * fact }
};

static const SFG_StrokeStrip chr_ol_74_strip[] = { {21, chr_ol_74_part_0} };

SFG_StrokeChar chr_ol_74 = { 780 * fact, 1, chr_ol_74_strip };


// K_.glif
static const SFG_StrokeVertex chr_ol_75_part_0[] = {
   { 639.0f * fact, 720.0f * fact },
   { 552.0f * fact, 616.0f * fact },
   { 458.0f * fact, 505.0f * fact },
   { 371.0f * fact, 401.0f * fact },
   { 139.0f * fact, 401.0f * fact },
   { 139.0f * fact, 720.0f * fact },
   { 57.0f * fact, 720.0f * fact },
   { 57.0f * fact, 0.0f * fact },
   { 139.0f * fact, 0.0f * fact },
   { 139.0f * fact, 319.0f * fact },
   { 371.0f * fact, 319.0f * fact },
   { 639.0f * fact, 0.0f * fact },
   { 744.0f * fact, 0.0f * fact },
   { 442.0f * fact, 360.0f * fact },
   { 743.0f * fact, 720.0f * fact },
   { 639.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_75_strip[] = { {16, chr_ol_75_part_0} };

SFG_StrokeChar chr_ol_75 = { 797 * fact, 1, chr_ol_75_strip };


// L_.glif
static const SFG_StrokeVertex chr_ol_76_part_0[] = {
   { 57.0f * fact, 0.0f * fact },
   { 777.0f * fact, 0.0f * fact },
   { 777.0f * fact, 81.0f * fact },
   { 138.0f * fact, 81.0f * fact },
   { 138.0f * fact, 721.0f * fact },
   { 57.0f * fact, 721.0f * fact },
   { 57.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_ol_76_strip[] = { {7, chr_ol_76_part_0} };

SFG_StrokeChar chr_ol_76 = { 779 * fact, 1, chr_ol_76_strip };


// M_.glif
static const SFG_StrokeVertex chr_ol_77_part_0[] = {
   { 466.0f * fact, 364.0f * fact },
   { 167.0f * fact, 720.0f * fact },
   { 56.0f * fact, 720.0f * fact },
   { 56.0f * fact, 0.0f * fact },
   { 137.0f * fact, 0.0f * fact },
   { 137.0f * fact, 628.0f * fact },
   { 466.0f * fact, 237.0f * fact },
   { 795.0f * fact, 628.0f * fact },
   { 795.0f * fact, 0.0f * fact },
   { 876.0f * fact, 0.0f * fact },
   { 876.0f * fact, 720.0f * fact },
   { 765.0f * fact, 720.0f * fact },
   { 466.0f * fact, 364.0f * fact }
};

static const SFG_StrokeStrip chr_ol_77_strip[] = { {13, chr_ol_77_part_0} };

SFG_StrokeChar chr_ol_77 = { 928 * fact, 1, chr_ol_77_strip };


// N_.glif
static const SFG_StrokeVertex chr_ol_78_part_0[] = {
   { 695.0f * fact, 92.0f * fact },
   { 167.0f * fact, 720.0f * fact },
   { 56.0f * fact, 720.0f * fact },
   { 56.0f * fact, 0.0f * fact },
   { 137.0f * fact, 0.0f * fact },
   { 137.0f * fact, 628.0f * fact },
   { 665.0f * fact, 0.0f * fact },
   { 776.0f * fact, 0.0f * fact },
   { 776.0f * fact, 720.0f * fact },
   { 695.0f * fact, 720.0f * fact },
   { 695.0f * fact, 92.0f * fact }
};

static const SFG_StrokeStrip chr_ol_78_strip[] = { {11, chr_ol_78_part_0} };

SFG_StrokeChar chr_ol_78 = { 832 * fact, 1, chr_ol_78_strip };


// O_.glif
static const SFG_StrokeVertex chr_ol_79_part_0[] = {
   { 174.0f * fact, 720.0f * fact },
   { 107.0f * fact, 720.0f * fact },
   { 54.0f * fact, 667.0f * fact },
   { 54.0f * fact, 600.0f * fact },
   { 54.0f * fact, 120.0f * fact },
   { 54.0f * fact, 53.0f * fact },
   { 107.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 654.0f * fact, 0.0f * fact },
   { 721.0f * fact, 0.0f * fact },
   { 774.0f * fact, 53.0f * fact },
   { 774.0f * fact, 120.0f * fact },
   { 774.0f * fact, 600.0f * fact },
   { 774.0f * fact, 667.0f * fact },
   { 721.0f * fact, 720.0f * fact },
   { 654.0f * fact, 720.0f * fact },
   { 174.0f * fact, 720.0f * fact }
};

// O_.glif
static const SFG_StrokeVertex chr_ol_79_part_1[] = {
   { 174.0f * fact, 81.0f * fact },
   { 153.0f * fact, 81.0f * fact },
   { 135.0f * fact, 99.0f * fact },
   { 135.0f * fact, 120.0f * fact },
   { 135.0f * fact, 600.0f * fact },
   { 135.0f * fact, 621.0f * fact },
   { 153.0f * fact, 639.0f * fact },
   { 174.0f * fact, 639.0f * fact },
   { 654.0f * fact, 639.0f * fact },
   { 675.0f * fact, 639.0f * fact },
   { 693.0f * fact, 621.0f * fact },
   { 693.0f * fact, 600.0f * fact },
   { 693.0f * fact, 120.0f * fact },
   { 693.0f * fact, 99.0f * fact },
   { 675.0f * fact, 81.0f * fact },
   { 654.0f * fact, 81.0f * fact },
   { 174.0f * fact, 81.0f * fact }
};

static const SFG_StrokeStrip chr_ol_79_strip[] = { {17, chr_ol_79_part_0}, {17, chr_ol_79_part_1} };

SFG_StrokeChar chr_ol_79 = { 828 * fact, 2, chr_ol_79_strip };


// P_.glif
static const SFG_StrokeVertex chr_ol_80_part_0[] = {
   { 56.0f * fact, 719.0f * fact },
   { 56.0f * fact, 0.0f * fact },
   { 137.0f * fact, 0.0f * fact },
   { 137.0f * fact, 272.0f * fact },
   { 152.0f * fact, 270.0f * fact },
   { 167.0f * fact, 268.0f * fact },
   { 176.0f * fact, 268.0f * fact },
   { 656.0f * fact, 268.0f * fact },
   { 723.0f * fact, 268.0f * fact },
   { 776.0f * fact, 322.0f * fact },
   { 776.0f * fact, 388.0f * fact },
   { 776.0f * fact, 599.0f * fact },
   { 776.0f * fact, 665.0f * fact },
   { 723.0f * fact, 719.0f * fact },
   { 656.0f * fact, 719.0f * fact },
   { 56.0f * fact, 719.0f * fact }
};

// P_.glif
static const SFG_StrokeVertex chr_ol_80_part_1[] = {
   { 695.0f * fact, 388.0f * fact },
   { 695.0f * fact, 367.0f * fact },
   { 677.0f * fact, 350.0f * fact },
   { 656.0f * fact, 350.0f * fact },
   { 176.0f * fact, 350.0f * fact },
   { 155.0f * fact, 350.0f * fact },
   { 137.0f * fact, 367.0f * fact },
   { 137.0f * fact, 388.0f * fact },
   { 137.0f * fact, 599.0f * fact },
   { 137.0f * fact, 620.0f * fact },
   { 155.0f * fact, 638.0f * fact },
   { 176.0f * fact, 638.0f * fact },
   { 656.0f * fact, 638.0f * fact },
   { 677.0f * fact, 638.0f * fact },
   { 695.0f * fact, 620.0f * fact },
   { 695.0f * fact, 599.0f * fact },
   { 695.0f * fact, 388.0f * fact }
};

static const SFG_StrokeStrip chr_ol_80_strip[] = { {16, chr_ol_80_part_0}, {17, chr_ol_80_part_1} };

SFG_StrokeChar chr_ol_80 = { 791 * fact, 2, chr_ol_80_strip };


// Q_.glif
static const SFG_StrokeVertex chr_ol_81_part_0[] = {
   { 770.0f * fact, 81.0f * fact },
   { 772.0f * fact, 96.0f * fact },
   { 774.0f * fact, 111.0f * fact },
   { 774.0f * fact, 120.0f * fact },
   { 774.0f * fact, 600.0f * fact },
   { 774.0f * fact, 667.0f * fact },
   { 721.0f * fact, 720.0f * fact },
   { 654.0f * fact, 720.0f * fact },
   { 174.0f * fact, 720.0f * fact },
   { 107.0f * fact, 720.0f * fact },
   { 54.0f * fact, 667.0f * fact },
   { 54.0f * fact, 600.0f * fact },
   { 54.0f * fact, 120.0f * fact },
   { 54.0f * fact, 53.0f * fact },
   { 107.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 868.0f * fact, 0.0f * fact },
   { 868.0f * fact, 81.0f * fact },
   { 770.0f * fact, 81.0f * fact }
};

// Q_.glif
static const SFG_StrokeVertex chr_ol_81_part_1[] = {
   { 174.0f * fact, 81.0f * fact },
   { 153.0f * fact, 81.0f * fact },
   { 135.0f * fact, 99.0f * fact },
   { 135.0f * fact, 120.0f * fact },
   { 135.0f * fact, 600.0f * fact },
   { 135.0f * fact, 621.0f * fact },
   { 153.0f * fact, 639.0f * fact },
   { 174.0f * fact, 639.0f * fact },
   { 654.0f * fact, 639.0f * fact },
   { 675.0f * fact, 639.0f * fact },
   { 693.0f * fact, 621.0f * fact },
   { 693.0f * fact, 600.0f * fact },
   { 693.0f * fact, 120.0f * fact },
   { 693.0f * fact, 99.0f * fact },
   { 675.0f * fact, 81.0f * fact },
   { 654.0f * fact, 81.0f * fact },
   { 174.0f * fact, 81.0f * fact }
};

static const SFG_StrokeStrip chr_ol_81_strip[] = { {19, chr_ol_81_part_0}, {17, chr_ol_81_part_1} };

SFG_StrokeChar chr_ol_81 = { 884 * fact, 2, chr_ol_81_strip };


// R_.glif
static const SFG_StrokeVertex chr_ol_82_part_0[] = {
   { 775.0f * fact, 599.0f * fact },
   { 775.0f * fact, 665.0f * fact },
   { 722.0f * fact, 719.0f * fact },
   { 655.0f * fact, 719.0f * fact },
   { 55.0f * fact, 719.0f * fact },
   { 55.0f * fact, 0.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 136.0f * fact, 272.0f * fact },
   { 151.0f * fact, 270.0f * fact },
   { 165.0f * fact, 268.0f * fact },
   { 175.0f * fact, 268.0f * fact },
   { 439.0f * fact, 268.0f * fact },
   { 664.0f * fact, 0.0f * fact },
   { 771.0f * fact, 0.0f * fact },
   { 545.0f * fact, 268.0f * fact },
   { 655.0f * fact, 268.0f * fact },
   { 722.0f * fact, 268.0f * fact },
   { 775.0f * fact, 322.0f * fact },
   { 775.0f * fact, 388.0f * fact },
   { 775.0f * fact, 599.0f * fact }
};

// R_.glif
static const SFG_StrokeVertex chr_ol_82_part_1[] = {
   { 175.0f * fact, 350.0f * fact },
   { 154.0f * fact, 350.0f * fact },
   { 136.0f * fact, 367.0f * fact },
   { 136.0f * fact, 388.0f * fact },
   { 136.0f * fact, 599.0f * fact },
   { 136.0f * fact, 620.0f * fact },
   { 154.0f * fact, 638.0f * fact },
   { 175.0f * fact, 638.0f * fact },
   { 655.0f * fact, 638.0f * fact },
   { 676.0f * fact, 638.0f * fact },
   { 693.0f * fact, 620.0f * fact },
   { 693.0f * fact, 599.0f * fact },
   { 693.0f * fact, 388.0f * fact },
   { 693.0f * fact, 367.0f * fact },
   { 676.0f * fact, 350.0f * fact },
   { 655.0f * fact, 350.0f * fact },
   { 175.0f * fact, 350.0f * fact }
};

static const SFG_StrokeStrip chr_ol_82_strip[] = { {20, chr_ol_82_part_0}, {17, chr_ol_82_part_1} };

SFG_StrokeChar chr_ol_82 = { 825 * fact, 2, chr_ol_82_strip };


// S_.glif
static const SFG_StrokeVertex chr_ol_83_part_0[] = {
   { 771.0f * fact, 600.0f * fact },
   { 771.0f * fact, 667.0f * fact },
   { 718.0f * fact, 720.0f * fact },
   { 651.0f * fact, 720.0f * fact },
   { 171.0f * fact, 720.0f * fact },
   { 104.0f * fact, 720.0f * fact },
   { 51.0f * fact, 667.0f * fact },
   { 51.0f * fact, 600.0f * fact },
   { 51.0f * fact, 439.0f * fact },
   { 51.0f * fact, 373.0f * fact },
   { 104.0f * fact, 319.0f * fact },
   { 171.0f * fact, 319.0f * fact },
   { 651.0f * fact, 319.0f * fact },
   { 672.0f * fact, 319.0f * fact },
   { 690.0f * fact, 302.0f * fact },
   { 690.0f * fact, 281.0f * fact },
   { 690.0f * fact, 120.0f * fact },
   { 690.0f * fact, 99.0f * fact },
   { 672.0f * fact, 81.0f * fact },
   { 651.0f * fact, 81.0f * fact },
   { 171.0f * fact, 81.0f * fact },
   { 150.0f * fact, 81.0f * fact },
   { 132.0f * fact, 99.0f * fact },
   { 132.0f * fact, 120.0f * fact },
   { 132.0f * fact, 148.0f * fact },
   { 51.0f * fact, 148.0f * fact },
   { 51.0f * fact, 120.0f * fact },
   { 51.0f * fact, 53.0f * fact },
   { 104.0f * fact, 0.0f * fact },
   { 171.0f * fact, 0.0f * fact },
   { 651.0f * fact, 0.0f * fact },
   { 718.0f * fact, 0.0f * fact },
   { 771.0f * fact, 53.0f * fact },
   { 771.0f * fact, 120.0f * fact },
   { 771.0f * fact, 281.0f * fact },
   { 771.0f * fact, 347.0f * fact },
   { 718.0f * fact, 401.0f * fact },
   { 651.0f * fact, 401.0f * fact },
   { 171.0f * fact, 401.0f * fact },
   { 150.0f * fact, 401.0f * fact },
   { 132.0f * fact, 418.0f * fact },
   { 132.0f * fact, 439.0f * fact },
   { 132.0f * fact, 600.0f * fact },
   { 132.0f * fact, 621.0f * fact },
   { 150.0f * fact, 639.0f * fact },
   { 171.0f * fact, 639.0f * fact },
   { 651.0f * fact, 639.0f * fact },
   { 672.0f * fact, 639.0f * fact },
   { 690.0f * fact, 621.0f * fact },
   { 690.0f * fact, 600.0f * fact },
   { 690.0f * fact, 572.0f * fact },
   { 771.0f * fact, 572.0f * fact },
   { 771.0f * fact, 600.0f * fact }
};

static const SFG_StrokeStrip chr_ol_83_strip[] = { {53, chr_ol_83_part_0} };

SFG_StrokeChar chr_ol_83 = { 822 * fact, 1, chr_ol_83_strip };


// T_.glif
static const SFG_StrokeVertex chr_ol_84_part_0[] = {
   { 20.0f * fact, 720.0f * fact },
   { 20.0f * fact, 639.0f * fact },
   { 340.0f * fact, 639.0f * fact },
   { 340.0f * fact, 0.0f * fact },
   { 421.0f * fact, 0.0f * fact },
   { 421.0f * fact, 639.0f * fact },
   { 740.0f * fact, 639.0f * fact },
   { 740.0f * fact, 720.0f * fact },
   { 20.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_84_strip[] = { {9, chr_ol_84_part_0} };

SFG_StrokeChar chr_ol_84 = { 759 * fact, 1, chr_ol_84_strip };


// U_.glif
static const SFG_StrokeVertex chr_ol_85_part_0[] = {
   { 693.0f * fact, 120.0f * fact },
   { 693.0f * fact, 99.0f * fact },
   { 675.0f * fact, 81.0f * fact },
   { 654.0f * fact, 81.0f * fact },
   { 174.0f * fact, 81.0f * fact },
   { 153.0f * fact, 81.0f * fact },
   { 135.0f * fact, 99.0f * fact },
   { 135.0f * fact, 120.0f * fact },
   { 135.0f * fact, 720.0f * fact },
   { 54.0f * fact, 720.0f * fact },
   { 54.0f * fact, 120.0f * fact },
   { 54.0f * fact, 53.0f * fact },
   { 107.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 654.0f * fact, 0.0f * fact },
   { 721.0f * fact, 0.0f * fact },
   { 774.0f * fact, 53.0f * fact },
   { 774.0f * fact, 120.0f * fact },
   { 774.0f * fact, 720.0f * fact },
   { 693.0f * fact, 720.0f * fact },
   { 693.0f * fact, 120.0f * fact }
};

static const SFG_StrokeStrip chr_ol_85_strip[] = { {21, chr_ol_85_part_0} };

SFG_StrokeChar chr_ol_85 = { 828 * fact, 1, chr_ol_85_strip };


// V_.glif
static const SFG_StrokeVertex chr_ol_86_part_0[] = {
   { 499.0f * fact, 79.0f * fact },
   { 130.0f * fact, 720.0f * fact },
   { 35.0f * fact, 720.0f * fact },
   { 173.0f * fact, 478.0f * fact },
   { 312.0f * fact, 242.0f * fact },
   { 452.0f * fact, 0.0f * fact },
   { 546.0f * fact, 0.0f * fact },
   { 689.0f * fact, 248.0f * fact },
   { 819.0f * fact, 471.0f * fact },
   { 962.0f * fact, 720.0f * fact },
   { 867.0f * fact, 720.0f * fact },
   { 499.0f * fact, 79.0f * fact }
};

static const SFG_StrokeStrip chr_ol_86_strip[] = { {12, chr_ol_86_part_0} };

SFG_StrokeChar chr_ol_86 = { 1003 * fact, 1, chr_ol_86_strip };


// W_.glif
static const SFG_StrokeVertex chr_ol_87_part_0[] = {
   { 1060.0f * fact, 720.0f * fact },
   { 996.0f * fact, 541.0f * fact },
   { 914.0f * fact, 319.0f * fact },
   { 851.0f * fact, 142.0f * fact },
   { 640.0f * fact, 720.0f * fact },
   { 542.0f * fact, 720.0f * fact },
   { 477.0f * fact, 541.0f * fact },
   { 396.0f * fact, 319.0f * fact },
   { 332.0f * fact, 142.0f * fact },
   { 121.0f * fact, 720.0f * fact },
   { 35.0f * fact, 720.0f * fact },
   { 297.0f * fact, 0.0f * fact },
   { 367.0f * fact, 0.0f * fact },
   { 591.0f * fact, 615.0f * fact },
   { 815.0f * fact, 0.0f * fact },
   { 886.0f * fact, 0.0f * fact },
   { 1148.0f * fact, 720.0f * fact },
   { 1060.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_87_strip[] = { {18, chr_ol_87_part_0} };

SFG_StrokeChar chr_ol_87 = { 1179 * fact, 1, chr_ol_87_strip };


// X_.glif
static const SFG_StrokeVertex chr_ol_88_part_0[] = {
   { 651.0f * fact, 720.0f * fact },
   { 401.0f * fact, 423.0f * fact },
   { 150.0f * fact, 720.0f * fact },
   { 46.0f * fact, 720.0f * fact },
   { 348.0f * fact, 360.0f * fact },
   { 46.0f * fact, 0.0f * fact },
   { 150.0f * fact, 0.0f * fact },
   { 401.0f * fact, 297.0f * fact },
   { 651.0f * fact, 0.0f * fact },
   { 756.0f * fact, 0.0f * fact },
   { 656.0f * fact, 117.0f * fact },
   { 550.0f * fact, 243.0f * fact },
   { 453.0f * fact, 360.0f * fact },
   { 755.0f * fact, 720.0f * fact },
   { 651.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_88_strip[] = { {15, chr_ol_88_part_0} };

SFG_StrokeChar chr_ol_88 = { 812 * fact, 1, chr_ol_88_strip };


// Y_.glif
static const SFG_StrokeVertex chr_ol_89_part_0[] = {
   { 665.0f * fact, 720.0f * fact },
   { 577.0f * fact, 608.0f * fact },
   { 476.0f * fact, 480.0f * fact },
   { 389.0f * fact, 368.0f * fact },
   { 110.0f * fact, 720.0f * fact },
   { 17.0f * fact, 720.0f * fact },
   { 126.0f * fact, 573.0f * fact },
   { 241.0f * fact, 418.0f * fact },
   { 348.0f * fact, 271.0f * fact },
   { 348.0f * fact, 0.0f * fact },
   { 429.0f * fact, 0.0f * fact },
   { 429.0f * fact, 271.0f * fact },
   { 760.0f * fact, 720.0f * fact },
   { 665.0f * fact, 720.0f * fact }
};

static const SFG_StrokeStrip chr_ol_89_strip[] = { {14, chr_ol_89_part_0} };

SFG_StrokeChar chr_ol_89 = { 806 * fact, 1, chr_ol_89_strip };


// Z_.glif
static const SFG_StrokeVertex chr_ol_90_part_0[] = {
   { 51.0f * fact, 639.0f * fact },
   { 679.0f * fact, 639.0f * fact },
   { 51.0f * fact, 111.0f * fact },
   { 51.0f * fact, 0.0f * fact },
   { 771.0f * fact, 0.0f * fact },
   { 771.0f * fact, 81.0f * fact },
   { 143.0f * fact, 81.0f * fact },
   { 771.0f * fact, 609.0f * fact },
   { 771.0f * fact, 720.0f * fact },
   { 51.0f * fact, 720.0f * fact },
   { 51.0f * fact, 639.0f * fact }
};

static const SFG_StrokeStrip chr_ol_90_strip[] = { {11, chr_ol_90_part_0} };

SFG_StrokeChar chr_ol_90 = { 821 * fact, 1, chr_ol_90_strip };


// bracketleft.glif
static const SFG_StrokeVertex chr_ol_91_part_0[] = {
   { 54.0f * fact, 0.0f * fact },
   { 204.0f * fact, 0.0f * fact },
   { 204.0f * fact, 82.0f * fact },
   { 136.0f * fact, 82.0f * fact },
   { 136.0f * fact, 638.0f * fact },
   { 204.0f * fact, 638.0f * fact },
   { 204.0f * fact, 721.0f * fact },
   { 54.0f * fact, 721.0f * fact },
   { 54.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_ol_91_strip[] = { {9, chr_ol_91_part_0} };

SFG_StrokeChar chr_ol_91 = { 275 * fact, 1, chr_ol_91_strip };


// backslash.glif
static const SFG_StrokeVertex chr_ol_92_part_0[] = {
   { 511.0f * fact, 107.0f * fact },
   { 5.0f * fact, 725.0f * fact },
   { 5.0f * fact, 621.0f * fact },
   { 511.0f * fact, 1.0f * fact },
   { 511.0f * fact, 107.0f * fact }
};

static const SFG_StrokeStrip chr_ol_92_strip[] = { {5, chr_ol_92_part_0} };

SFG_StrokeChar chr_ol_92 = { 520 * fact, 1, chr_ol_92_strip };


// bracketright.glif
static const SFG_StrokeVertex chr_ol_93_part_0[] = {
   { 51.0f * fact, 638.0f * fact },
   { 118.0f * fact, 638.0f * fact },
   { 118.0f * fact, 82.0f * fact },
   { 51.0f * fact, 82.0f * fact },
   { 51.0f * fact, 0.0f * fact },
   { 201.0f * fact, 0.0f * fact },
   { 201.0f * fact, 721.0f * fact },
   { 51.0f * fact, 721.0f * fact },
   { 51.0f * fact, 638.0f * fact }
};

static const SFG_StrokeStrip chr_ol_93_strip[] = { {9, chr_ol_93_part_0} };

SFG_StrokeChar chr_ol_93 = { 276 * fact, 1, chr_ol_93_strip };


// underscore.glif
static const SFG_StrokeVertex chr_ol_95_part_0[] = {
   { 782.0f * fact, 0.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 54.0f * fact, -82.0f * fact },
   { 782.0f * fact, -82.0f * fact },
   { 782.0f * fact, 0.0f * fact }
};

static const SFG_StrokeStrip chr_ol_95_strip[] = { {5, chr_ol_95_part_0} };

SFG_StrokeChar chr_ol_95 = { 828 * fact, 1, chr_ol_95_strip };


// grave.glif
static const SFG_StrokeVertex chr_ol_96_part_0[] = {
   { 113.0f * fact, 1011.0f * fact },
   { 32.0f * fact, 1011.0f * fact },
   { 45.0f * fact, 960.0f * fact },
   { 54.0f * fact, 926.0f * fact },
   { 67.0f * fact, 872.0f * fact },
   { 147.0f * fact, 872.0f * fact },
   { 113.0f * fact, 1011.0f * fact }
};

static const SFG_StrokeStrip chr_ol_96_strip[] = { {7, chr_ol_96_part_0} };

SFG_StrokeChar chr_ol_96 = { 213 * fact, 1, chr_ol_96_strip };


// a.glif
static const SFG_StrokeVertex chr_ol_97_part_0[] = {
   { 52.0f * fact, 580.0f * fact },
   { 52.0f * fact, 498.0f * fact },
   { 521.0f * fact, 498.0f * fact },
   { 543.0f * fact, 498.0f * fact },
   { 560.0f * fact, 480.0f * fact },
   { 560.0f * fact, 459.0f * fact },
   { 560.0f * fact, 331.0f * fact },
   { 52.0f * fact, 331.0f * fact },
   { 52.0f * fact, 121.0f * fact },
   { 52.0f * fact, 54.0f * fact },
   { 106.0f * fact, 0.0f * fact },
   { 173.0f * fact, 0.0f * fact },
   { 643.0f * fact, 0.0f * fact },
   { 643.0f * fact, 459.0f * fact },
   { 643.0f * fact, 526.0f * fact },
   { 589.0f * fact, 580.0f * fact },
   { 521.0f * fact, 580.0f * fact },
   { 52.0f * fact, 580.0f * fact }
};

// a.glif
static const SFG_StrokeVertex chr_ol_97_part_1[] = {
   { 560.0f * fact, 82.0f * fact },
   { 173.0f * fact, 82.0f * fact },
   { 152.0f * fact, 82.0f * fact },
   { 134.0f * fact, 100.0f * fact },
   { 134.0f * fact, 121.0f * fact },
   { 134.0f * fact, 249.0f * fact },
   { 560.0f * fact, 249.0f * fact },
   { 560.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_97_strip[] = { {18, chr_ol_97_part_0}, {8, chr_ol_97_part_1} };

SFG_StrokeChar chr_ol_97 = { 694 * fact, 2, chr_ol_97_strip };


// b.glif
static const SFG_StrokeVertex chr_ol_98_part_0[] = {
   { 136.0f * fact, 580.0f * fact },
   { 136.0f * fact, 770.0f * fact },
   { 54.0f * fact, 770.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 524.0f * fact, 0.0f * fact },
   { 591.0f * fact, 0.0f * fact },
   { 645.0f * fact, 54.0f * fact },
   { 645.0f * fact, 121.0f * fact },
   { 645.0f * fact, 459.0f * fact },
   { 645.0f * fact, 526.0f * fact },
   { 591.0f * fact, 580.0f * fact },
   { 524.0f * fact, 580.0f * fact },
   { 136.0f * fact, 580.0f * fact }
};

// b.glif
static const SFG_StrokeVertex chr_ol_98_part_1[] = {
   { 563.0f * fact, 121.0f * fact },
   { 563.0f * fact, 100.0f * fact },
   { 545.0f * fact, 82.0f * fact },
   { 524.0f * fact, 82.0f * fact },
   { 176.0f * fact, 82.0f * fact },
   { 154.0f * fact, 82.0f * fact },
   { 136.0f * fact, 100.0f * fact },
   { 136.0f * fact, 121.0f * fact },
   { 136.0f * fact, 459.0f * fact },
   { 136.0f * fact, 480.0f * fact },
   { 154.0f * fact, 498.0f * fact },
   { 176.0f * fact, 498.0f * fact },
   { 524.0f * fact, 498.0f * fact },
   { 545.0f * fact, 498.0f * fact },
   { 563.0f * fact, 480.0f * fact },
   { 563.0f * fact, 459.0f * fact },
   { 563.0f * fact, 121.0f * fact }
};

static const SFG_StrokeStrip chr_ol_98_strip[] = { {13, chr_ol_98_part_0}, {17, chr_ol_98_part_1} };

SFG_StrokeChar chr_ol_98 = { 667 * fact, 2, chr_ol_98_strip };


// c.glif
static const SFG_StrokeVertex chr_ol_99_part_0[] = {
   { 172.0f * fact, 82.0f * fact },
   { 151.0f * fact, 82.0f * fact },
   { 133.0f * fact, 100.0f * fact },
   { 133.0f * fact, 121.0f * fact },
   { 133.0f * fact, 459.0f * fact },
   { 133.0f * fact, 480.0f * fact },
   { 151.0f * fact, 498.0f * fact },
   { 172.0f * fact, 498.0f * fact },
   { 640.0f * fact, 498.0f * fact },
   { 640.0f * fact, 580.0f * fact },
   { 172.0f * fact, 580.0f * fact },
   { 105.0f * fact, 580.0f * fact },
   { 51.0f * fact, 526.0f * fact },
   { 51.0f * fact, 459.0f * fact },
   { 51.0f * fact, 121.0f * fact },
   { 51.0f * fact, 54.0f * fact },
   { 105.0f * fact, 0.0f * fact },
   { 172.0f * fact, 0.0f * fact },
   { 642.0f * fact, 0.0f * fact },
   { 642.0f * fact, 82.0f * fact },
   { 172.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_99_strip[] = { {21, chr_ol_99_part_0} };

SFG_StrokeChar chr_ol_99 = { 695 * fact, 1, chr_ol_99_strip };


// d.glif
static const SFG_StrokeVertex chr_ol_100_part_0[] = {
   { 532.0f * fact, 770.0f * fact },
   { 532.0f * fact, 580.0f * fact },
   { 145.0f * fact, 580.0f * fact },
   { 77.0f * fact, 580.0f * fact },
   { 23.0f * fact, 526.0f * fact },
   { 23.0f * fact, 459.0f * fact },
   { 23.0f * fact, 121.0f * fact },
   { 23.0f * fact, 54.0f * fact },
   { 77.0f * fact, 0.0f * fact },
   { 145.0f * fact, 0.0f * fact },
   { 614.0f * fact, 0.0f * fact },
   { 614.0f * fact, 770.0f * fact },
   { 532.0f * fact, 770.0f * fact }
};

// d.glif
static const SFG_StrokeVertex chr_ol_100_part_1[] = {
   { 145.0f * fact, 82.0f * fact },
   { 124.0f * fact, 82.0f * fact },
   { 106.0f * fact, 100.0f * fact },
   { 106.0f * fact, 121.0f * fact },
   { 106.0f * fact, 459.0f * fact },
   { 106.0f * fact, 480.0f * fact },
   { 124.0f * fact, 498.0f * fact },
   { 145.0f * fact, 498.0f * fact },
   { 493.0f * fact, 498.0f * fact },
   { 514.0f * fact, 498.0f * fact },
   { 532.0f * fact, 480.0f * fact },
   { 532.0f * fact, 459.0f * fact },
   { 532.0f * fact, 121.0f * fact },
   { 532.0f * fact, 100.0f * fact },
   { 514.0f * fact, 82.0f * fact },
   { 493.0f * fact, 82.0f * fact },
   { 145.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_100_strip[] = { {13, chr_ol_100_part_0}, {17, chr_ol_100_part_1} };

SFG_StrokeChar chr_ol_100 = { 667 * fact, 2, chr_ol_100_strip };


// e.glif
static const SFG_StrokeVertex chr_ol_101_part_0[] = {
   { 172.0f * fact, 580.0f * fact },
   { 105.0f * fact, 580.0f * fact },
   { 51.0f * fact, 526.0f * fact },
   { 51.0f * fact, 459.0f * fact },
   { 51.0f * fact, 121.0f * fact },
   { 51.0f * fact, 54.0f * fact },
   { 105.0f * fact, 0.0f * fact },
   { 172.0f * fact, 0.0f * fact },
   { 642.0f * fact, 0.0f * fact },
   { 642.0f * fact, 82.0f * fact },
   { 172.0f * fact, 82.0f * fact },
   { 151.0f * fact, 82.0f * fact },
   { 133.0f * fact, 100.0f * fact },
   { 133.0f * fact, 121.0f * fact },
   { 133.0f * fact, 249.0f * fact },
   { 642.0f * fact, 249.0f * fact },
   { 642.0f * fact, 459.0f * fact },
   { 642.0f * fact, 526.0f * fact },
   { 588.0f * fact, 580.0f * fact },
   { 520.0f * fact, 580.0f * fact },
   { 172.0f * fact, 580.0f * fact }
};

// e.glif
static const SFG_StrokeVertex chr_ol_101_part_1[] = {
   { 133.0f * fact, 331.0f * fact },
   { 133.0f * fact, 459.0f * fact },
   { 133.0f * fact, 480.0f * fact },
   { 151.0f * fact, 498.0f * fact },
   { 172.0f * fact, 498.0f * fact },
   { 520.0f * fact, 498.0f * fact },
   { 542.0f * fact, 498.0f * fact },
   { 559.0f * fact, 480.0f * fact },
   { 559.0f * fact, 459.0f * fact },
   { 559.0f * fact, 331.0f * fact },
   { 133.0f * fact, 331.0f * fact }
};

static const SFG_StrokeStrip chr_ol_101_strip[] = { {21, chr_ol_101_part_0}, {11, chr_ol_101_part_1} };

SFG_StrokeChar chr_ol_101 = { 692 * fact, 2, chr_ol_101_strip };


// f.glif
static const SFG_StrokeVertex chr_ol_102_part_0[] = {
   { 386.0f * fact, 688.0f * fact },
   { 386.0f * fact, 770.0f * fact },
   { 174.0f * fact, 770.0f * fact },
   { 106.0f * fact, 770.0f * fact },
   { 53.0f * fact, 717.0f * fact },
   { 53.0f * fact, 649.0f * fact },
   { 53.0f * fact, 0.0f * fact },
   { 135.0f * fact, 0.0f * fact },
   { 135.0f * fact, 498.0f * fact },
   { 386.0f * fact, 498.0f * fact },
   { 386.0f * fact, 580.0f * fact },
   { 135.0f * fact, 580.0f * fact },
   { 135.0f * fact, 649.0f * fact },
   { 135.0f * fact, 670.0f * fact },
   { 153.0f * fact, 688.0f * fact },
   { 174.0f * fact, 688.0f * fact },
   { 386.0f * fact, 688.0f * fact }
};

static const SFG_StrokeStrip chr_ol_102_strip[] = { {17, chr_ol_102_part_0} };

SFG_StrokeChar chr_ol_102 = { 407 * fact, 1, chr_ol_102_strip };


// g.glif
static const SFG_StrokeVertex chr_ol_103_part_0[] = {
   { 631.0f * fact, 459.0f * fact },
   { 631.0f * fact, 526.0f * fact },
   { 578.0f * fact, 580.0f * fact },
   { 510.0f * fact, 580.0f * fact },
   { 162.0f * fact, 580.0f * fact },
   { 95.0f * fact, 580.0f * fact },
   { 41.0f * fact, 526.0f * fact },
   { 41.0f * fact, 459.0f * fact },
   { 41.0f * fact, 121.0f * fact },
   { 41.0f * fact, 54.0f * fact },
   { 95.0f * fact, 0.0f * fact },
   { 162.0f * fact, 0.0f * fact },
   { 549.0f * fact, 0.0f * fact },
   { 549.0f * fact, -107.0f * fact },
   { 549.0f * fact, -128.0f * fact },
   { 531.0f * fact, -146.0f * fact },
   { 510.0f * fact, -146.0f * fact },
   { 148.0f * fact, -146.0f * fact },
   { 148.0f * fact, -229.0f * fact },
   { 510.0f * fact, -229.0f * fact },
   { 578.0f * fact, -229.0f * fact },
   { 631.0f * fact, -175.0f * fact },
   { 631.0f * fact, -107.0f * fact },
   { 631.0f * fact, 459.0f * fact }
};

// g.glif
static const SFG_StrokeVertex chr_ol_103_part_1[] = {
   { 162.0f * fact, 82.0f * fact },
   { 141.0f * fact, 82.0f * fact },
   { 123.0f * fact, 100.0f * fact },
   { 123.0f * fact, 121.0f * fact },
   { 123.0f * fact, 459.0f * fact },
   { 123.0f * fact, 480.0f * fact },
   { 141.0f * fact, 498.0f * fact },
   { 162.0f * fact, 498.0f * fact },
   { 510.0f * fact, 498.0f * fact },
   { 531.0f * fact, 498.0f * fact },
   { 549.0f * fact, 480.0f * fact },
   { 549.0f * fact, 459.0f * fact },
   { 549.0f * fact, 121.0f * fact },
   { 549.0f * fact, 100.0f * fact },
   { 531.0f * fact, 82.0f * fact },
   { 510.0f * fact, 82.0f * fact },
   { 162.0f * fact, 82.0f * fact }
};

static const SFG_StrokeStrip chr_ol_103_strip[] = { {24, chr_ol_103_part_0}, {17, chr_ol_103_part_1} };

SFG_StrokeChar chr_ol_103 = { 683 * fact, 2, chr_ol_103_strip };


// h.glif
static const SFG_StrokeVertex chr_ol_104_part_0[] = {
   { 136.0f * fact, 580.0f * fact },
   { 136.0f * fact, 770.0f * fact },
   { 54.0f * fact, 770.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 136.0f * fact, 459.0f * fact },
   { 136.0f * fact, 480.0f * fact },
   { 154.0f * fact, 498.0f * fact },
   { 176.0f * fact, 498.0f * fact },
   { 524.0f * fact, 498.0f * fact },
   { 545.0f * fact, 498.0f * fact },
   { 563.0f * fact, 480.0f * fact },
   { 563.0f * fact, 459.0f * fact },
   { 563.0f * fact, 0.0f * fact },
   { 645.0f * fact, 0.0f * fact },
   { 645.0f * fact, 459.0f * fact },
   { 645.0f * fact, 526.0f * fact },
   { 590.0f * fact, 580.0f * fact },
   { 524.0f * fact, 580.0f * fact },
   { 136.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_104_strip[] = { {20, chr_ol_104_part_0} };

SFG_StrokeChar chr_ol_104 = { 668 * fact, 1, chr_ol_104_strip };


// i.glif
static const SFG_StrokeVertex chr_ol_105_part_0[] = {
   { 52.0f * fact, 0.0f * fact },
   { 134.0f * fact, 0.0f * fact },
   { 134.0f * fact, 580.0f * fact },
   { 52.0f * fact, 580.0f * fact },
   { 52.0f * fact, 0.0f * fact }
};

// i.glif
static const SFG_StrokeVertex chr_ol_105_part_1[] = {
   { 52.0f * fact, 770.0f * fact },
   { 52.0f * fact, 688.0f * fact },
   { 134.0f * fact, 688.0f * fact },
   { 134.0f * fact, 770.0f * fact },
   { 52.0f * fact, 770.0f * fact }
};

static const SFG_StrokeStrip chr_ol_105_strip[] = { {5, chr_ol_105_part_0}, {5, chr_ol_105_part_1} };

SFG_StrokeChar chr_ol_105 = { 208 * fact, 2, chr_ol_105_strip };


// j.glif
static const SFG_StrokeVertex chr_ol_106_part_0[] = {
   { 97.0f * fact, 770.0f * fact },
   { 97.0f * fact, 688.0f * fact },
   { 180.0f * fact, 688.0f * fact },
   { 180.0f * fact, 770.0f * fact },
   { 97.0f * fact, 770.0f * fact }
};

// j.glif
static const SFG_StrokeVertex chr_ol_106_part_1[] = {
   { 180.0f * fact, 580.0f * fact },
   { 97.0f * fact, 580.0f * fact },
   { 97.0f * fact, -121.0f * fact },
   { 97.0f * fact, -142.0f * fact },
   { 79.0f * fact, -160.0f * fact },
   { 58.0f * fact, -160.0f * fact },
   { -187.0f * fact, -160.0f * fact },
   { -187.0f * fact, -243.0f * fact },
   { 58.0f * fact, -243.0f * fact },
   { 126.0f * fact, -243.0f * fact },
   { 180.0f * fact, -189.0f * fact },
   { 180.0f * fact, -121.0f * fact },
   { 180.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_106_strip[] = { {5, chr_ol_106_part_0}, {13, chr_ol_106_part_1} };

SFG_StrokeChar chr_ol_106 = { 239 * fact, 2, chr_ol_106_strip };


// k.glif
static const SFG_StrokeVertex chr_ol_107_part_0[] = {
   { 524.0f * fact, 580.0f * fact },
   { 296.0f * fact, 331.0f * fact },
   { 136.0f * fact, 331.0f * fact },
   { 136.0f * fact, 770.0f * fact },
   { 54.0f * fact, 770.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 136.0f * fact, 249.0f * fact },
   { 296.0f * fact, 249.0f * fact },
   { 524.0f * fact, 0.0f * fact },
   { 631.0f * fact, 0.0f * fact },
   { 369.0f * fact, 290.0f * fact },
   { 632.0f * fact, 580.0f * fact },
   { 524.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_107_strip[] = { {14, chr_ol_107_part_0} };

SFG_StrokeChar chr_ol_107 = { 646 * fact, 1, chr_ol_107_strip };


// l.glif
static const SFG_StrokeVertex chr_ol_108_part_0[] = {
   { 52.0f * fact, 770.0f * fact },
   { 52.0f * fact, 121.0f * fact },
   { 52.0f * fact, 54.0f * fact },
   { 106.0f * fact, 0.0f * fact },
   { 173.0f * fact, 0.0f * fact },
   { 271.0f * fact, 0.0f * fact },
   { 271.0f * fact, 82.0f * fact },
   { 173.0f * fact, 82.0f * fact },
   { 152.0f * fact, 82.0f * fact },
   { 134.0f * fact, 100.0f * fact },
   { 134.0f * fact, 121.0f * fact },
   { 134.0f * fact, 770.0f * fact },
   { 52.0f * fact, 770.0f * fact }
};

static const SFG_StrokeStrip chr_ol_108_strip[] = { {13, chr_ol_108_part_0} };

SFG_StrokeChar chr_ol_108 = { 302 * fact, 1, chr_ol_108_strip };


// m.glif
static const SFG_StrokeVertex chr_ol_109_part_0[] = {
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 137.0f * fact, 0.0f * fact },
   { 137.0f * fact, 459.0f * fact },
   { 137.0f * fact, 480.0f * fact },
   { 155.0f * fact, 498.0f * fact },
   { 176.0f * fact, 498.0f * fact },
   { 418.0f * fact, 498.0f * fact },
   { 439.0f * fact, 498.0f * fact },
   { 457.0f * fact, 480.0f * fact },
   { 457.0f * fact, 459.0f * fact },
   { 457.0f * fact, 0.0f * fact },
   { 540.0f * fact, 0.0f * fact },
   { 540.0f * fact, 459.0f * fact },
   { 540.0f * fact, 480.0f * fact },
   { 557.0f * fact, 498.0f * fact },
   { 579.0f * fact, 498.0f * fact },
   { 820.0f * fact, 498.0f * fact },
   { 842.0f * fact, 498.0f * fact },
   { 860.0f * fact, 480.0f * fact },
   { 860.0f * fact, 459.0f * fact },
   { 860.0f * fact, 0.0f * fact },
   { 941.0f * fact, 0.0f * fact },
   { 941.0f * fact, 459.0f * fact },
   { 941.0f * fact, 526.0f * fact },
   { 888.0f * fact, 580.0f * fact },
   { 820.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_109_strip[] = { {28, chr_ol_109_part_0} };

SFG_StrokeChar chr_ol_109 = { 978 * fact, 1, chr_ol_109_strip };


// n.glif
static const SFG_StrokeVertex chr_ol_110_part_0[] = {
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 136.0f * fact, 459.0f * fact },
   { 136.0f * fact, 480.0f * fact },
   { 154.0f * fact, 498.0f * fact },
   { 176.0f * fact, 498.0f * fact },
   { 524.0f * fact, 498.0f * fact },
   { 545.0f * fact, 498.0f * fact },
   { 563.0f * fact, 480.0f * fact },
   { 563.0f * fact, 459.0f * fact },
   { 563.0f * fact, 0.0f * fact },
   { 645.0f * fact, 0.0f * fact },
   { 645.0f * fact, 459.0f * fact },
   { 645.0f * fact, 526.0f * fact },
   { 591.0f * fact, 580.0f * fact },
   { 524.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_110_strip[] = { {18, chr_ol_110_part_0} };

SFG_StrokeChar chr_ol_110 = { 696 * fact, 1, chr_ol_110_strip };


// o.glif
static const SFG_StrokeVertex chr_ol_111_part_0[] = {
   { 172.0f * fact, 580.0f * fact },
   { 105.0f * fact, 580.0f * fact },
   { 51.0f * fact, 526.0f * fact },
   { 51.0f * fact, 459.0f * fact },
   { 51.0f * fact, 121.0f * fact },
   { 51.0f * fact, 54.0f * fact },
   { 105.0f * fact, 0.0f * fact },
   { 172.0f * fact, 0.0f * fact },
   { 520.0f * fact, 0.0f * fact },
   { 588.0f * fact, 0.0f * fact },
   { 642.0f * fact, 54.0f * fact },
   { 642.0f * fact, 121.0f * fact },
   { 642.0f * fact, 459.0f * fact },
   { 642.0f * fact, 526.0f * fact },
   { 588.0f * fact, 580.0f * fact },
   { 520.0f * fact, 580.0f * fact },
   { 172.0f * fact, 580.0f * fact }
};

// o.glif
static const SFG_StrokeVertex chr_ol_111_part_1[] = {
   { 520.0f * fact, 498.0f * fact },
   { 542.0f * fact, 498.0f * fact },
   { 559.0f * fact, 480.0f * fact },
   { 559.0f * fact, 459.0f * fact },
   { 559.0f * fact, 121.0f * fact },
   { 559.0f * fact, 100.0f * fact },
   { 542.0f * fact, 82.0f * fact },
   { 520.0f * fact, 82.0f * fact },
   { 172.0f * fact, 82.0f * fact },
   { 151.0f * fact, 82.0f * fact },
   { 133.0f * fact, 100.0f * fact },
   { 133.0f * fact, 121.0f * fact },
   { 133.0f * fact, 459.0f * fact },
   { 133.0f * fact, 480.0f * fact },
   { 151.0f * fact, 498.0f * fact },
   { 172.0f * fact, 498.0f * fact },
   { 520.0f * fact, 498.0f * fact }
};

static const SFG_StrokeStrip chr_ol_111_strip[] = { {17, chr_ol_111_part_0}, {17, chr_ol_111_part_1} };

SFG_StrokeChar chr_ol_111 = { 692 * fact, 2, chr_ol_111_strip };


// p.glif
static const SFG_StrokeVertex chr_ol_112_part_0[] = {
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, -230.0f * fact },
   { 136.0f * fact, -230.0f * fact },
   { 136.0f * fact, 0.0f * fact },
   { 524.0f * fact, 0.0f * fact },
   { 591.0f * fact, 0.0f * fact },
   { 645.0f * fact, 54.0f * fact },
   { 645.0f * fact, 121.0f * fact },
   { 645.0f * fact, 459.0f * fact },
   { 645.0f * fact, 526.0f * fact },
   { 591.0f * fact, 580.0f * fact },
   { 524.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact }
};

// p.glif
static const SFG_StrokeVertex chr_ol_112_part_1[] = {
   { 563.0f * fact, 121.0f * fact },
   { 563.0f * fact, 100.0f * fact },
   { 545.0f * fact, 82.0f * fact },
   { 524.0f * fact, 82.0f * fact },
   { 176.0f * fact, 82.0f * fact },
   { 154.0f * fact, 82.0f * fact },
   { 136.0f * fact, 100.0f * fact },
   { 136.0f * fact, 121.0f * fact },
   { 136.0f * fact, 459.0f * fact },
   { 136.0f * fact, 480.0f * fact },
   { 154.0f * fact, 498.0f * fact },
   { 176.0f * fact, 498.0f * fact },
   { 524.0f * fact, 498.0f * fact },
   { 545.0f * fact, 498.0f * fact },
   { 563.0f * fact, 480.0f * fact },
   { 563.0f * fact, 459.0f * fact },
   { 563.0f * fact, 121.0f * fact }
};

static const SFG_StrokeStrip chr_ol_112_strip[] = { {13, chr_ol_112_part_0}, {17, chr_ol_112_part_1} };

SFG_StrokeChar chr_ol_112 = { 664 * fact, 2, chr_ol_112_strip };


// q.glif
static const SFG_StrokeVertex chr_ol_113_part_0[] = {
   { 20.0f * fact, 121.0f * fact },
   { 20.0f * fact, 54.0f * fact },
   { 74.0f * fact, 0.0f * fact },
   { 142.0f * fact, 0.0f * fact },
   { 529.0f * fact, 0.0f * fact },
   { 529.0f * fact, -230.0f * fact },
   { 611.0f * fact, -230.0f * fact },
   { 611.0f * fact, 580.0f * fact },
   { 142.0f * fact, 580.0f * fact },
   { 74.0f * fact, 580.0f * fact },
   { 20.0f * fact, 526.0f * fact },
   { 20.0f * fact, 459.0f * fact },
   { 20.0f * fact, 121.0f * fact }
};

// q.glif
static const SFG_StrokeVertex chr_ol_113_part_1[] = {
   { 103.0f * fact, 459.0f * fact },
   { 103.0f * fact, 480.0f * fact },
   { 120.0f * fact, 498.0f * fact },
   { 142.0f * fact, 498.0f * fact },
   { 490.0f * fact, 498.0f * fact },
   { 511.0f * fact, 498.0f * fact },
   { 529.0f * fact, 480.0f * fact },
   { 529.0f * fact, 459.0f * fact },
   { 529.0f * fact, 121.0f * fact },
   { 529.0f * fact, 100.0f * fact },
   { 511.0f * fact, 82.0f * fact },
   { 490.0f * fact, 82.0f * fact },
   { 142.0f * fact, 82.0f * fact },
   { 120.0f * fact, 82.0f * fact },
   { 103.0f * fact, 100.0f * fact },
   { 103.0f * fact, 121.0f * fact },
   { 103.0f * fact, 459.0f * fact }
};

static const SFG_StrokeStrip chr_ol_113_strip[] = { {13, chr_ol_113_part_0}, {17, chr_ol_113_part_1} };

SFG_StrokeChar chr_ol_113 = { 664 * fact, 2, chr_ol_113_strip };


// r.glif
static const SFG_StrokeVertex chr_ol_114_part_0[] = {
   { 173.0f * fact, 580.0f * fact },
   { 106.0f * fact, 580.0f * fact },
   { 52.0f * fact, 526.0f * fact },
   { 52.0f * fact, 459.0f * fact },
   { 52.0f * fact, 0.0f * fact },
   { 134.0f * fact, 0.0f * fact },
   { 134.0f * fact, 459.0f * fact },
   { 134.0f * fact, 480.0f * fact },
   { 152.0f * fact, 498.0f * fact },
   { 173.0f * fact, 498.0f * fact },
   { 499.0f * fact, 498.0f * fact },
   { 499.0f * fact, 580.0f * fact },
   { 173.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_114_strip[] = { {13, chr_ol_114_part_0} };

SFG_StrokeChar chr_ol_114 = { 512 * fact, 1, chr_ol_114_strip };


// s.glif
static const SFG_StrokeVertex chr_ol_115_part_0[] = {
   { 639.0f * fact, 459.0f * fact },
   { 639.0f * fact, 526.0f * fact },
   { 585.0f * fact, 580.0f * fact },
   { 517.0f * fact, 580.0f * fact },
   { 169.0f * fact, 580.0f * fact },
   { 102.0f * fact, 580.0f * fact },
   { 48.0f * fact, 526.0f * fact },
   { 48.0f * fact, 459.0f * fact },
   { 48.0f * fact, 370.0f * fact },
   { 48.0f * fact, 303.0f * fact },
   { 102.0f * fact, 249.0f * fact },
   { 169.0f * fact, 249.0f * fact },
   { 517.0f * fact, 249.0f * fact },
   { 538.0f * fact, 249.0f * fact },
   { 556.0f * fact, 231.0f * fact },
   { 556.0f * fact, 210.0f * fact },
   { 556.0f * fact, 121.0f * fact },
   { 556.0f * fact, 100.0f * fact },
   { 538.0f * fact, 82.0f * fact },
   { 517.0f * fact, 82.0f * fact },
   { 169.0f * fact, 82.0f * fact },
   { 148.0f * fact, 82.0f * fact },
   { 130.0f * fact, 100.0f * fact },
   { 130.0f * fact, 121.0f * fact },
   { 130.0f * fact, 129.0f * fact },
   { 48.0f * fact, 129.0f * fact },
   { 48.0f * fact, 121.0f * fact },
   { 48.0f * fact, 54.0f * fact },
   { 102.0f * fact, 0.0f * fact },
   { 169.0f * fact, 0.0f * fact },
   { 517.0f * fact, 0.0f * fact },
   { 585.0f * fact, 0.0f * fact },
   { 639.0f * fact, 54.0f * fact },
   { 639.0f * fact, 121.0f * fact },
   { 639.0f * fact, 210.0f * fact },
   { 639.0f * fact, 277.0f * fact },
   { 585.0f * fact, 331.0f * fact },
   { 517.0f * fact, 331.0f * fact },
   { 169.0f * fact, 331.0f * fact },
   { 148.0f * fact, 331.0f * fact },
   { 130.0f * fact, 349.0f * fact },
   { 130.0f * fact, 370.0f * fact },
   { 130.0f * fact, 459.0f * fact },
   { 130.0f * fact, 480.0f * fact },
   { 148.0f * fact, 498.0f * fact },
   { 169.0f * fact, 498.0f * fact },
   { 517.0f * fact, 498.0f * fact },
   { 538.0f * fact, 498.0f * fact },
   { 556.0f * fact, 480.0f * fact },
   { 556.0f * fact, 459.0f * fact },
   { 556.0f * fact, 451.0f * fact },
   { 639.0f * fact, 451.0f * fact },
   { 639.0f * fact, 459.0f * fact }
};

static const SFG_StrokeStrip chr_ol_115_strip[] = { {53, chr_ol_115_part_0} };

SFG_StrokeChar chr_ol_115 = { 686 * fact, 1, chr_ol_115_strip };


// t.glif
static const SFG_StrokeVertex chr_ol_116_part_0[] = {
   { 386.0f * fact, 498.0f * fact },
   { 386.0f * fact, 580.0f * fact },
   { 135.0f * fact, 580.0f * fact },
   { 135.0f * fact, 770.0f * fact },
   { 53.0f * fact, 770.0f * fact },
   { 53.0f * fact, 121.0f * fact },
   { 53.0f * fact, 54.0f * fact },
   { 106.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 386.0f * fact, 0.0f * fact },
   { 386.0f * fact, 82.0f * fact },
   { 174.0f * fact, 82.0f * fact },
   { 153.0f * fact, 82.0f * fact },
   { 135.0f * fact, 100.0f * fact },
   { 135.0f * fact, 121.0f * fact },
   { 135.0f * fact, 498.0f * fact },
   { 386.0f * fact, 498.0f * fact }
};

static const SFG_StrokeStrip chr_ol_116_strip[] = { {17, chr_ol_116_part_0} };

SFG_StrokeChar chr_ol_116 = { 410 * fact, 1, chr_ol_116_strip };


// u.glif
static const SFG_StrokeVertex chr_ol_117_part_0[] = {
   { 562.0f * fact, 580.0f * fact },
   { 562.0f * fact, 121.0f * fact },
   { 562.0f * fact, 100.0f * fact },
   { 544.0f * fact, 82.0f * fact },
   { 522.0f * fact, 82.0f * fact },
   { 174.0f * fact, 82.0f * fact },
   { 153.0f * fact, 82.0f * fact },
   { 135.0f * fact, 100.0f * fact },
   { 135.0f * fact, 121.0f * fact },
   { 135.0f * fact, 580.0f * fact },
   { 53.0f * fact, 580.0f * fact },
   { 53.0f * fact, 121.0f * fact },
   { 53.0f * fact, 54.0f * fact },
   { 107.0f * fact, 0.0f * fact },
   { 174.0f * fact, 0.0f * fact },
   { 522.0f * fact, 0.0f * fact },
   { 590.0f * fact, 0.0f * fact },
   { 644.0f * fact, 54.0f * fact },
   { 644.0f * fact, 121.0f * fact },
   { 644.0f * fact, 580.0f * fact },
   { 562.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_117_strip[] = { {21, chr_ol_117_part_0} };

SFG_StrokeChar chr_ol_117 = { 695 * fact, 1, chr_ol_117_strip };


// v.glif
static const SFG_StrokeVertex chr_ol_118_part_0[] = {
   { 661.0f * fact, 580.0f * fact },
   { 389.0f * fact, 80.0f * fact },
   { 117.0f * fact, 580.0f * fact },
   { 21.0f * fact, 580.0f * fact },
   { 127.0f * fact, 386.0f * fact },
   { 235.0f * fact, 195.0f * fact },
   { 341.0f * fact, 0.0f * fact },
   { 436.0f * fact, 0.0f * fact },
   { 757.0f * fact, 580.0f * fact },
   { 661.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_118_strip[] = { {10, chr_ol_118_part_0} };

SFG_StrokeChar chr_ol_118 = { 790 * fact, 1, chr_ol_118_strip };


// w.glif
static const SFG_StrokeVertex chr_ol_119_part_0[] = {
   { 957.0f * fact, 580.0f * fact },
   { 793.0f * fact, 149.0f * fact },
   { 594.0f * fact, 580.0f * fact },
   { 486.0f * fact, 580.0f * fact },
   { 299.0f * fact, 148.0f * fact },
   { 123.0f * fact, 580.0f * fact },
   { 35.0f * fact, 580.0f * fact },
   { 112.0f * fact, 384.0f * fact },
   { 185.0f * fact, 197.0f * fact },
   { 263.0f * fact, 0.0f * fact },
   { 335.0f * fact, 0.0f * fact },
   { 540.0f * fact, 479.0f * fact },
   { 756.0f * fact, 0.0f * fact },
   { 827.0f * fact, 0.0f * fact },
   { 1046.0f * fact, 580.0f * fact },
   { 957.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_119_strip[] = { {16, chr_ol_119_part_0} };

SFG_StrokeChar chr_ol_119 = { 1071 * fact, 1, chr_ol_119_strip };


// x.glif
static const SFG_StrokeVertex chr_ol_120_part_0[] = {
   { 531.0f * fact, 580.0f * fact },
   { 473.0f * fact, 513.0f * fact },
   { 398.0f * fact, 426.0f * fact },
   { 342.0f * fact, 360.0f * fact },
   { 154.0f * fact, 580.0f * fact },
   { 46.0f * fact, 580.0f * fact },
   { 288.0f * fact, 295.0f * fact },
   { 46.0f * fact, 0.0f * fact },
   { 154.0f * fact, 0.0f * fact },
   { 213.0f * fact, 72.0f * fact },
   { 284.0f * fact, 161.0f * fact },
   { 342.0f * fact, 232.0f * fact },
   { 531.0f * fact, 0.0f * fact },
   { 639.0f * fact, 0.0f * fact },
   { 396.0f * fact, 295.0f * fact },
   { 638.0f * fact, 580.0f * fact },
   { 531.0f * fact, 580.0f * fact }
};

static const SFG_StrokeStrip chr_ol_120_strip[] = { {17, chr_ol_120_part_0} };

SFG_StrokeChar chr_ol_120 = { 692 * fact, 1, chr_ol_120_strip };


// y.glif
static const SFG_StrokeVertex chr_ol_121_part_0[] = {
   { 632.0f * fact, 578.0f * fact },
   { 550.0f * fact, 578.0f * fact },
   { 550.0f * fact, 121.0f * fact },
   { 550.0f * fact, 100.0f * fact },
   { 532.0f * fact, 82.0f * fact },
   { 511.0f * fact, 82.0f * fact },
   { 163.0f * fact, 82.0f * fact },
   { 142.0f * fact, 82.0f * fact },
   { 124.0f * fact, 100.0f * fact },
   { 124.0f * fact, 121.0f * fact },
   { 124.0f * fact, 578.0f * fact },
   { 42.0f * fact, 578.0f * fact },
   { 42.0f * fact, 121.0f * fact },
   { 42.0f * fact, 54.0f * fact },
   { 96.0f * fact, 0.0f * fact },
   { 163.0f * fact, 0.0f * fact },
   { 550.0f * fact, 0.0f * fact },
   { 550.0f * fact, -108.0f * fact },
   { 550.0f * fact, -129.0f * fact },
   { 532.0f * fact, -147.0f * fact },
   { 511.0f * fact, -147.0f * fact },
   { 149.0f * fact, -147.0f * fact },
   { 149.0f * fact, -230.0f * fact },
   { 511.0f * fact, -230.0f * fact },
   { 579.0f * fact, -230.0f * fact },
   { 632.0f * fact, -176.0f * fact },
   { 632.0f * fact, -108.0f * fact },
   { 632.0f * fact, 578.0f * fact }
};

static const SFG_StrokeStrip chr_ol_121_strip[] = { {28, chr_ol_121_part_0} };

SFG_StrokeChar chr_ol_121 = { 685 * fact, 1, chr_ol_121_strip };


// z.glif
static const SFG_StrokeVertex chr_ol_122_part_0[] = {
   { 54.0f * fact, 498.0f * fact },
   { 555.0f * fact, 498.0f * fact },
   { 54.0f * fact, 112.0f * fact },
   { 54.0f * fact, 0.0f * fact },
   { 645.0f * fact, 0.0f * fact },
   { 645.0f * fact, 82.0f * fact },
   { 144.0f * fact, 82.0f * fact },
   { 645.0f * fact, 468.0f * fact },
   { 645.0f * fact, 580.0f * fact },
   { 54.0f * fact, 580.0f * fact },
   { 54.0f * fact, 498.0f * fact }
};

static const SFG_StrokeStrip chr_ol_122_strip[] = { {11, chr_ol_122_part_0} };

SFG_StrokeChar chr_ol_122 = { 698 * fact, 1, chr_ol_122_strip };


// braceleft.glif
static const SFG_StrokeVertex chr_ol_123_part_0[] = {
   { 152.0f * fact, 291.0f * fact },
   { 68.0f * fact, 360.0f * fact },
   { 152.0f * fact, 427.0f * fact },
   { 152.0f * fact, 598.0f * fact },
   { 152.0f * fact, 619.0f * fact },
   { 170.0f * fact, 637.0f * fact },
   { 191.0f * fact, 637.0f * fact },
   { 220.0f * fact, 637.0f * fact },
   { 220.0f * fact, 720.0f * fact },
   { 191.0f * fact, 720.0f * fact },
   { 124.0f * fact, 720.0f * fact },
   { 70.0f * fact, 666.0f * fact },
   { 70.0f * fact, 598.0f * fact },
   { 70.0f * fact, 434.0f * fact },
   { 23.0f * fact, 409.0f * fact },
   { 23.0f * fact, 313.0f * fact },
   { 38.0f * fact, 304.0f * fact },
   { 58.0f * fact, 291.0f * fact },
   { 70.0f * fact, 284.0f * fact },
   { 70.0f * fact, 121.0f * fact },
   { 70.0f * fact, 54.0f * fact },
   { 124.0f * fact, 0.0f * fact },
   { 191.0f * fact, 0.0f * fact },
   { 220.0f * fact, 0.0f * fact },
   { 220.0f * fact, 82.0f * fact },
   { 191.0f * fact, 82.0f * fact },
   { 170.0f * fact, 82.0f * fact },
   { 152.0f * fact, 100.0f * fact },
   { 152.0f * fact, 121.0f * fact },
   { 152.0f * fact, 291.0f * fact }
};

static const SFG_StrokeStrip chr_ol_123_strip[] = { {30, chr_ol_123_part_0} };

SFG_StrokeChar chr_ol_123 = { 289 * fact, 1, chr_ol_123_strip };


// bar.glif
static const SFG_StrokeVertex chr_ol_124_part_0[] = {
   { 54.0f * fact, -115.0f * fact },
   { 136.0f * fact, -115.0f * fact },
   { 136.0f * fact, 842.0f * fact },
   { 54.0f * fact, 842.0f * fact },
   { 54.0f * fact, -115.0f * fact }
};

static const SFG_StrokeStrip chr_ol_124_strip[] = { {5, chr_ol_124_part_0} };

SFG_StrokeChar chr_ol_124 = { 214 * fact, 1, chr_ol_124_strip };


// braceright.glif
static const SFG_StrokeVertex chr_ol_125_part_0[] = {
   { 119.0f * fact, 121.0f * fact },
   { 119.0f * fact, 100.0f * fact },
   { 101.0f * fact, 82.0f * fact },
   { 80.0f * fact, 82.0f * fact },
   { 51.0f * fact, 82.0f * fact },
   { 51.0f * fact, 0.0f * fact },
   { 80.0f * fact, 0.0f * fact },
   { 147.0f * fact, 0.0f * fact },
   { 201.0f * fact, 54.0f * fact },
   { 201.0f * fact, 121.0f * fact },
   { 201.0f * fact, 283.0f * fact },
   { 247.0f * fact, 312.0f * fact },
   { 247.0f * fact, 407.0f * fact },
   { 233.0f * fact, 415.0f * fact },
   { 215.0f * fact, 427.0f * fact },
   { 201.0f * fact, 434.0f * fact },
   { 201.0f * fact, 598.0f * fact },
   { 201.0f * fact, 666.0f * fact },
   { 147.0f * fact, 720.0f * fact },
   { 80.0f * fact, 720.0f * fact },
   { 51.0f * fact, 720.0f * fact },
   { 51.0f * fact, 637.0f * fact },
   { 80.0f * fact, 637.0f * fact },
   { 101.0f * fact, 637.0f * fact },
   { 119.0f * fact, 619.0f * fact },
   { 119.0f * fact, 598.0f * fact },
   { 119.0f * fact, 425.0f * fact },
   { 203.0f * fact, 359.0f * fact },
   { 119.0f * fact, 290.0f * fact },
   { 119.0f * fact, 121.0f * fact }
};

static const SFG_StrokeStrip chr_ol_125_strip[] = { {30, chr_ol_125_part_0} };

SFG_StrokeChar chr_ol_125 = { 289 * fact, 1, chr_ol_125_strip };


// asciitilde.glif
static const SFG_StrokeVertex chr_ol_126_part_0[] = {
   { 295.0f * fact, 283.0f * fact },
   { 220.0f * fact, 291.0f * fact },
   { 162.0f * fact, 354.0f * fact },
   { 86.0f * fact, 354.0f * fact },
   { 62.0f * fact, 354.0f * fact },
   { 42.0f * fact, 351.0f * fact },
   { 24.0f * fact, 345.0f * fact },
   { 24.0f * fact, 308.0f * fact },
   { 44.0f * fact, 315.0f * fact },
   { 70.0f * fact, 316.0f * fact },
   { 86.0f * fact, 316.0f * fact },
   { 167.0f * fact, 316.0f * fact },
   { 224.0f * fact, 245.0f * fact },
   { 297.0f * fact, 245.0f * fact },
   { 321.0f * fact, 245.0f * fact },
   { 344.0f * fact, 251.0f * fact },
   { 363.0f * fact, 258.0f * fact },
   { 363.0f * fact, 299.0f * fact },
   { 342.0f * fact, 289.0f * fact },
   { 320.0f * fact, 283.0f * fact },
   { 297.0f * fact, 283.0f * fact },
   { 295.0f * fact, 283.0f * fact }
};

static const SFG_StrokeStrip chr_ol_126_strip[] = { {22, chr_ol_126_part_0} };

SFG_StrokeChar chr_ol_126 = { 404 * fact, 1, chr_ol_126_strip };




static const SFG_StrokeChar *ol_chars[] = { 
   0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 
   &chr_ol_32, &chr_ol_33, &chr_ol_34, &chr_ol_35, &chr_ol_36, &chr_ol_37, &chr_ol_38, &chr_ol_39, 
   &chr_ol_40, &chr_ol_41, &chr_ol_42, &chr_ol_43, &chr_ol_44, &chr_ol_45, &chr_ol_46, &chr_ol_47, 
   &chr_ol_48, &chr_ol_49, &chr_ol_50, &chr_ol_51, &chr_ol_52, &chr_ol_53, &chr_ol_54, &chr_ol_55, 
   &chr_ol_56, &chr_ol_57, &chr_ol_58, &chr_ol_59, &chr_ol_60, &chr_ol_61, &chr_ol_62, &chr_ol_63, 
   &chr_ol_64, &chr_ol_65, &chr_ol_66, &chr_ol_67, &chr_ol_68, &chr_ol_69, &chr_ol_70, &chr_ol_71, 
   &chr_ol_72, &chr_ol_73, &chr_ol_74, &chr_ol_75, &chr_ol_76, &chr_ol_77, &chr_ol_78, &chr_ol_79, 
   &chr_ol_80, &chr_ol_81, &chr_ol_82, &chr_ol_83, &chr_ol_84, &chr_ol_85, &chr_ol_86, &chr_ol_87, 
   &chr_ol_88, &chr_ol_89, &chr_ol_90, &chr_ol_91, &chr_ol_92, &chr_ol_93, 0, &chr_ol_95, 
   &chr_ol_96, &chr_ol_97, &chr_ol_98, &chr_ol_99, &chr_ol_100,   &chr_ol_101,   &chr_ol_102,   &chr_ol_103, 
   &chr_ol_104,   &chr_ol_105,   &chr_ol_106,   &chr_ol_107,   &chr_ol_108,   &chr_ol_109,   &chr_ol_110,   &chr_ol_111, 
   &chr_ol_112,   &chr_ol_113,   &chr_ol_114,   &chr_ol_115,   &chr_ol_116,   &chr_ol_117,   &chr_ol_118,   &chr_ol_119, 
   &chr_ol_120,   &chr_ol_121,   &chr_ol_122,   &chr_ol_123,   &chr_ol_124,   &chr_ol_125,   &chr_ol_126,   0, 
};

const SFG_StrokeFont fgStrokeOrbitronLight = { (char*)"Orbitron-Light", 128, 10, ol_chars };