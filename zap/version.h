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

#define ZAP_GAME_NAME "Bitfighter"

#define MASTER_PROTOCOL_VERSION 5  // Change this when releasing an incompatible cm/sm protocol (must be int)
                                   // MASTER_PROTOCOL_VERSION = 4, client 015a and older (CS_PROTOCOL_VERSION <= 32) can not connect to our new master.
#define CS_PROTOCOL_VERSION 36     // Change this when releasing an incompatible cs protocol (must be int)
// 016 = 33 
// 017, 017a = 35
// 018 = 36

#define BUILD_VERSION 6059         // Version of the game according to hg, will be unique every release (must be int)
// 016  = 3737                     // Get from "hg summary"
// 017  = 4252
// 017a = 4265
// 017b = 4537
// 018  = 6059

#define ZAP_GAME_RELEASE "018"     // Change this with every release -- for display purposes only, string,
                                   // will also be used for name of installer on windows, so be careful with spaces  

#endif

