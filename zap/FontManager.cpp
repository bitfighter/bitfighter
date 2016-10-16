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

#include "../fontstash/stb_truetype.h"

#include "tnlPlatform.h"
#include "../master/GameJoltConnector.h"

extern "C"
{
#  include "glinc.h"
#  define FONTSTASH_IMPLEMENTATION
#  include "../fontstash/fontstash.h"
#  define GLFONTSTASH_IMPLEMENTATION
#  include "../fontstash/glfontstash.h"
}

#include <string>

using namespace std;


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

   TNLAssert(FontManager::getStash(), "Invalid font stash!");

   if(FontManager::getStash() == NULL)
   {
      mOk = false;
      return;
   }

   for(U32 i = 0; i < sizeof(SystemFontDirectories) / sizeof(SystemFontDirectories[0]) && mStashFontId <= 0; i++) {
      string file = string(SystemFontDirectories[i]) + getFileSeparator() + fontFile;
      mStashFontId = fonsAddFont(FontManager::getStash(), file.c_str(), file.c_str());
   }

   // Prepare physfs for searching for sfx
   //PhysFS::clearSearchPath();
   //PhysFS::mountAll(settings->getFolderManager()->getFontDirs().getConstStlVector());

   // If fonsAddFont returned -1, then the font failed to be added
   if(mStashFontId == FONS_INVALID) {
      //string realDir = PhysFS::getRealDir(fontFile);
      //string file = joindir(realDir, fontFile);
      string file = checkName(fontFile, settings->getFolderManager()->getFontDirs());

      mStashFontId = fonsAddFont(FontManager::getStash(), "", file.c_str());
   }

   TNLAssert(mStashFontId != FONS_INVALID, "Invalid font id!");

   if(mStashFontId == FONS_INVALID)
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


static FontId currentFontId;

static BfFont *fontList[FontCount] = { NULL };

FONScontext *FontManager::mStash = NULL;
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


// stashPtr is a pointer that we pass in when we register the callback
void fontStashErrorHandlerCallback(void* stashPtr, int error, int val)
{
   switch(error)
   {
      case FONS_ATLAS_FULL:
      { 
         // Let's make the atlas a bit bigger so we can hold more glyphs and not get back here
         FONScontext *stash = (FONScontext *)stashPtr;
      
         // We should never get here; if we do, we want to figure out how much memory we really need, so we'll just bump this up by 
         // 10%.  If we ever get here, we need to increase the initial size (see call to glfonsCreate) so that we never hit this.
         S32 newWidth = stash->atlas->width * 1.1;    
         S32 newHeight = stash->atlas->height;

         TNLAssert(false, "Increase initial atlas size (glfonsCreate)");
         logprintf(LogConsumer::LogWarning, "FontStash warning: Atlas full. Increasing size to %d x %d (w x h).", newWidth, newHeight);
         S32 ok = fonsExpandAtlas(stash, newWidth, newHeight);

         if(!ok)
         {
            TNLAssert(false, "Resizing FontStash atlas failed... disabling TTF fonts.")
            logprintf(LogConsumer::LogError, "Resizing FontStash atlas failed... disabling TTF fonts.");
            FontManager::disableTtfFonts();
         }

         return;
      }

      case FONS_SCRATCH_FULL:
         TNLAssert(false, "FontStash error: Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.");
         logprintf(LogConsumer::LogError, "FontStash error: Scratch memory for rendering fonts full. Falling back to default font.");
         break;

      case FONS_STATES_OVERFLOW:
         TNLAssert(false, "FontStash error: Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.");
         logprintf(LogConsumer::LogError, "FontStash error: Stack overflow. Falling back to default font.");
         break;

      case FONS_STATES_UNDERFLOW:
         TNLAssert(false, "FontStash error: Trying to pop too many states fonsPopState().");
         logprintf(LogConsumer::LogError, "FontStash error: Stack underflow. Falling back to default font.");
         break;

      default:
         TNLAssert(false, "FontStash error: Unknown error!");
         logprintf(LogConsumer::LogError, "FontStash error: Unknown problem. Falling back to default font.");
   }

   FontManager::disableTtfFonts();
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

   TNLAssert(mGL != NULL, "RenderManager is NULL.  Bad things will happen!");

   mUsingExternalFonts = useExternalFonts;

   // Our stroke fonts -- these are always available
   fontList[FontRoman]               = new BfFont(&fgStrokeRoman);
   fontList[FontOrbitronLightStroke] = new BfFont(&fgStrokeOrbitronLight);
   fontList[FontOrbitronMedStroke]   = new BfFont(&fgStrokeOrbitronMed);

   if(mUsingExternalFonts)
   {
      TNLAssert(mStash == NULL, "This should be NULL, or else we'll have a memory leak!");
      mStash = glfonsCreate(2048, 512, FONS_ZERO_TOPLEFT);
      fonsSetErrorCallback(mStash, fontStashErrorHandlerCallback, mStash); // <== 3rd arg gets passed to the callback as 1st param

      TNLAssert(settings, "Settings can't be NULL if we are using external fonts!");

      // Our TTF fonts
      //fontList[FontOrbitronLight]  = new BfFont("Orbitron Light.ttf",  settings);
      //fontList[FontOrbitronMedium] = new BfFont("Orbitron Medium.ttf", settings);
      fontList[FontDroidSansMono]  = new BfFont("DroidSansMono.ttf",   settings);
      fontList[FontWebDings]       = new BfFont("webhostinghub-glyphs.ttf", settings);
      fontList[FontPlay]           = new BfFont("Play-Regular-hinting.ttf", settings);
      fontList[FontPlayBold]       = new BfFont("Play-Bold.ttf",       settings);
      fontList[FontModernVision]   = new BfFont("Modern-Vision.ttf",   settings);

      // Set texture blending function
      mGL->setDefaultBlendFunction();
   }
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
      glfonsDelete(mStash);
      mStash = NULL;
   }
}


