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

#ifndef BF_NO_SCREENSHOTS

#include "stringUtils.h"
#include "config.h"
#include "ScreenInfo.h"
#include "ClientGame.h"
#include "ConfigEnum.h"
#include "UI.h"

#include "tnlLog.h"

#ifdef TNL_OS_MOBILE
#include "SDL_opengles.h"
#else
#include "SDL_opengl.h"
#endif

#include "string.h"
#include "cmath"

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


void ScreenShooter::resizeViewportToCanvas(UIManager *uiManager)
{
   // Grab the canvas width/height and normalize our screen to it
   S32 width = gScreenInfo.getGameCanvasWidth();
   S32 height = gScreenInfo.getGameCanvasHeight();

   glViewport(0, 0, width, height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   glOrtho(0, width, height, 0, 0, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glScissor(0, 0, width, height);

   // Now render a frame to draw our new viewport to the back buffer
   uiManager->renderCurrent();
}


// Stolen from VideoSystem::actualizeScreenMode()
void ScreenShooter::restoreViewportToWindow(GameSettings *settings)
{
   DisplayMode displayMode = settings->getIniSettings()->displayMode;

   // Set up video/window flags amd parameters and get ready to change the window
   S32 sdlWindowWidth, sdlWindowHeight;
   F64 orthoLeft = 0, orthoRight = 0, orthoTop = 0, orthoBottom = 0;

   // Set up variables according to display mode
   switch(displayMode)
   {
      case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
         sdlWindowWidth = gScreenInfo.getPhysicalScreenWidth();
         sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
         orthoRight = gScreenInfo.getGameCanvasWidth();
         orthoBottom = gScreenInfo.getGameCanvasHeight();
         break;

      case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
         sdlWindowWidth = gScreenInfo.getPhysicalScreenWidth();
         sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
         orthoLeft = -1 * (gScreenInfo.getHorizDrawMargin());
         orthoRight = gScreenInfo.getGameCanvasWidth() + gScreenInfo.getHorizDrawMargin();
         orthoBottom = gScreenInfo.getGameCanvasHeight() + gScreenInfo.getVertDrawMargin();
         orthoTop = -1 * (gScreenInfo.getVertDrawMargin());
         break;

      case DISPLAY_MODE_WINDOWED:
      default:  //  Fall through OK
         sdlWindowWidth = (S32) floor((F32)gScreenInfo.getGameCanvasWidth()  * settings->getIniSettings()->winSizeFact + 0.5f);
         sdlWindowHeight = (S32) floor((F32)gScreenInfo.getGameCanvasHeight() * settings->getIniSettings()->winSizeFact + 0.5f);
         orthoRight = gScreenInfo.getGameCanvasWidth();
         orthoBottom = gScreenInfo.getGameCanvasHeight();
      break;
   }

   glViewport(0, 0, sdlWindowWidth, sdlWindowHeight);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, 0, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Now scissor
   if(displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
   {
      glScissor(gScreenInfo.getHorizPhysicalMargin(), // x
            gScreenInfo.getVertPhysicalMargin(),      // y
            gScreenInfo.getDrawAreaWidth(),           // width
            gScreenInfo.getDrawAreaHeight());         // height
   }
   else
      glScissor(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());
}


// Thanks to the good developers of naev for excellent code to base this off of.
// Much was copied directly.
void ScreenShooter::saveScreenshot(UIManager *uiManager, GameSettings *settings)
{
   string folder = settings->getFolderManager()->screenshotDir;

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

   // We default to resizing the opengl viewport to the standard canvas size, unless we're
   // in the editor or our window is smaller than the canvas size
   bool doResize = uiManager->getCurrentUI()->getMenuID() != EditorUI &&
         gScreenInfo.getWindowWidth() >= gScreenInfo.getGameCanvasWidth();

   // Change opengl viewport temporarily to have consistent screenshot sizes
   if(doResize)
      resizeViewportToCanvas(uiManager);

   // Now let's grab them pixels
   S32 width;
   S32 height;

   // If we're resizing, use the default canvas size
   if(doResize)
   {
      width = gScreenInfo.getGameCanvasWidth();
      height = gScreenInfo.getGameCanvasHeight();
   }
   // Otherwise just take the window size
   else
   {
      width = gScreenInfo.getWindowWidth();
      height = gScreenInfo.getWindowHeight();
   }

   // Allocate buffer
   GLubyte *screenBuffer = new GLubyte[BytesPerPixel * width * height];  // Glubyte * 3 = 24 bits
   png_bytep *rows = new png_bytep[height];

   // Set alignment at smallest for compatibility
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   // Grab the front buffer with the new viewport
   glReadBuffer(GL_BACK);

   // Read pixels from buffer - slow operation
   glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screenBuffer);

   // Change opengl viewport back to what it was
   if(doResize)
      restoreViewportToWindow(settings);

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

// We have created FILE* pointer, we write FILE* pointer here, not in libpng14.dll
// This is to avoid conflicts with different FILE struct between compilers.
// alternative is to make FILE* pointer get created, written and closed entirely inside libpng14.dll
static void PNGAPI png_user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   FILE *f = (png_FILE_p)(png_get_io_ptr(png_ptr));
   png_size_t check = fwrite(data, 1, length, f);
   if (check != length)
      png_error(png_ptr, "Write Error");
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
   //png_init_io(pngContainer, file); // Having fwrite inside libpng14.dll (and fopen outside of .dll) can be crashy...
   png_set_write_fn(pngContainer, file, &png_user_write_data, NULL);

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

#endif // BF_NO_SCREENSHOTS
