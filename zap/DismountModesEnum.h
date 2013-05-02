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

#ifndef _DISMOUNT_MODES_ENUM_H
#define _DISMOUNT_MODES_ENUM_H

namespace Zap 
{

// Reasons/modes we might dismount an item
enum DismountMode
{
   DISMOUNT_NORMAL,              // Item was dismounted under normal circumstances
   DISMOUNT_MOUNT_WAS_KILLED,    // Item was dismounted due to death of mount
   DISMOUNT_SILENT,              // Item was dismounted, do not make an announcement
};


}

#endif