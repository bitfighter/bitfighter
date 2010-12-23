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

#include "tnlJournal.h"
#include "tnlEndian.h"
#include "tnlLog.h"

namespace TNL
{

Vector<JournalEntryRecord *> *JournalEntryRecord::mEntryVector;

bool Journal::mInsideEntrypoint = false;
Journal::Mode Journal::mCurrentMode = Journal::Inactive;
U32 Journal::mReadBreakBitPos = 0;

FILE *Journal::mJournalFile = NULL;
BitStream Journal::mWriteStream;
BitStream Journal::mReadStream;
Journal *Journal::mJournal = NULL;
U32 Journal::mWritePosition = 0;
U32 Journal::mBreakBlockIndex = 0;
U32 Journal::mBlockIndex = 0;

JournalBlockTypeToken *JournalBlockTypeToken::mList = NULL;
bool JournalBlockTypeToken::mInitialized = false;

JournalBlockTypeToken::JournalBlockTypeToken(const char *typeString)
{
   mString = typeString;
   mValue = 0xFFFFFFFF;
   mNext = mList;
   mList = this;
}

S32 QSORT_CALLBACK JBTTCompare(JournalBlockTypeToken **a, JournalBlockTypeToken **b)
{
   return strcmp((*a)->getString(), (*b)->getString());
}

U32 JournalBlockTypeToken::getValue()
{
   if(!mInitialized)
   {
      mInitialized = true;
      Vector<JournalBlockTypeToken *> vec;
      for(JournalBlockTypeToken *walk = mList; walk; walk = walk->mNext)
         vec.push_back(walk);

      vec.sort(JBTTCompare);
      U32 lastValue = 0;
      const char *lastString = "";
      for(S32 i = 0; i < vec.size(); i++)
      {
         if(!strcmp(vec[i]->mString, lastString))
            vec[i]->mValue = lastValue;
         else
         {
            lastValue++;
            vec[i]->mValue = lastValue;
            lastString = vec[i]->mString;
         }
      }
   }
   return mValue;
}

const char *JournalBlockTypeToken::findName(U32 value)
{
   for(JournalBlockTypeToken *walk = mList; walk; walk = walk->mNext)
      if(walk->mValue == value)
         return walk->mString;
   return "INVALID";
}

JournalEntryRecord::JournalEntryRecord(const char *functionName)
{
   S32 i;
   if(!mEntryVector)
      mEntryVector = new Vector<JournalEntryRecord *>;

   for(i = 0; i < mEntryVector->size(); i++)
   {
      if(strcmp((*mEntryVector)[i]->mFunctionName, functionName) < 0)
         break;
   }
   mEntryVector->insert(i);
   (*mEntryVector)[i] = this;
   mFunctionName = functionName;

   mEntryIndex = 0;
}

JournalEntryRecord::~JournalEntryRecord()
{
   if(mEntryVector)
   {
      delete mEntryVector;
      mEntryVector = NULL;
   }
}

Journal::Journal()
{
   TNLAssert(mJournal == NULL, "Cannot construct more than one Journal instance.");
   mJournal = this;
}

// the journal stream is written as a single continuous bit stream.
// the first four bytes of the stream are the total number of bits in
// the stream.  As a block is written, the bits in the write stream are
// all erased except for those in the last byte, which are moved to the first.
void Journal::syncWriteStream()
{
   if(mWriteStream.getBytePosition() == 0)
      return;

   U32 totalBits = (mWritePosition << 3) + mWriteStream.getBitPosition();
   
   // seek back to the beginning
   fseek(mJournalFile, 0, SEEK_SET);

   // Use this variable to suppress warnings on gcc, value never used
   size_t a;
   // write the new total bits
   U32 writeBits = convertHostToLEndian(totalBits);
   a = fwrite(&writeBits, 1, sizeof(U32), mJournalFile);

   // seek to the writing position
   fseek(mJournalFile, mWritePosition, SEEK_SET);

   U32 bytesToWrite = mWriteStream.getBytePosition();
   // write the bytes to the file
   a = fwrite(mWriteStream.getBuffer(), 1, bytesToWrite, mJournalFile);
   fflush(mJournalFile);

   // adjust the write stream
   if(totalBits & 7)
   {
      U8 *buffer = mWriteStream.getBuffer();
      buffer[0] = buffer[bytesToWrite - 1];
      mWriteStream.setBitPosition(totalBits & 7);
      mWritePosition += bytesToWrite - 1;
   }
   else
   {
      mWritePosition += bytesToWrite;
      mWriteStream.setBitPosition(0);
   }
}

void Journal::record(const char *fileName)
{
   mJournalFile = fopen(fileName, "wb");
   if(mJournalFile)
   {
      mCurrentMode = Record;
      mWritePosition = sizeof(U32);
   }
}

void Journal::load(const char *fileName)
{
   FILE *theJournal = fopen(fileName, "rb");
   if(!theJournal)
      return;

   fseek(theJournal, 0, SEEK_END);
   U32 fileSize = ftell(theJournal);
   fseek(theJournal, 0, SEEK_SET);

   bool a;   // a and b only here to suppress warnings on cpp, not used elsewhere
   size_t b;

   a = mReadStream.resize(fileSize);   
   U32 bitCount;
   b = fread(mReadStream.getBuffer(), 1, fileSize, theJournal);
   mReadStream.read(&bitCount);
   mReadStream.setMaxBitSizes(bitCount);

   if(!mReadBreakBitPos || mReadBreakBitPos > bitCount)
      mReadBreakBitPos = bitCount;

   fclose(theJournal);
   mCurrentMode = Playback;
}

void Journal::callEntry(const char *funcName, Functor *theCall)
{
   if(mCurrentMode == Playback)
      return;

   TNLAssert(mInsideEntrypoint == false, "Journal entries cannot be reentrant!");
   mInsideEntrypoint = true;

   S32 entryIndex;
   for(entryIndex = 0; entryIndex < JournalEntryRecord::mEntryVector->size(); entryIndex++)
   {
      if(!strcmp((*JournalEntryRecord::mEntryVector)[entryIndex]->mFunctionName, funcName))
         break;
   }
   TNLAssert(entryIndex != JournalEntryRecord::mEntryVector->size(), "No entry point found!");

   if(mCurrentMode == Record)
   {
#ifdef TNL_ENABLE_BIG_JOURNALS
      TNL_JOURNAL_WRITE( (U16(0x1234)) );
#endif
      mWriteStream.writeRangedU32(entryIndex, 0, JournalEntryRecord::mEntryVector->size() - 1);
      theCall->write(mWriteStream);
#ifdef TNL_ENABLE_BIG_JOURNALS
      TNL_JOURNAL_WRITE( (U16(0x5678)) );
#endif
      syncWriteStream();
   }
   theCall->dispatch(this);
   mInsideEntrypoint = false;
}

void Journal::checkReadPosition()
{
   if(!mReadStream.isValid() || mReadStream.getBitPosition() > mReadBreakBitPos)    // Was >=, but that caused crashing on very last command of the file.  
                                                                                    // Changing to > fixes the problem, or at least the symptom.
   {
      if(!mReadStream.isValid())
         logprintf(LogConsumer::LogFatalError, "checkReadPosition failed: Invalid stream read");
      else
         logprintf(LogConsumer::LogFatalError, "checkReadPosition failed: Read past end of journal");

      TNL_DEBUGBREAK();
   }
}

void Journal::beginBlock(U32 blockId, bool writeBlock)
{
   if(writeBlock)
   {
#ifdef TNL_ENABLE_BIG_JOURNALS
      TNL_JOURNAL_WRITE( (U16(0x1234 ^ blockId)) );
#endif
   }
   else
   {
      mBlockIndex++;
      if(mBreakBlockIndex && mBlockIndex >= mBreakBlockIndex)
         TNL_DEBUGBREAK();

#ifdef TNL_ENABLE_BIG_JOURNALS
      U16 startToken;
      TNL_JOURNAL_READ( (&startToken) );
      if((startToken ^ 0x1234) != blockId)
      {
         logprintf(LogConsumer::LogFatalError, "Expected token %s - got %s", JournalBlockTypeToken::findName(blockId), JournalBlockTypeToken::findName(startToken ^ 0x1234);
         TNL_DEBUGBREAK();
      }
#endif
   }
}

void Journal::endBlock(U32 blockId, bool writeBlock)
{
   if(writeBlock)
   {
#ifdef TNL_ENABLE_BIG_JOURNALS
      TNL_JOURNAL_WRITE( (U16(0x5678 ^ blockId)) );
#endif
      syncWriteStream();
   }
   else
   {
#ifdef TNL_ENABLE_BIG_JOURNALS
      U16 endToken;
      TNL_JOURNAL_READ( (&endToken) );
      if((endToken ^ 0x5678) != blockId)
      {
         logprintf(LogConsumer::LogFatalError, "Expected token %s - got %s", JournalBlockTypeToken::findName(blockId), JournalBlockTypeToken::findName(endToken ^ 0x5678);
         TNL_DEBUGBREAK();
      }
#endif
      checkReadPosition();
   }
}

void Journal::processNextJournalEntry()
{
   if(mCurrentMode != Playback)
      return;

#ifdef TNL_ENABLE_BIG_JOURNALS
   U16 token;
   TNL_JOURNAL_READ( (&token) );
   if(token != 0x1234)
      TNL_DEBUGBREAK();
#endif

   U32 index = mReadStream.readRangedU32(0, JournalEntryRecord::mEntryVector->size());

   JournalEntryRecord *theEntry = (*JournalEntryRecord::mEntryVector)[index];

   // check for errors...
   if(!theEntry)
   {
      TNLAssert(0, "blech!");
   }
   theEntry->mFunctor->read(mReadStream);

#ifdef TNL_ENABLE_BIG_JOURNALS
   TNL_JOURNAL_READ( (&token) );
   if(token != 0x5678)
      TNL_DEBUGBREAK();
#endif

   checkReadPosition();

   mInsideEntrypoint = true;
   theEntry->mFunctor->dispatch(this);
   mInsideEntrypoint = false;
}

};

