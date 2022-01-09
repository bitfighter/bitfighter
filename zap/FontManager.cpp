//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FontManager.h"         // Class header

#include "Renderer.h"
#include "GameSettings.h"
#include "DisplayManager.h"

#include "stringUtils.h"         // For getFileSeparator()
#include "MathUtils.h"           // For MIN/MAX

// Our stroke fonts
#include "FontStrokeRoman.h"
#include "FontOrbitronLight.h"
#include "FontOrbitronMedium.h"

#include "../fontstash/stb_truetype.h"
#include <tnlPlatform.h>

#include <string>

using namespace std;

// Theses values should be plenty for the fonts we have
static const unsigned MAX_STRING_LENGTH = 255;
static const unsigned MAX_STRIPS_PER_CHARACTER = 32;
static const unsigned MAX_POINTS_PER_STRIP = 128;

namespace Zap {


// Stroke font constructor
BfFont::BfFont(const ::SFG_StrokeFont *strokeFont)
{
   mIsStrokeFont = true;
   mOk = true;
   mStrokeFont = strokeFont;

   // TTF only stuff that is ignored
   mStashFontId = 0;
}


const char *BfFont::SystemFontDirectories[] = {
#ifdef TNL_OS_LINUX
   "/usr/share/fonts/truetype/droid",
   "/usr/share/fonts/truetype/play",
   "/usr/share/fonts/truetype/ocr-a"
#else
   ""
#endif
};


// TTF font constructor
BfFont::BfFont(const string &fontFile, GameSettings *settings)
{
   mIsStrokeFont = false;
   mStrokeFont = NULL;     // Stroke font only
   mStashFontId = 0;

   TNLAssert(FontManager::getStash(), "Invalid font stash!");

   if(FontManager::getStash() == NULL)
   {
      mOk = false;
      return;
   }

   for(U32 i = 0; i < sizeof(SystemFontDirectories) / sizeof(SystemFontDirectories[0]) && mStashFontId <= 0; i++) {
      string file = string(SystemFontDirectories[i]) + getFileSeparator() + fontFile;
      mStashFontId = sth_add_font(FontManager::getStash(), file.c_str());
   }

   if(mStashFontId <= 0) {
      string file = settings->getFolderManager()->fontsDir + getFileSeparator() + fontFile;
      mStashFontId = sth_add_font(FontManager::getStash(), file.c_str());
   }

   TNLAssert(mStashFontId > 0, "Invalid font id!");

   if(mStashFontId == 0)
   {
      mOk = false;
      return;
   }

   mOk = true;
}


// Destructor
BfFont::~BfFont()
{
   // Do nothing
}


const SFG_StrokeFont *BfFont::getStrokeFont()
{
   return mStrokeFont;
}


bool BfFont::isStrokeFont()
{
   return mIsStrokeFont;
}


S32 BfFont::getStashFontId()
{
   return mStashFontId;
}


////////////////////////////////////////
////////////////////////////////////////


static FontId currentFontId = FontDefault;

static BfFont *fontList[FontCount] = {NULL};

sth_stash *FontManager::mStash = NULL;
bool FontManager::mUsingExternalFonts = true;

FontManager::FontManager()
{
   for(S32 i = 0; i < FontCount; i++)
      fontList[i] = NULL;
}


// This must be run after VideoSystem::updateDisplayState()
// If useExternalFonts is false, settings can be NULL
void FontManager::initialize(GameSettings *settings, bool useExternalFonts)
{
   cleanup();  // Makes sure its been cleaned up first, many tests call init without cleanup

   mUsingExternalFonts = useExternalFonts;

   // Our stroke fonts
   fontList[FontRoman]               = new BfFont(&fgStrokeRoman);
   fontList[FontOrbitronLightStroke] = new BfFont(&fgStrokeOrbitronLight);
   fontList[FontOrbitronMedStroke]   = new BfFont(&fgStrokeOrbitronMed);

   if(mUsingExternalFonts)
   {
      TNLAssert(mStash == NULL, "This should be NULL, or else we'll have a memory leak!");
      mStash = sth_create(512, 512);

      TNLAssert(settings, "Settings can't be NULL if we are using external fonts!");

      // Our TTF fonts
      //fontList[FontOrbitronLight]  = new BfFont("Orbitron Light.ttf",  settings);
      //fontList[FontOrbitronMedium] = new BfFont("Orbitron Medium.ttf", settings);
      fontList[FontDroidSansMono]  = new BfFont("DroidSansMono.ttf",   settings);
      fontList[FontWebDings]       = new BfFont("webhostinghub-glyphs.ttf", settings);
      fontList[FontPlay]           = new BfFont("Play-Regular-hinting.ttf", settings);
      fontList[FontPlayBold]       = new BfFont("Play-Bold.ttf",       settings);
      fontList[FontModernVision]   = new BfFont("Modern-Vision.ttf",   settings);
   }
}


// Runs intialize preserving mUsingExternalFonts
void FontManager::reinitialize(GameSettings *settings)
{
   initialize(settings, mUsingExternalFonts);
}


void FontManager::cleanup()
{
   for(S32 i = 0; i < FontCount; i++)
   {
      delete fontList[i];
      fontList[i] = NULL;
   }

   if(mUsingExternalFonts)
   {
      sth_delete(mStash);
      mStash = NULL;
   }
}


BfFont *FontManager::getFont(FontId currentFontId)
{
   BfFont *font;

   if(mUsingExternalFonts || currentFontId < FirstExternalFont)
      font = fontList[currentFontId];
   else
      font = fontList[FontDefault];

   TNLAssert(font, "Font is NULL... Did the FontManager get initialized?");

   return font;
}


S32 FontManager::getStrokeFontStringLength(const SFG_StrokeFont *font, const char *string)
{
   TNLAssert(font, "Null font!");

   if(!font || !string || !*string)
      return 0;

   F32 length = 0.0;
   F32 lineLength = 0.0;

   while(U8 c = *string++)
      if(c < font->Quantity)
      {
         if(c == '\n')  // EOL; reset the length of this line 
         {
            if(length < lineLength)
               length = lineLength;
            lineLength = 0;
         }
         else           // Not an EOL, increment the length of this line
         {
            const SFG_StrokeChar *schar = font->Characters[c];
            if(schar)
               lineLength += schar->Right;
         }
      }

   if(length < lineLength)
      length = lineLength;

   return S32(length + 0.5);
}


S32 FontManager::getTtfFontStringLength(BfFont *font, const char *string)
{
   F32 minx, miny, maxx, maxy;
   sth_dim_text(mStash, font->getStashFontId(), legacyRomanSizeFactorThanksGlut, string, &minx, &miny, &maxx, &maxy);

   return S32(maxx - minx);
}


void FontManager::getStrokeCharacterPoints(const SFG_StrokeChar *schar, F32 xOffset, F32 *outPoints, U32 *pointCount)
{
   const SFG_StrokeStrip *strip = schar->Strips;
   U32 arrayOffset = 0;

   for(S32 i = 0; i < schar->Number; i++, strip++)
   {
      // Get first point
      F32 lastX = strip->Vertices[0].X;
      F32 lastY = strip->Vertices[0].Y;

      // Construct individual lines from the strip. This allows us to render the character using a
      // single GL call with GL_LINES.
      for(S32 j = 1; j < strip->Number; j++)
      {
         U32 arrayIndex = arrayOffset + (j - 1) * 4;

         outPoints[arrayIndex] = lastX + xOffset;
         outPoints[arrayIndex + 1] = lastY;
         outPoints[arrayIndex + 2] = strip->Vertices[j].X + xOffset;
         outPoints[arrayIndex + 3] = strip->Vertices[j].Y;

         lastX = strip->Vertices[j].X;
         lastY = strip->Vertices[j].Y;
      }

      arrayOffset += (strip->Number - 1) * 4;
   }

   *pointCount = arrayOffset / 2; // 2 floats per vertex
}


extern F32 gDefaultLineWidth;


void FontManager::renderStrokedString(F32 size, const char *string)
{
   Renderer &r = Renderer::get();
   const SFG_StrokeFont *font = getFont(currentFontId)->getStrokeFont();
   static F32 modelview[16];
   r.getMatrix(MatrixType::ModelView, modelview);    // Fills modelview[]

   if(!font)
      return;

   // Clamp to range of 0.5 - 1 then multiply by line width (2 by default)
   F32 linewidth =
      CLAMP(size * DisplayManager::getScreenInfo()->getPixelRatio() * modelview[0] * 0.05f, 0.5f, 1.0f) * gDefaultLineWidth;
   r.setLineWidth(linewidth);

   // Get all necessary points to render the string with a single GL call
   static F32 points[MAX_STRING_LENGTH * MAX_STRIPS_PER_CHARACTER * MAX_POINTS_PER_STRIP];
   U32 totalPointCount = 0;
   F32 nextXOffset = 0;

   for(S32 i = 0; (string[i] != 0) && (i < MAX_STRING_LENGTH); i++)
   {
      if(!(string[i] > 0))
         continue;
      if(!(string[i] < font->Quantity))
         continue;

      const SFG_StrokeChar *schar = font->Characters[string[i]];
      U32 pointCount;

      if(!schar)
         continue;

      // 2 values per vertex
      FontManager::getStrokeCharacterPoints(schar, nextXOffset, points + totalPointCount * 2, &pointCount);
      totalPointCount += pointCount;
      nextXOffset += schar->Right;
   }

   F32 scaleFactor = size / 120.0f;  // Where does this magic number come from?
   r.scale(scaleFactor, -scaleFactor, 1);
   r.renderVertexArray(points, totalPointCount, RenderType::Lines);
   r.setLineWidth(gDefaultLineWidth);
}


sth_stash *FontManager::getStash()
{
   return mStash;
}


void FontManager::drawTTFString(BfFont *font, const char *string, F32 size)
{
   F32 outPos;

   sth_begin_draw(mStash);

   sth_draw_text(mStash, font->getStashFontId(), size, 0.0, 0.0, string, &outPos);

   sth_end_draw(mStash);
}


S32 FontManager::getStringLength(const char *string)
{
   BfFont *font = getFont(currentFontId);

   if(font->isStrokeFont())
      return getStrokeFontStringLength(font->getStrokeFont(), string);
   else
      return getTtfFontStringLength(font, string);
}


void FontManager::renderString(F32 size, const char *string)
{
   BfFont *font = getFont(currentFontId);

   if(font->isStrokeFont())
   {
      renderStrokedString(size, string);
   }
   else
   {
      // Bonkers factor because we built the game around thinking the font size was 120
      // when it was really 152.381 (see bottom of FontStrokeRoman.h as well as magic scale
      // factor of 120.0f a few lines below).  This factor == 152.381 / 120
      static F32 legacyNormalizationFactor = legacyRomanSizeFactorThanksGlut / 120.0f;    // == 1.26984166667f

      // In order to make TTF fonts crisp at all font and window sizes, we first
      // correct for the pixelRatio scaling, and then generate a texture with twice
      // the resolution we need. This produces crisp, anti-aliased text even after the
      // texture is resampled.
      F32 k = DisplayManager::getScreenInfo()->getPixelRatio() * 2.0f;

      // Flip upside down because y = -y
      Renderer::get().scale(1 / k, -1 / k, 1);
      // `size * k` becomes `size` due to the glScale above
      drawTTFString(font, string, size * k * legacyNormalizationFactor);
   }
}


void FontManager::setFont(FontId fontId)
{
   currentFontId = fontId;
}


void FontManager::setFontContext(FontContext fontContext)
{
   switch(fontContext)
   {
   case BigMessageContext:
      setFont(FontOrbitronMedStroke);
      return;

   case OldSkoolContext:
      setFont(FontRoman);
      return;

   case MenuContext:
   case MotdContext:
   case HelpContext:
   case ErrorMsgContext:
   case ReleaseVersionContext:
   case LevelInfoContext:
   case ScoreboardContext:
   case MenuHeaderContext:
      setFont(FontPlay);
      return;

   case FPSContext:
   case LevelInfoHeadlineContext:
   case LoadoutIndicatorContext:
   case ScoreboardHeadlineContext:
   case HelperMenuHeadlineContext:
      setFont(FontModernVision);
      return;

   case HelperMenuContext:
   case HelpItemContext:
      setFont(FontPlay);
      return;

   case KeyContext:
      setFont(FontPlay);
      return;

   case TextEffectContext:
      setFont(FontOrbitronLightStroke);
      return;

   case ChatMessageContext:
   case InputContext:
      setFont(FontDroidSansMono);
      return;

   case WebDingContext:
      setFont(FontWebDings);
      return;

   case TimeLeftHeadlineContext:
   case TimeLeftIndicatorContext:
   case TeamShuffleContext:
      setFont(FontPlay);
      return;

   default:
      TNLAssert(false, "Unknown font context!");
      break;
   }
}


static Vector<FontId> contextStack;

void FontManager::pushFontContext(FontContext fontContext)
{
   contextStack.push_back(currentFontId);
   setFontContext(fontContext);
}


void FontManager::popFontContext()
{
   FontId fontId;

   TNLAssert(contextStack.size() > 0, "No items on context stack!");

   if(contextStack.size() > 0)
   {
      fontId = contextStack.last();
      contextStack.erase(contextStack.size() - 1);
   }
   else
      fontId = FontDefault;

   setFont(fontId);
}


}
