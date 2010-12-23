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

#ifndef _TNL_CLIENTPUZZLE_H_
#define _TNL_CLIENTPUZZLE_H_

#include "tnlDataChunker.h"
#include "tnlNonce.h"

// JMQ: work around X.h header file
#if defined(TNL_OS_LINUX) && defined(Success)
#undef Success
#endif

namespace TNL {

/// The ClientPuzzleManager class issues, solves and validates client
/// puzzles for connection authentication.
class ClientPuzzleManager
{
public:
private:
   /// NonceTable manages the list of client nonces for which clients
   /// have constructed valid puzzle solutions for the current server
   /// nonce.  There are 2 nonce tables in the ClientPuzzleManager -
   /// one for the current nonce and one for the previous nonce.

   class NonceTable {
    private:
      struct Entry {
         Nonce mNonce;
         Entry *mHashNext;
      };
      enum {
         MinHashTableSize = 127,
         MaxHashTableSize = 387,
      };

      Entry **mHashTable;
      U32 mHashTableSize;
      DataChunker mChunker;

    public:
      /// NonceTable constructor
      NonceTable() { reset(); }

      /// Resets and clears the nonce table
      void reset();

      /// checks if the given nonce is already in the table and adds it
      /// if it is not.  Returns true if the nonce was not in the table
      /// when the function was called.
      bool checkAdd(Nonce &theNonce);
   };

   U32 mCurrentDifficulty;
   U32 mLastUpdateTime;
   U32 mLastTickTime;

   Nonce mCurrentNonce;
   Nonce mLastNonce;

   NonceTable *mCurrentNonceTable;
   NonceTable *mLastNonceTable;
   static bool checkOneSolution(U32 solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity);
public:
   ClientPuzzleManager();
   ~ClientPuzzleManager();

   /// Checks to see if a new nonce needs to be created, and if so
   /// generates one and tosses out the current list of accepted nonces
   void tick(U32 currentTime);

   /// Error codes that can be returned by checkSolution
   enum ErrorCode {
      Success,
      InvalidSolution,
      InvalidServerNonce,
      InvalidClientNonce,
      InvalidPuzzleDifficulty,
      ErrorCodeCount,
   };

   /// Difficulty levels of puzzles
   enum {
      PuzzleRefreshTime          = 30000, ///< Refresh the server puzzle every 30 seconds
      InitialPuzzleDifficulty    = 17, ///< Initial puzzle difficulty is set so clients do approx 2-3x the shared secret
                                       ///  generation of the server
      MaxPuzzleDifficulty        = 26, ///< Maximum puzzle difficulty is approx 1 minute to solve on ~2004 hardware.
      MaxSolutionComputeFragment = 30, ///< Number of milliseconds spent computing solution per call to solvePuzzle.
      SolutionFragmentIterations = 50000, ///< Number of attempts to spend on the client puzzle per call to solvePuzzle.
   };

   /// Checks a puzzle solution submitted by a client to see if it is a valid solution for the current or previous puzzle nonces
   ErrorCode checkSolution(U32 solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity);

   /// Computes a puzzle solution value for the given puzzle difficulty and server nonce.  If the execution time of this function
   /// exceeds MaxSolutionComputeFragment milliseconds, it will return the current trail solution in the solution variable and a
   /// return value of false.
   ///
   /// @note Although the behavior of this function can be tweaked using MaxSolutionComputeFragment and
   ///       SolutionFragmentIterations, it's important to bias these settings in favor of rapid puzzle
   ///       completion. A client puzzle is only valid for two times PuzzleRefreshTime, so for about a
   ///       minute, maximum. Most of the time the puzzle can be solved in only a few hundred
   ///       milliseconds. It's better to solve the puzzle fast than to let it drag out, (ie, it's better to
   ///       let your application hitch for a moment whilst calculating than to make the user endure many
   ///       seconds of lag) so reducing the timeout or iterations should be done only if you know what
   ///       you're doing.
   ///
   static bool solvePuzzle(U32 *solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity);

   /// Returns the current server nonce
   Nonce getCurrentNonce() { return mCurrentNonce; }

   /// Returns the current client puzzle difficulty
   U32 getCurrentDifficulty() { return mCurrentDifficulty; }
};

};

#endif
