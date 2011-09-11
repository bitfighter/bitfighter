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

#ifndef SCREENSHOOTER_H_
#define SCREENSHOOTER_H_

#include "tnlTypes.h"

#include "png.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

class ScreenShooter
{
private:
   static const S32 BitDepth = 8;
   static const S32 BytesPerPixel = 3;  // 3 bytes = 24 bits

   static bool writePNG(const char *file_name, png_bytep *rows,
                        S32 width, S32 height, S32 colorType, S32 bitDepth);

public:
   ScreenShooter();
   virtual ~ScreenShooter();

   static void saveScreenshot(const string &folder);
};

} /* namespace Zap */
#endif /* SCREENSHOOTER_H_ */
