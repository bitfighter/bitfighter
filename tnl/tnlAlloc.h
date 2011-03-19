//-----------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------
//
// scopedAlloc was pulled from the Recast library rcScopedDelete class
//
// Recast is found at:
//    http://code.google.com/p/recastnavigation/
//
// See also the portions of the Recast library modified for Bitfighter in the
// Bitfighter source code
//
//------------------------------------------------------------------------------------

#ifndef _TNL_ALLOC_H_
#define _TNL_ALLOC_H_

namespace TNL
{

// Simple helper class to delete array in scope
template<class T> class scopedAlloc
{
   T* ptr;
   inline T* operator=(T* p);
public:
   inline scopedAlloc() : ptr(0) {}
   inline scopedAlloc(T* p) : ptr(p) {}
   inline ~scopedAlloc() { if(ptr) free(ptr); }
   inline operator T*() { return ptr; }
};

};

#endif // _TNL_ALLOC_H_

