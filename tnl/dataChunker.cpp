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

#include "tnl.h"
#include "tnlDataChunker.h"


//----------------------------------------------------------------------------

namespace TNL {

DataChunker::DataChunker(S32 size)
{
   chunkSize          = size;
   curBlock           = new DataBlock(size);
   curBlock->next     = NULL;
   curBlock->curIndex = 0;
}

DataChunker::~DataChunker()
{
   freeBlocks();
}

void *DataChunker::alloc(S32 size)
{
   TNLAssert(size <= chunkSize, "Data chunk too large.");
   if(!curBlock || size + curBlock->curIndex > chunkSize)
   {
      DataBlock *temp = new DataBlock(chunkSize);
      temp->next = curBlock;
      temp->curIndex = 0;
      curBlock = temp;
   }
   void *ret = curBlock->data + curBlock->curIndex;
   curBlock->curIndex += (size + 3) & ~3; // dword align
   return ret;
}

DataChunker::DataBlock::DataBlock(S32 size)
{
   data = new U8[size];
}

DataChunker::DataBlock::~DataBlock()
{
   delete[] data;
}

void DataChunker::freeBlocks()
{
   while(curBlock)
   {
      DataBlock *temp = curBlock->next;
      delete curBlock;
      curBlock = temp;
   }
}

};
