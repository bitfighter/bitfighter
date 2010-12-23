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

#ifndef _TNL_HUFFMANSTRINGPROCESSOR_H_
#define _TNL_HUFFMANSTRINGPROCESSOR_H_

namespace TNL {

/// HuffmanStringProcessor does Huffman coding on strings written into
/// BitStream objects.
namespace HuffmanStringProcessor
{
   static const S32 MAX_SENDABLE_LINE_LENGTH = 255;
   /// Reads a Huffman compressed string out of a BitStream.
   bool readHuffBuffer(BitStream* pStream, char* out_pBuffer);

   /// Writes and Huffman compresses a string into a BitStream.
   ///
   /// @param   pStream         Stream to output compressed data to.
   /// @param   out_pBuffer     String to compress.
   /// @param   maxLen          Maximum length of the string. If string length
   ///                          exceeds this, then the string is terminated
   ///                          early.
   ///
   /// @note The Huffman encoder uses BitStream::writeString as a fallback.
   ///       WriteString can only write strings of up to 255 characters length.
   ///       Therefore, it is wise not to exceed that limit.
   bool writeHuffBuffer(BitStream* pStream, const char* out_pBuffer, U32 maxLen);
};

};

#endif
