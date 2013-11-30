//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _DATABASE_ACCESS_THREAD_H_
#define _DATABASE_ACCESS_THREAD_H_


#include "tnlThread.h"
#include "tnlLog.h"

namespace Master
{

class ThreadEntry : public RefPtrData
{
public:
   virtual ~ThreadEntry() {};

   virtual void run() = 0;    // runs on seperate thread
   virtual void finish() {};  // finishes the entry on primary thread after "run()" is done to avoid 2 threads crashing in to the same network TNL and others.
};

class DatabaseAccessThread : public TNL::Thread
{
private:
   U32 mEntryStart;
   U32 mEntryThread;
   U32 mEntryEnd;

   bool mRunning;
   bool mThreadActive;

   static const U32 mEntrySize = 128;

   RefPtr<ThreadEntry> mEntry[mEntrySize];

public:
   DatabaseAccessThread() // Constructor
   {
      mEntryStart = 0;
      mEntryThread = 0;
      mEntryEnd = 0;

      mRunning = true;
      mThreadActive = false;
   }


   void addEntry(ThreadEntry *entry)
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
      if(!mThreadActive)
      {
         mThreadActive = true;
         start();
      }
   }


   U32 run()
   {
      mThreadActive = true;
      while(mRunning)
      {
         if(mEntryThread != mEntryEnd)
         {
            mEntry[mEntryThread]->run();
            mEntryThread++;
            if(mEntryThread >= mEntrySize)
               mEntryThread = 0;
         }
         else
            Platform::sleep(50);
      }

      mThreadActive = false;

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
      while(mThreadActive)
         Platform::sleep(50);
   }

   ~DatabaseAccessThread()
   {
      terminate();
   }

};


}

#endif