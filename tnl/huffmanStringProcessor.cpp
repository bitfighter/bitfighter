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

#include "tnlBitStream.h"
#include "tnlVector.h"
#include "tnlHuffmanStringProcessor.h"

namespace TNL {

namespace HuffmanStringProcessor {
   // This is extern so we can keep our huge hairy table definition at the end of the file.
   extern const U32 mCharFreqs[256];
   bool mTablesBuilt = false;

   struct HuffNode {
      U32 pop;

      S16 index0;
      S16 index1;
   };
   struct HuffLeaf {
      U32 pop;

      U8  numBits;
      U8  symbol;
      U32 code;   // no code should be longer than 32 bits.
   };

   Vector<HuffNode> mHuffNodes;
   Vector<HuffLeaf> mHuffLeaves;

   void buildTables();

   // We have to be a bit careful with these, since they are pointers...
   struct HuffWrap {
      HuffNode* pNode;
      HuffLeaf* pLeaf;

     public:
      HuffWrap() : pNode(NULL), pLeaf(NULL) { }

      void set(HuffLeaf* in_leaf) { pNode = NULL; pLeaf = in_leaf; }
      void set(HuffNode* in_node) { pLeaf = NULL; pNode = in_node; }

      U32 getPop() { if (pNode) return pNode->pop; else return pLeaf->pop; }
   };


   S16 determineIndex(HuffWrap&);

