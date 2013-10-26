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

#ifndef _DATABASE_ACCESS_THREAD_H_
#define _DATABASE_ACCESS_THREAD_H_


#include "tnlThread.h"

namespace Master
{

class MasterSettings;

class DatabaseAccessThread : public TNL::Thread
{
public:

   class BasicEntry : public RefPtrData
   {
   protected:
      const MasterSettings *mSettings;

   public:
      BasicEntry(const MasterSettings *settings) { mSettings = settings; } // Quickie constructor

      virtual void run() = 0;    // runs on seperate thread
      virtual void finish() {};  // finishes the entry on primary thread after "run()" is done to avoid 2 threads crashing in to the same network TNL and others.
   };

private:
   U32 mEntryStart;
   U32 mEntryThread;
   U32 mEntryEnd;

   bool mRunning;

   static const U32 mEntrySize = 32;

   RefPtr<BasicEntry> mEntry[mEntrySize];

public:
   DatabaseAccessThread() // Constructor
   {
      mEntryStart = 0;
      mEntryThread = 0;
      mEntryEnd = 0;

      mRunning = true;
   }


   void addEntry(BasicEntry *entry)
   {
      U32 entryEnd = mEntryEnd + 1;

      if(entryEnd >= mEntrySize)
         entryEnd = 0;

      if(entryEnd == mEntryStart)      // Too many entries
      {
         logprintf(LogConsumer::LogError, "Database thread overloaded - database access too slow?");
         return;
      }

      mEntry[mEntryEnd] = entry;
      mEntryEnd = entryEnd;
   }


   U32 run()
   {
      while(mRunning)
      {
         Platform::sleep(50);
         while(mEntryThread != mEntryEnd)
         {
            mEntry[mEntryThread]->run();
            mEntryThread++;
            if(mEntryThread >= mEntrySize)
               mEntryThread = 0;
         }
      }

      delete this;

      return 0;
   }


   void idle()
   {
      while(mEntryStart != mEntryThread)
      {
         mEntry[mEntryStart]->finish();
         mEntry[mEntryStart].set(NULL);  // RefPtr, we can just set to NULL and it will delete itself
         mEntryStart++;

         if(mEntryStart >= mEntrySize)
            mEntryStart = 0;
      }
   }

   void terminate()
   {
      mRunning = false;
   }
};


}

#endif