//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "tnlVector.h"

namespace TNL {

bool VectorResize(U32 *aSize, U32 *aCount, void **arrayPtr, U32 newCount, U32 elemSize)
{
   if (newCount > 0) {
      U32 blocks = newCount / VectorBlockSize;
      if (newCount % VectorBlockSize)
         blocks++;
      S32 mem_size = blocks * VectorBlockSize * elemSize;
      *arrayPtr = *arrayPtr ? realloc(*arrayPtr,mem_size) :
         malloc(mem_size);

      *aCount = newCount;
      *aSize = blocks * VectorBlockSize;
      return true;
   }
   if (*arrayPtr) {
      free(*arrayPtr);
      *arrayPtr = 0;
   }

   *aSize = 0;
   *aCount = 0;
   return true;
}

};
