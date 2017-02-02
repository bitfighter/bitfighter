//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FontManager.h"         // Class header

#include "GameSettings.h"
#include "DisplayManager.h"

//#include "physfs.hpp"
#include "RenderUtils.h"         // For various rendering helpers
#include "stringUtils.h"         // For getFileSeparator()
#include "MathUtils.h"           // For MIN/MAX

// Our stroke fonts
#include "FontStrokeRoman.h"
#include "FontOrbitronLight.h"
#include "FontOrbitronMedium.h"

#include "tnlPlatform.h"
#include "../master/GameJoltConnector.h"

#include "../nanovg/nanovg.h"

#include <string>

using namespace std;

#define FONS_INVALID -1

namespace Zap {


// Stroke font constructor
BfFont::BfFont(const ::SFG_StrokeFont *strokeFont)
{
   mIsStrokeFont = true;
   mOk = true;
   mStrokeFont = strokeFont;

   // TTF only stuff that is ignored
   mStashFontId = FONS_INVALID;
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
   mStashFontId = FONS_INVALID;

   // This seems to fail... wrong folder?
   //for(U32 i = 0; i < sizeof(SystemFontDirectories) / sizeof(SystemFontDirectories[0]) && mStashFontId <= 0; i++) {
   //   string file = string(SystemFontDirectories[i]) + getFileSeparator() + fontFile;
   //   mStashFontId = fonsAddFont(FontManager::getStash(), file.c_str(), file.c_str());
   //   printf("Adding font %s: %d\n", file.c_str(), mStashFontId);//xyzzy
   //}

   // Prepare physfs for searching for sfx
   //PhysFS::clearSearchPath();
   //PhysFS::mountAll(settings->getFolderManager()->getFontDirs().getConstStlVector());

   // If fonsAddFont returned -1, then the font failed to be added
   if(mStashFontId == FONS_INVALID)
   {
      //string realDir = PhysFS::getRealDir(fontFile);
      //string file = joindir(realDir, fontFile);
      string file = checkName(fontFile, settings->getFolderManager()->getFontDirs());

      mStashFontId = nvgCreateFont(RenderManager::getNVG(), "", file.c_str());

      printf("Adding font %s: %d\n", file.c_str(), mStashFontId);//xyzzy
   }

   TNLAssert(mStashFontId != FONS_INVALID, "Invalid font id!");

   mOk = mStashFontId != FONS_INVALID;
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


static FontId currentFontId;

static BfFont *fontList[FontCount] = { NULL };

bool FontManager::mUsingExternalFonts = true;

// Constructorer
FontManager::FontManager()
{
   for(S32 i = 0; i < FontCount; i++)
      fontList[i] = NULL;
}


// Destructor
FontManager::~FontManager()
{
   // Do nothing
}


// Runs intialize preserving mUsingExternalFonts
void FontManager::reinitialize(GameSettings *settings)
{
   initialize(settings, mUsingExternalFonts);
}


void FontManager::disableTtfFonts()
{
   mUsingExternalFonts = false;
}


// This must be run after VideoSystem::actualizeScreenMode()
// If useExternalFonts is false, settings can be NULL
void FontManager::initialize(GameSettings *settings, bool useExternalFonts)
{
   cleanup();  // Makes sure its been cleaned up first, many tests call init without cleanup

   TNLAssert(nvg != NULL, "RenderManager is NULL.  Bad things will happen!");

   mUsingExternalFonts = useExternalFonts;

   // Our stroke fonts -- these are always available
   fontList[FontRoman]               = new BfFont(&fgStrokeRoman);
   fontList[FontOrbitronLightStroke] = new BfFont(&fgStrokeOrbitronLight);
   fontList[FontOrbitronMedStroke]   = new BfFont(&fgStrokeOrbitronMed);

   if(mUsingExternalFonts)
   {
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


void FontManager::cleanup()
{
   for(S32 i = 0; i < FontCount; i++)
   {
      delete fontList[i];
      fontList[i] = NULL;
   }
}


void FontManager::drawTTFString(BfFont *font, const char *string, F32 size)
{
   nvgFontSize(nvg, size);
   nvgText(nvg, 0.0, 0.0, string, NULL);
}


void FontManager::setFont(FontId fontId)
{
   currentFontId = fontId;
   nvgFontFaceId(nvg, getFont(fontId)->getStashFontId());
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
      case EditorWarningContext:
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


void FontManager::setFontColor(const Color &color, F32 alpha)
{
   // Change color for both strokes and TTF (fill) fonts
   nvgStrokeColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
   nvgFillColor(nvg, nvgRGBAf(color.r, color.g, color.b, alpha));
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
      fontId = FontRoman;

   setFont(fontId);
}


void FontManager::drawStrokeCharacter(const SFG_StrokeFont *font, S32 character)
{
   /*
    * Draw a stroke character
    */
   const SFG_StrokeChar *schar;
   const SFG_StrokeStrip *strip;
   S32 i, j;

   if(!(character >= 0))
      return;
   if(!(character < font->Quantity))
      return;
   if(!font)
      return;

   schar = font->Characters[ character ];

   if(!schar)
      return;

   strip = schar->Strips;

   for(i = 0; i < schar->Number; i++, strip++)
   {
      nvgBeginPath(nvg);
      nvgMoveTo(nvg, strip->Vertices[0].X, strip->Vertices[0].Y);
      for(j = 1; j < strip->Number; j++)
         nvgLineTo(nvg, strip->Vertices[j].X, strip->Vertices[j].Y);
      nvgStroke(nvg);
   }
   nvgTranslate(nvg, schar->Right, 0.0f);
}


BfFont *FontManager::getFont(FontId currentFontId)
{
   BfFont *font;

   if(mUsingExternalFonts || currentFontId < FirstExternalFont)
      font = fontList[currentFontId];
   else
     font = fontList[FontRoman]; 

   TNLAssert(font, "Font is NULL... Did the FontManager get initialized?");

   return font;
}

// FIXME reconcile why this factor is needed; it shouldn't be
#define MAGIC_FONTSTASH_FACTOR 10.0
F32 FontManager::getStringLength(const char *string)
{
   BfFont *font = getFont(currentFontId);

   if(font->isStrokeFont())
      return getStrokeFontStringLength(font->getStrokeFont(), string);
   else
      return getTtfFontStringLength(font, string) * MAGIC_FONTSTASH_FACTOR;
}


F32 FontManager::getStrokeFontStringLength(const SFG_StrokeFont *font, const char *string)
{
   TNLAssert(font, "Null font!");

   if(!font || !string || ! *string)
      return 0;

   F32 length = 0.0;
   F32 lineLength = 0.0;

   while(unsigned char c = *string++)
      if(c < font->Quantity )
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

   return length + 0.5f;
}


F32 FontManager::getTtfFontStringLength(BfFont *font, const char *string)
{
   F32 bounds[4];
   F32 length = nvgTextBounds(nvg, 0, 0, string, NULL, bounds);
   
   if(strcmp(string, "Delete Selection") == 0)
   {
      F32 len = (bounds[2] - bounds[0]);  //xyzzy
      printf("zzxz Len = %f/%f", length, len);
   }

   return length;
}


void FontManager::renderString(F32 x, F32 y, F32 size, F32 angle, const char *string)
{
   BfFont *font = getFont(currentFontId);

   if(font->isStrokeFont())
   {
      nvgSave(nvg);
      nvgTranslate(nvg, x, y);
      nvgRotate(nvg, angle);

      // Clamp to range of 0.5 - 1 then multiply by line width (2 by default)
      F32 linewidth = RenderUtils::DEFAULT_LINE_WIDTH;

      F32 scaleFactor = size / 120.0f;
      nvgScale(nvg, scaleFactor, -scaleFactor);

      // Upscaled because we downscaled the whole character
      nvgStrokeWidth(nvg, linewidth/scaleFactor);
      for(S32 i = 0; string[i]; i++)
         FontManager::drawStrokeCharacter(font->getStrokeFont(), string[i]);

      nvgStrokeWidth(nvg, RenderUtils::DEFAULT_LINE_WIDTH);

      nvgRestore(nvg);
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
//      F32 k = DisplayManager::getScreenInfo()->getPixelRatio() * 2;
//      F32 rk = 1/kp;
//
//      mGL->glScale(rk, rk);
//
//      // 'size * k' becomes 'size' due to the glScale above
//      drawTTFString(font, string, size * k * legacyNormalizationFactor);

      // TODO redo upscaling?  check fonts in fullscreen with new fontstash to
      // see if they look good
      nvgSave(nvg);
      nvgTranslate(nvg, x, y);
      nvgRotate(nvg, angle);

      drawTTFString(font, string, size * legacyNormalizationFactor);

      nvgRestore(nvg);
   }
}


}
