//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef SCREENSHOOTER_H_
#define SCREENSHOOTER_H_

#ifdef TNL_OS_MOBILE
#define BF_NO_SCREENSHOTS
#endif

#include "tnlTypes.h"
#include "RenderManager.h"

#ifndef BF_NO_SCREENSHOTS

#include "png.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

class UIManager;
class GameSettings;

class ScreenShooter: RenderManager
{
private:
   static const S32 BitDepth = 8;
   static const S32 BytesPerPixel = 3;  // 3 bytes = 24 bits

   static void resizeViewportToCanvas(UIManager *uiManager);
   static void restoreViewportToWindow(GameSettings *settings);

   static bool writePNG(const char *file_name, png_bytep *rows,
                        S32 width, S32 height, S32 colorType, S32 bitDepth);
public:
   ScreenShooter();
   virtual ~ScreenShooter();

   static void saveScreenshot(UIManager *uiManager, GameSettings *settings, string filename = "");
};

} /* namespace Zap */

#endif // BF_NO_SCREENSHOTS

#endif /* SCREENSHOOTER_H_ */