FONScontext *FontManager::getStash()
{
   return mStash;
}


void FontManager::drawTTFString(BfFont *font, const char *string, F32 size)
{
   fonsSetFont(mStash, font->getStashFontId());
   fonsSetSize(mStash, size);
   fonsDrawText(mStash, 0.0, 0.0, string, NULL);
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

      case MenuContext:
      case OldSkoolContext:
         setFont(FontRoman);
         return;

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


void FontManager::setFontColor(const Color &color)
{
   mGL->glColor(color);    // For stroke fonts
   fonsSetColor(mStash, glfonsRGBA(color.r * 255, color.g * 255, color.b * 255, 255));    // For FontStash
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
      // I didn't find any strip->Number larger than 34, so I chose a buffer of 64
      // This may change if we go crazy with i18n some day...
      static F32 characterVertexArray[128];
      for(j = 0; j < strip->Number; j++)
      {
        characterVertexArray[2*j]     = strip->Vertices[j].X;
        characterVertexArray[(2*j)+1] = strip->Vertices[j].Y;
      }
      mGL->renderVertexArray(characterVertexArray, strip->Number, GLOPT::LineStrip);
   }
   mGL->glTranslate(schar->Right, 0.0f);
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


F32 FontManager::getStringLength(const char* string)
{
   BfFont *font = getFont(currentFontId);

   if(font->isStrokeFont())
      return getStrokeFontStringLength(font->getStrokeFont(), string);
   else
      return getTtfFontStringLength(font, string);
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
   // I think it's this simple...
   // TODO FIX - "Release 020"
   // - old:  size = 717 at all screen modes
   // - new:  size = 219 at windowed; 319 in fullscreen
   F32 bounds[4];
   F32 length = fonsTextBounds2(mStash, legacyRomanSizeFactorThanksGlut, 0, 0, string, NULL, bounds);
   //F32 length = fonsTextBounds(mStash, 0, 0, string, NULL, NULL);

   //F32 len = (bounds[2] - bounds[0]);
   //if(strcmp(string, "LEFT - previous page   |   RIGHT, SPACE - next page   |   ESC exits") == 0)
   //   printf("zzxz Len = %f/%f", length, len);

   return length;
}


void FontManager::renderString(F32 size, const char *string)
{
   BfFont *font = getFont(currentFontId);

   if(font->isStrokeFont())
   {
      static F32 modelview[16];
      mGL->glGetValue(GLOPT::ModelviewMatrix, modelview);    // Fills modelview[]

      // Clamp to range of 0.5 - 1 then multiply by line width (2 by default)
      F32 linewidth =
            CLAMP(size * DisplayManager::getScreenInfo()->getPixelRatio() * modelview[0] * 0.05f, 0.5f, 1.0f) * RenderUtils::DEFAULT_LINE_WIDTH;

      mGL->glLineWidth(linewidth);

      F32 scaleFactor = size / 120.0f;  // Where does this magic number come from?
      mGL->glScale(scaleFactor, -scaleFactor);
      for(S32 i = 0; string[i]; i++)
         FontManager::drawStrokeCharacter(font->getStrokeFont(), string[i]);

      mGL->glLineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
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
      F32 k = DisplayManager::getScreenInfo()->getPixelRatio() * 2;
      F32 rk = 1/k;

      mGL->glScale(rk, rk);

      // 'size * k' becomes 'size' due to the glScale above
      drawTTFString(font, string, size * k * legacyNormalizationFactor);
   }
}


}
