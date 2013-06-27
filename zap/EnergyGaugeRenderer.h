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

#ifndef _ENERGY_GAUGE_RENDERER_
#define _ENERGY_GAUGE_RENDERER_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap {   namespace UI
{

class EnergyGaugeRenderer
{
public:
   static const S32 GAUGE_WIDTH = 200;
   static const S32 GUAGE_HEIGHT = 20;
   static const S32 SAFTEY_LINE_EXTEND = 4;      // How far the safety line extends above/below the main bar

   static void render(S32 energy);
};

} } // Nested namespace


#endif

