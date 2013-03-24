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

#include "OpenglUtils.h"         // For various rendering helpers
#include "stringUtils.h"         // For getFileSeparator()
#include "MathUtils.h"           // For MIN/MAX
#include "ScreenInfo.h"

// Our stroke fonts
#include "FontStrokeRoman.h"
#include "FontOrbitronLight.h"
#include "FontOrbitronMedium.h"

#include "../fontstash/stb_truetype.h"

#include <string>

using namespace std;


namespace Zap {

// Stroke font constructor
BfFont::BfFont(FontManager::FontId fontId, const ::SFG_StrokeFont *strokeFont)
{
   mFontId = fontId;
   
   mIsStrokeFont = true;
   mOk = true;
   mStrokeFont = strokeFont;

   //mStash = NULL;       // TTF only
}

static sth_stash *mStash = NULL;


extern string getInstalledDataDir();

// TTF font constructor
BfFont::BfFont(FontManager::FontId fontId, const string &fontFile)
{
   mFontId = fontId;
      
   mIsStrokeFont = false;
   mStrokeFont = NULL;     // Stroke font only

   if(!mStash)
      mStash = sth_create(512, 512);

   TNLAssert(mStash, "Invalid font stash!");

   if(!mStash)
   {
      mOk = false;
      return;
   }

   string file = getInstalledDataDir() + getFileSeparator() + "fonts" + getFileSeparator() + fontFile;

   mStashFontId = sth_add_font(mStash, file.c_str());

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
   if(mStash)
   {
      sth_delete(mStash);
      mStash = NULL;
   }
}


const SFG_StrokeFont *BfFont::getStrokeFont()
{
   return mStrokeFont;
}


bool BfFont::isStrokeFont()
{
   return mIsStrokeFont;
}


FontManager::FontId BfFont::getId()
{
   return mFontId;
}


sth_stash *BfFont::getStash()
{
   return mStash;
}


S32 BfFont::getStashFontId()
{
   return mStashFontId;
}


////////////////////////////////////////
////////////////////////////////////////

//const SFG_StrokeFont *gCurrentFont = &fgStrokeRoman;  // fgStrokeOrbitronLight, fgStrokeOrbitronMed

static FontManager::FontId currentFontId;

static BfFont *fontList[FontManager::FontCount];

void FontManager::initialize()
{
   // Our stroke fonts
   fontList[FontRoman]               = new BfFont(FontRoman,               &fgStrokeRoman);
   fontList[FontOrbitronLightStroke] = new BfFont(FontOrbitronLightStroke, &fgStrokeOrbitronLight);
   fontList[FontOrbitronMedStroke]   = new BfFont(FontOrbitronMedStroke,   &fgStrokeOrbitronMed);

   // Our TTF fonts
   fontList[FontOcrA]          = new BfFont(FontOcrA,          "OCRA.ttf");
   fontList[FontOrbitronLight] = new BfFont(FontOrbitronLight, "Orbitron Light.ttf");
   fontList[FontPrimeRegular]  = new BfFont(FontPrimeRegular,  "prime_regular.ttf");
}


void FontManager::cleanup()
{
   for(S32 i = 0; i < FontCount; i++)
      delete fontList[i];
}


void FontManager::drawTTFString(BfFont *font, const char *string, F32 size)
{
   F32 outPos;

   sth_begin_draw(font->getStash());

   // 152.381f is what I stole from the bottom of FontStrokeRoman.h as the font size
   sth_draw_text(font->getStash(), font->getStashFontId(), size, 0.0, 0.0, string, &outPos);

   sth_end_draw(font->getStash());
}


void FontManager::setFont(FontId fontId)
{
   currentFontId = fontId;
}


void FontManager::setFontContext(FontContext fontContext)
{
   switch(fontContext)
   {
      case MenuContext:
         setFont(FontOcrA);
         return;
      case HUDContext:
         setFont(FontRoman);
         return;
      case HelpContext:
         setFont(FontRoman);
         return;
      case TextEffectContext:
         setFont(FontOrbitronLightStroke);
         return;
      default:
         TNLAssert(false, "Unknown font context!");
   }
}


static Vector<FontManager::FontId> contextStack;

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
   sth_dim_text(font->getStash(), font->getStashFontId(), legacyRomanSizeFactorThanksGlut, string, &minx, &miny, &maxx, &maxy);

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