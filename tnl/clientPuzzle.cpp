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
#include "tnlClientPuzzle.h"
#include "tnlRandom.h"

#include "../libtomcrypt/mycrypt.h"

namespace TNL {

void ClientPuzzleManager::NonceTable::reset()
{
   mChunker.freeBlocks();
   mHashTableSize = Random::readI(MinHashTableSize, MaxHashTableSize) * 2 + 1;
   mHashTable = (Entry **) mChunker.alloc(sizeof(Entry *) * mHashTableSize);
   for(U32 i = 0; i < mHashTableSize; i++)
      mHashTable[i] = NULL;
}

bool ClientPuzzleManager::NonceTable::checkAdd(Nonce &theNonce)
{
   U32 nonce1 = readU32FromBuffer(theNonce.data);
   U32 nonce2 = readU32FromBuffer(theNonce.data + 4);

   U64 fullNonce = (U64(nonce1) << 32) | nonce2;

   U32 hashIndex = U32(fullNonce % mHashTableSize);
   for(Entry *walk = mHashTable[hashIndex]; walk; walk = walk->mHashNext)
      if(walk->mNonce == theNonce)
         return false;
   Entry *newEntry = (Entry *) mChunker.alloc(sizeof(Entry));
   newEntry->mNonce = theNonce;
   newEntry->mHashNext = mHashTable[hashIndex];
   mHashTable[hashIndex] = newEntry;
   return true;
}

ClientPuzzleManager::ClientPuzzleManager()
{
   mCurrentDifficulty = InitialPuzzleDifficulty;
   mLastUpdateTime = 0;
   mLastTickTime = 0;
   //Random::read(mCurrentNonce.data, Nonce::NonceSize);
   //Random::read(mLastNonce.data, Nonce::NonceSize);
   mCurrentNonce.getRandom();
   mLastNonce.getRandom();

   mCurrentNonceTable = new NonceTable;
   mLastNonceTable = new NonceTable;
}

ClientPuzzleManager::~ClientPuzzleManager()
{
   delete mCurrentNonceTable;
   delete mLastNonceTable;
}

void ClientPuzzleManager::tick(U32 currentTime)
{
   if(!mLastTickTime)
      mLastTickTime = currentTime;

   // use delta of last tick time and current time to manage puzzle
   // difficulty.

   // not yet implemented.


   // see if it's time to refresh the current puzzle:
   U32 timeDelta = currentTime - mLastUpdateTime;
   if(timeDelta > PuzzleRefreshTime)
   {
      mLastUpdateTime = currentTime;
      mLastNonce = mCurrentNonce;
      NonceTable *tempTable = mLastNonceTable;
      mLastNonceTable = mCurrentNonceTable;
      mCurrentNonceTable = tempTable;

      mLastNonce = mCurrentNonce;
      mCurrentNonceTable->reset();
      //Random::read(mCurrentNonce.data, Nonce::NonceSize);
      mCurrentNonce.getRandom();
   }
}

bool ClientPuzzleManager::checkOneSolution(U32 solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity)
{
   U8 buffer[8];
   writeU32ToBuffer(solution, buffer);
   writeU32ToBuffer(clientIdentity, buffer + 4);

   hash_state hashState;
   U8 hash[32];

   sha256_init(&hashState);
   sha256_process(&hashState, buffer, sizeof(buffer));
   sha256_process(&hashState, clientNonce.data, Nonce::NonceSize);
   sha256_process(&hashState, serverNonce.data, Nonce::NonceSize);
   sha256_done(&hashState, hash);

   U32 index = 0;
   while(puzzleDifficulty > 8)
   {
      if(hash[index])
         return false;
      index++;
      puzzleDifficulty -= 8;
   }
   U8 mask = 0xFF << (8 - puzzleDifficulty);
   return (mask & hash[index]) == 0;
}

ClientPuzzleManager::ErrorCode ClientPuzzleManager::checkSolution(U32 solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity)
{
   if(puzzleDifficulty != mCurrentDifficulty)
      return InvalidPuzzleDifficulty;
   NonceTable *theTable = NULL;
   if(serverNonce == mCurrentNonce)
      theTable = mCurrentNonceTable;
   else if(serverNonce == mLastNonce)
      theTable = mLastNonceTable;
   if(!theTable)
      return InvalidServerNonce;
   if(!checkOneSolution(solution, clientNonce, serverNonce, puzzleDifficulty, clientIdentity))
      return InvalidSolution;
   if(!theTable->checkAdd(clientNonce))
      return InvalidClientNonce;
   return Success;
}

bool ClientPuzzleManager::solvePuzzle(U32 *solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity)
{
   U32 startTime = Platform::getRealMilliseconds();
   U32 startValue = *solution;

   // Until we're done...
   for(;;)
   {
      U32 nextValue = startValue + SolutionFragmentIterations;
      for(;startValue < nextValue; startValue++)
      {
         if(checkOneSolution(startValue, clientNonce, serverNonce, puzzleDifficulty, clientIdentity))
         {
            *solution = startValue;
            return true;
         }
      }

      // Then we check to see if we're out of time...
      if(Platform::getRealMilliseconds() - startTime > MaxSolutionComputeFragment)
      {
         *solution = startValue;
         return false;
      }
   }
}

};
