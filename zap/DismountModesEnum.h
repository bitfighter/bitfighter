//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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
