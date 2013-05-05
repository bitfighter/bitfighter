//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "FontManager.h"         // Class header

#include "GameSettings.h"
#include "ScreenInfo.h"

#include "OpenglUtils.h"         // For various rendering helpers
#include "stringUtils.h"         // For getFileSeparator()
#include "MathUtils.h"           // For MIN/MAX

// Our stroke fonts
#include "FontStrokeRoman.h"
#include "FontOrbitronLight.h"
#include "FontOrbitronMedium.h"

#include "../fontstash/stb_truetype.h"

#include <string>

using namespace std;


namespace Zap {


// Stroke font constructor
BfFont::BfFont(FontId fontId, const ::SFG_StrokeFont *strokeFont)
{
   mFontId = fontId;
   
   mIsStrokeFont = true;
   mOk = true;
   mStrokeFont = strokeFont;

   // TTF only stuff that is ignored
   mStashFontId = 0;
}


// TTF font constructor
BfFont::BfFont(FontId fontId, const string &fontFile, GameSettings *settings)
{
   mFontId = fontId;
      
   mIsStrokeFont = false;
   mStrokeFont = NULL;     // Stroke font only

   TNLAssert(FontManager::getStash(), "Invalid font stash!");

   if(FontManager::getStash() == NULL)
   {
      mOk = false;
      return;
   }

   string file = settings->getFolderManager()->fontsDir + getFileSeparator() + fontFile;

   mStashFontId = sth_add_font(FontManager::getStash(), file.c_str());

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


FontId BfFont::getId()
{
   return mFontId;
}


S32 BfFont::getStashFontId()
{
   return mStashFontId;
}


////////////////////////////////////////
////////////////////////////////////////


static FontId currentFontId;

static BfFont *fontList[FontCount];

sth_stash *FontManager::mStash = NULL;

// This must be run after VideoSystem::actualizeScreenMode()
void FontManager::initialize(GameSettings *settings)
{
   if(mStash == NULL)
      mStash = sth_create(512, 512);

   // Our stroke fonts
   fontList[FontRoman]               = new BfFont(FontRoman,               &fgStrokeRoman);
   fontList[FontOrbitronLightStroke] = new BfFont(FontOrbitronLightStroke, &fgStrokeOrbitronLight);
   fontList[FontOrbitronMedStroke]   = new BfFont(FontOrbitronMedStroke,   &fgStrokeOrbitronMed);

   // Our TTF fonts
   fontList[FontOcrA]           = new BfFont(FontOcrA,          "Digital.ttf",         settings);
   fontList[FontOrbitronLight]  = new BfFont(FontOrbitronLight, "Orbitron Light.ttf",  settings);
   fontList[FontOrbitronMedium] = new BfFont(FontOrbitronLight, "Orbitron Medium.ttf", settings);
   fontList[FontPrimeRegular]   = new BfFont(FontPrimeRegular,  "prime_regular.ttf",   settings);
   fontList[FontTenby5]         = new BfFont(FontTenby5,        "tenbyfive.ttf",       settings);

   // set texture blending function
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void FontManager::cleanup()
{
   for(S32 i = 0; i < FontCount; i++)
      if(fontList[i] != NULL)
         delete fontList[i];

   if(mStash != NULL)
   {
      sth_delete(mStash);
      mStash = NULL;
   }
}


sth_stash *FontManager::getStash()
{
   return mStash;
}


void FontManager::drawTTFString(BfFont *font, const char *string, F32 size)
{
   F32 outPos;

   sth_begin_draw(mStash);

   // 152.381f is what I stole from the bottom of FontStrokeRoman.h as the font size
   sth_draw_text(mStash, font->getStashFontId(), size, 0.0, 0.0, string, &outPos);

   sth_end_draw(mStash);
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
         setFont(FontRoman);
         return;
      
      case HelpContext:
         setFont(FontRoman);
         return;

      case HelpItemContext:
      case HUDContext:
      case LevelInfoContext:
      case LoadoutIndicatorContext:
      case OverlayMenuContext:
         setFont(FontTenby5);
         return;

      case TextEffectContext:
         setFont(FontOrbitronLightStroke);
         return;

      default:
         TNLAssert(false, "Unknown font context!");
         break;
   }
}
// FontRoman,
// FontOrbitronLightStroke,
// FontOrbitronMedStroke,
// FontOcrA,
// FontOrbitronLight,
// FontOrbitronMedium,
// FontPrimeRegular
// Tenby5


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
      renderVertexArray(characterVertexArray, strip->Number, GL_LINE_STRIP);
   }
   glTranslatef( schar->Right, 0.0, 0.0 );
}


S32 FontManager::getStringLength(const char* string)
{
   BfFont *font = fontList[currentFontId];

   if(font->isStrokeFont())
      return getStrokeFontStringLength(font->getStrokeFont(), string);
   else
      return getTtfFontStringLength(font, string);
}


S32 FontManager::getStrokeFontStringLength(const SFG_StrokeFont *font, const char *string)
{
   TNLAssert(font, "Null font!");

   if(!font || !string || ! *string)
      return 0;

   F32 length = 0.0;
   F32 lineLength = 0.0;

   while(U8 c = *string++)
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

   return S32(length + 0.5);
}


S32 FontManager::getTtfFontStringLength(BfFont *font, const char *string)
{
   F32 minx, miny, maxx, maxy;
   sth_dim_text(mStash, font->getStashFontId(), legacyRomanSizeFactorThanksGlut, string, &minx, &miny, &maxx, &maxy);

   return maxx - minx;
}


extern F32 gDefaultLineWidth;

void FontManager::renderString(F32 size, const char *string)
{
   BfFont *font = fontList[currentFontId];

   if(font->isStrokeFont())
   {
      static F32 modelview[16];
      glGetFloatv(GL_MODELVIEW_MATRIX, modelview);    // Fills modelview[]

      F32 linewidth = MAX(MIN(size * gScreenInfo.getPixelRatio() * modelview[0] / 20, 1.0f), 0.5f)    // Clamp to range of 0.5 - 1
                                                                           * gDefaultLineWidth;       // then multiply by line width (2 by default)
      glLineWidth(linewidth);

      F32 scaleFactor = size / 120.0f;  // Where does this magic number come from?
      glScalef(scaleFactor, -scaleFactor, 1);
      for(S32 i = 0; string[i]; i++)
         FontManager::drawStrokeCharacter(font->getStrokeFont(), string[i]);

      glLineWidth(gDefaultLineWidth);
   }
   else
   {
      // Flip upside down because y = -y
      glScalef(1, -1, 1);

      // Bonkers factor because we build the game around thinking the font size was 120
      // when it was really 152.381 (see bottom of FontStrokeRoman.h as well as magic scale
      // factor of 120.0f a few lines below).  This factor == 152.381 / 120
      static F32 legacyNormalizationFactor = legacyRomanSizeFactorThanksGlut / 120.0f;    // == 1.26984166667f

      drawTTFString(font, string, size * legacyNormalizationFactor);
   }
}


}
