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

#ifndef _TNL_JOURNAL_H_
#define _TNL_JOURNAL_H_

#include "tnlMethodDispatch.h"
#include <stdio.h>

#define TNL_ENABLE_JOURNALING
//#define TNL_ENABLE_BIG_JOURNALS
namespace TNL
{

/// The Journal class represents the recordable entry point(s) into program execution.
/// When journaling is enabled by the TNL_ENABLE_JOURNALING macro, any calls into specially
/// marked Journal methods will be intercepted and potentially recorded for later playback.
/// If TNL_ENABLE_JOURNALING is not defined, all of the interception code will be disabled.

class Journal : public Object
{
   static FILE *mJournalFile;
   static BitStream mReadStream;
   static BitStream mWriteStream;
   static Journal *mJournal;
   static U32 mWritePosition;
   static U32 mReadBreakBitPos;
   static U32 mBreakBlockIndex;
   static U32 mBlockIndex;
public:
   enum Mode
   {
      Inactive,
      Record,
      Playback,
   };
protected:
   static Mode mCurrentMode;
   static bool mInsideEntrypoint;
   static void checkReadPosition();
   static void syncWriteStream();
public:
   Journal();
   void record(const char *fileName);
   void load(const char *fileName);

   void callEntry(const char *funcName, Functor *theCall);
   void processNextJournalEntry();

   static Mode getCurrentMode() { return mCurrentMode; }
   static Journal *get() { return mJournal; }
   static BitStream *getReadStream() { return &mReadStream; }
   static BitStream *getWriteStream() { return &mWriteStream; }
   static bool isInEntrypoint() { return mInsideEntrypoint; }

   static void beginBlock(U32 blockId, bool writeBlock);
   static void endBlock(U32 blockId, bool writeBlock);
};

struct JournalEntryRecord
{
   U32 mEntryIndex;
   const char *mFunctionName;
   JournalEntryRecord *mNext;
   Functor *mFunctor;

   static Vector<JournalEntryRecord *> *mEntryVector;

   JournalEntryRecord(const char *functionName);
   virtual ~JournalEntryRecord();
};

#ifdef TNL_ENABLE_JOURNALING
#define TNL_DECLARE_JOURNAL_ENTRYPOINT(func, args) \
      void func args; \
      void func##_body args

#define TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(className, func, args, argNames) \
      struct Journal_##className##_##func##_er : public JournalEntryRecord { \
      FunctorDecl<void (className::*) args> mFunctorDecl; \
      Journal_##className##_##func##_er(const char *name) : JournalEntryRecord(name), mFunctorDecl(&className::func##_body) { mFunctor = &mFunctorDecl; } \
      } gJournal_##className##_##func##_er(#func); \
      void className::func args { \
      gJournal_##className##_##func##_er.mFunctorDecl.set argNames; \
         callEntry(#func, gJournal_##className##_##func##_er.mFunctor); \
      } \
      void className::func##_body args

class JournalToken
{
   bool mWriting;
   U32 mBlockType;
public:
   JournalToken(U32 blockType, bool writing)
   {
      mWriting = writing;
      mBlockType = blockType;
      TNL::Journal::beginBlock(mBlockType, mWriting);
   }

   ~JournalToken()
   {
      TNL::Journal::endBlock(mBlockType, mWriting);
   }
};

class JournalBlockTypeToken
{
   const char *mString;
   U32 mValue;
   JournalBlockTypeToken *mNext;
   static bool mInitialized;
   static JournalBlockTypeToken *mList;
public:
   JournalBlockTypeToken(const char *typeString);
   U32 getValue();
   static const char *findName(U32 value);
   const char *getString() { return mString; }
};

#define TNL_JOURNAL_WRITE_BLOCK(blockType, x) \
{ \
   if(TNL::Journal::getCurrentMode() == TNL::Journal::Record && TNL::Journal::isInEntrypoint()) \
   { \
      static TNL::JournalBlockTypeToken typeToken(#blockType);\
      TNL::JournalToken dummy(typeToken.getValue(), true); \
      { \
      x \
      } \
   } \
}

#define TNL_JOURNAL_READ_BLOCK(blockType, x) \
{ \
   if(TNL::Journal::getCurrentMode() == TNL::Journal::Playback && TNL::Journal::isInEntrypoint()) \
   { \
      static TNL::JournalBlockTypeToken typeToken(#blockType);\
      TNL::JournalToken dummy(typeToken.getValue(), false); \
      { \
      x \
      } \
   } \
}

#define TNL_JOURNAL_READ(x) \
   TNL::Journal::getReadStream()->read x

#define TNL_JOURNAL_WRITE(x) \
   TNL::Journal::getWriteStream()->write x

#else
#define TNL_DECLARE_JOURNAL_ENTRYPOINT(func, args) \
      void func args

#define TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(className, func, args) \
   void className::func args

#define TNL_JOURNAL_WRITE_BLOCK(blockType, x)
#define TNL_JOURNAL_READ_BLOCK(blockType, x)

#endif
};


#endif

