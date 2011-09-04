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

#ifndef COLORS_H_
#define COLORS_H_

#include "Color.h"

namespace Zap 
{

namespace Colors 
{
   const Color red(1,0,0);
   const Color green(0,1,0);
   const Color blue(0,0,1);
   const Color yellow(1,1,0);
   const Color cyan(0,1,1);
   const Color magenta(1,0,1);
   const Color black(0,0,0);
   const Color white(1,1,1);
   const Color gray50(0.50);
   const Color gray80(0.80);
   const Color orange50(1, .50f, 0);      // Rabbit orange
   const Color orange67(1, .67f ,0);      // A more reddish orange
   const Color paleRed(1, .50f, .50f);    // A washed out red
};

}

#endif /* COLORS_H_ */
