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

#include "ScreenShooter.h"
#include "stringUtils.h"
#include "config.h"
#include "ScreenInfo.h"

#include "tnlLog.h"

#ifdef TNL_OS_MOBILE
#include "SDL_opengles.h"
#else
#include "SDL_opengl.h"
#endif

#include "string.h"

using namespace std;

namespace Zap
{

ScreenShooter::ScreenShooter()
{
   // Do nothing
}


ScreenShooter::~ScreenShooter()
{
   // Do nothing
}


// Thanks to the good developers of naev for excellent code to base this off of.
// Much was copied directly.
void ScreenShooter::saveScreenshot(const string &folder)
{
   // Let's find a filename to use
   makeSureFolderExists(folder);

   string fullFilename;
   S32 ctr = 0;

   while(true)    // Settle in for the long haul, boys.  This seems crazy...
   {
      fullFilename = joindir(folder, "screenshot_" + itos(ctr++) + ".png");

      if(!fileExists(fullFilename))
         break;
   }

   // Now let's grab them pixels
   S32 width = gScreenInfo.getWindowWidth();
   S32 height = gScreenInfo.getWindowHeight();

   // Allocate buffer
   GLubyte *screenBuffer = new GLubyte[BytesPerPixel * width * height];  // Glubyte * 3 = 24 bits
   png_bytep *rows = new png_bytep[height];

   // Read pixels from buffer - slow operation
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screenBuffer);

   // Convert Data
   for (S32 i = 0; i < height; i++)
      rows[i] = &screenBuffer[(height - i - 1) * (BytesPerPixel * width)];  // Backwards!

   // Write the PNG!
   if(!writePNG(fullFilename.c_str(), rows, width, height, PNG_COLOR_TYPE_RGB, BitDepth))
      logprintf(LogConsumer::LogError, "Creating screenshot failed!!");

   // Clean up
   delete [] rows;
   delete [] screenBuffer;
}


// Saves a PNG
bool ScreenShooter::writePNG(const char *file_name, png_bytep *rows, S32 width, S32 height, S32 colorType, S32 bitDepth)
{
   png_structp pngContainer;
   png_infop pngInfo;
   FILE *file;

   // Open file for writing
   if (!(file = fopen(file_name, "wb"))) {
      logprintf(LogConsumer::LogError, "Unable to open '%s' for writing.", file_name);
      return false;
   }

   // Build out PNG container
   if (!(pngContainer = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
      logprintf(LogConsumer::LogError, "Unable to create PNG container.");
      fclose(file);
      return false;
   }

   // Build out PNG information
   pngInfo = png_create_info_struct(pngContainer);

   // Set the image details
   png_init_io(pngContainer, file);
   png_set_compression_level(pngContainer, Z_DEFAULT_COMPRESSION);
   png_set_IHDR(pngContainer, pngInfo, width, height, bitDepth, colorType,
         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
         PNG_FILTER_TYPE_DEFAULT);

   // Write the image
   png_write_info(pngContainer, pngInfo);
   png_write_image(pngContainer, rows);
   png_write_end(pngContainer, NULL);

   // Destroy memory allocated to the PNG container
   png_destroy_write_struct(&pngContainer, &pngInfo);

   // Close the file
   fclose(file);

   return true;
}

} /* namespace Zap */