   void generateCodes(BitStream&, S32, S32);
};

//bool HuffmanStringProcessor::mTablesBuilt = false;
//Vector<HuffmanStringProcessor::HuffNode> HuffmanStringProcessor::mHuffNodes;
//Vector<HuffmanStringProcessor::HuffLeaf> HuffmanStringProcessor::mHuffLeaves;


void HuffmanStringProcessor::buildTables()
{
   TNLAssert(mTablesBuilt == false, "Cannot build tables twice!");
   mTablesBuilt = true;

   S32 i;

   // First, construct the array of wraps...
   //
   mHuffLeaves.resize(256);
   mHuffNodes.reserve(256);
   mHuffNodes.resize(mHuffNodes.size() + 1);
   for (i = 0; i < 256; i++) {
      HuffLeaf& rLeaf = mHuffLeaves[i];

      rLeaf.pop    = mCharFreqs[i] + 1;
      rLeaf.symbol = U8(i);

      memset(&rLeaf.code, 0, sizeof(rLeaf.code));
      rLeaf.numBits = 0;
   }

   S32 currWraps = 256;
   HuffWrap* pWrap = new HuffWrap[256];
   for (i = 0; i < 256; i++) {
      pWrap[i].set(&mHuffLeaves[i]);
   }

   while (currWraps != 1) {
      U32 min1 = 0xfffffffe, min2 = 0xffffffff;
      S32 index1 = -1, index2 = -1;

      for (i = 0; i < currWraps; i++) {
         if (pWrap[i].getPop() < min1) {
            min2   = min1;
            index2 = index1;

            min1   = pWrap[i].getPop();
            index1 = i;
         } else if (pWrap[i].getPop() < min2) {
            min2   = pWrap[i].getPop();
            index2 = i;
         }
      }
      TNLAssert(index1 != -1 && index2 != -1 && index1 != index2, "hrph");

      // Create a node for this...
      mHuffNodes.resize(mHuffNodes.size() + 1);
      HuffNode& rNode = mHuffNodes.last();
      rNode.pop    = pWrap[index1].getPop() + pWrap[index2].getPop();
      rNode.index0 = determineIndex(pWrap[index1]);
      rNode.index1 = determineIndex(pWrap[index2]);

      S32 mergeIndex = index1 > index2 ? index2 : index1;
      S32 nukeIndex  = index1 > index2 ? index1 : index2;
      pWrap[mergeIndex].set(&rNode);

      if (index2 != (currWraps - 1)) {
         pWrap[nukeIndex] = pWrap[currWraps - 1];
      }
      currWraps--;
   }
   TNLAssert(currWraps == 1, "wrong wraps?");
   TNLAssert(pWrap[0].pNode != NULL && pWrap[0].pLeaf == NULL, "Wrong wrap type!");

   // Ok, now we have one wrap, which is a node.  we need to make sure that this
   //  is the first node in the node list.
   mHuffNodes[0] = *(pWrap[0].pNode);
   delete [] pWrap;

   U32 code = 0;
   BitStream bs((U8 *) &code, 4);

   generateCodes(bs, 0, 0);
}

void HuffmanStringProcessor::generateCodes(BitStream& rBS, S32 index, S32 depth)
{
   if (index < 0) {
      // leaf node, copy the code in, and back out...
      HuffLeaf& rLeaf = mHuffLeaves[-(index + 1)];

      memcpy(&rLeaf.code, rBS.getBuffer(), sizeof(rLeaf.code));
      rLeaf.numBits = depth;
   } else {
      HuffNode& rNode = mHuffNodes[index];

      S32 pos = rBS.getBitPosition();

      rBS.writeFlag(false);
      generateCodes(rBS, rNode.index0, depth + 1);

      rBS.setBitPosition(pos);
      rBS.writeFlag(true);
      generateCodes(rBS, rNode.index1, depth + 1);

      rBS.setBitPosition(pos);
   }
}

S16 HuffmanStringProcessor::determineIndex(HuffWrap& rWrap)
{
   if (rWrap.pLeaf != NULL) {
      TNLAssert(rWrap.pNode == NULL, "um, never.");

      return -((rWrap.pLeaf - mHuffLeaves.address()) + 1);
   } else {
      TNLAssert(rWrap.pNode != NULL, "um, never.");

      return rWrap.pNode - mHuffNodes.address();
   }
}

bool HuffmanStringProcessor::readHuffBuffer(BitStream* pStream, char* out_pBuffer)
{
   if (mTablesBuilt == false)
      buildTables();

   if (pStream->readFlag()) {
      U32 len = pStream->readInt(8);
      for (U32 i = 0; i < len; i++) {
         S32 index = 0;
         while (true) {
            if (index >= 0) {
               if (pStream->readFlag() == true) {
                  index = mHuffNodes[index].index1;
               } else {
                  index = mHuffNodes[index].index0;
               }
            } else {
               out_pBuffer[i] = mHuffLeaves[-(index+1)].symbol;
               break;
            }
         }
      }
      out_pBuffer[len] = '\0';
      return true;
   } else {
      // Uncompressed string...
      U32 len = pStream->readInt(8);
      pStream->read(len, out_pBuffer);
      out_pBuffer[len] = '\0';
      return true;
   }
}

bool HuffmanStringProcessor::writeHuffBuffer(BitStream* pStream, const char* out_pBuffer, U32 maxLen)
{
   if (out_pBuffer == NULL) {
      pStream->writeFlag(false);
      pStream->writeInt(0, 8);
      return true;
   }

   if (mTablesBuilt == false)
      buildTables();

   U32 len = out_pBuffer ? strlen(out_pBuffer) : 0;
   TNLAssertV(len <= MAX_SENDABLE_LINE_LENGTH, ("String \"%s\" TOO long for writeString", out_pBuffer));
   if (len > maxLen)
      len = maxLen;

   U32 numBits = 0;
   U32 i;
   for (i = 0; i < len; i++)
      numBits += mHuffLeaves[(unsigned char)out_pBuffer[i]].numBits;

   if (numBits >= (len * 8)) {
      pStream->writeFlag(false);
      pStream->writeInt(len, 8);
      pStream->write(len, out_pBuffer);
   } else {
      pStream->writeFlag(true);
      pStream->writeInt(len, 8);
      for (i = 0; i < len; i++) {
         HuffLeaf& rLeaf = mHuffLeaves[((unsigned char)out_pBuffer[i])];
         pStream->writeBits(rLeaf.numBits, &rLeaf.code);
      }
   }

   return true;
}

const U32 HuffmanStringProcessor::mCharFreqs[256] = {
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
329   ,
21    ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
2809  ,
68    ,
0     ,
27    ,
0     ,
58    ,
3     ,
62    ,
4     ,
7     ,
0     ,
0     ,
15    ,
65    ,
554   ,
3     ,
394   ,
404   ,
189   ,
117   ,
30    ,
51    ,
27    ,
15    ,
34    ,
32    ,
80    ,
1     ,
142   ,
3     ,
142   ,
39    ,
0     ,
144   ,
125   ,
44    ,
122   ,
275   ,
70    ,
135   ,
61    ,
127   ,
8     ,
12    ,
113   ,
246   ,
122   ,
36    ,
185   ,
1     ,
149   ,
309   ,
335   ,
12    ,
11    ,
14    ,
54    ,
151   ,
0     ,
0     ,
2     ,
0     ,
0     ,
211   ,
0     ,
2090  ,
344   ,
736   ,
993   ,
2872  ,
701   ,
605   ,
646   ,
1552  ,
328   ,
305   ,
1240  ,
735   ,
1533  ,
1713  ,
562   ,
3     ,
1775  ,
1149  ,
1469  ,
979   ,
407   ,
553   ,
59    ,
279   ,
31    ,
0     ,
0     ,
0     ,
68    ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0     ,
0
};

};
