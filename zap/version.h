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

#ifndef _VERSION_H_
#define _VERSION_H_

#define MASTER_PROTOCOL_VERSION 3  // Change this when releasing an incompatible cm/sm protocol (must be int)
#define CS_PROTOCOL_VERSION 32     // Change this when releasing an incompatible cs protocol (must be int)
#define BUILD_VERSION 1836         // Version of the game according to hg, will be unique every release (must be int)
                                   // Get from "hg summary"
#define ZAP_GAME_NAME "Bitfighter"
#define ZAP_GAME_RELEASE "015a"    // Change this with every release -- for display purposes only, string, 
                                   // will also be used for name of installer on windows, so be careful with spaces  

#endif

