//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GameRecorder.h"
#include "tnlBitStream.h"
#include "tnlNetObject.h"
#include "gameType.h"
#include "ServerGame.h"
#include "stringUtils.h"
#include "tnlThread.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIManager.h"
#endif

#include "version.h"

#include <algorithm>

namespace Zap
{



// fwrite might have multiple 1-second freeze on VPS server or heavy disk access
// Having fwrite in separate thread might fix the game from freezing/lagging
// if run in VPS server or with heavy disk access

class WriteBufferThread : public Thread
{
private:
   FILE *f;
   U32 lastPos;
   U32 currPos;
   U8 buffer[1024*128];
   TNL::Semaphore sem1;
   U32 threadPos;
   bool exitNow;
public:

   WriteBufferThread(FILE *file)
   {
      TNLAssert(file != 0, "Must have a file handle");
      lastPos = 0;
      currPos = 0;
      threadPos = 0;
      exitNow = false;
      f = file;
      start();
   }
   ~WriteBufferThread()
   {
      exitNow = true;
      sem1.increment();
      while(f != 0)         // Wait until the other thread is done
         Platform::sleep(1);
   }


   U8 *getBuffer(U32 size)
   {
      if(currPos + size > sizeof(buffer))
      {
         lastPos = currPos;
         currPos = 0;
      }
      U32 t = threadPos;
      while(t > currPos && t < currPos + size) // Buffer too full, waiting...
      {
         Platform::sleep(1);
         t = threadPos;
      }
      return &buffer[currPos];
   }
   void addBuffer(U32 size)
   {
      currPos += size;
      sem1.increment();
   }

   U32 run()
   {
      U32 currPos1 = currPos; // currPos could change anytime by other thread
      while(!exitNow || currPos1 != threadPos)
      {
         if(currPos1 < threadPos)
         {

            fwrite(&buffer[threadPos], 1, lastPos - threadPos, f);
            threadPos = 0;
         }
         else if(currPos1 > threadPos)
         {
            fwrite(&buffer[threadPos], 1, currPos1 - threadPos, f);
            threadPos = currPos1;
         }
         else
            sem1.wait();  // Waits until sem1.increment
         currPos1 = currPos;
      }
      fclose(f);
      f = 0;
      return 0;
   }
};

static void gameRecorderScoping(GameRecorderServer *conn, Game *game)
{
   GameType *gt = game->getGameType();
   if(gt)
      conn->objectLocalScopeAlways(gt);


   const Vector<DatabaseObject *> &gameObjects = *(game->getGameObjDatabase()->findObjects_fast());
   for(S32 i=0; i < gameObjects.size(); i++)
   {
      BfObject *obj = dynamic_cast<BfObject *>(gameObjects[i]);
      if(obj && obj->isGhostable())
         conn->objectLocalScopeAlways(obj);
   }
}


static string newRecordingFileName(const string &dir, const string &levelName, const string &hostName)
{
   makeSureFolderExists(dir);

   Vector<string> files;
   getFilesFromFolder(dir, files);

   S32 max_id = 0;
   for(S32 i = 0; i < files.size(); i++)
   {
      S32 id = Zap::stoi(files[i]);
      if(max_id < id)
         max_id = id;
   }

   string file = itos(max_id + 1);

   string file2 = makeFilenameFromString(levelName.c_str());
   if(file2.size() == 0)
      file2 = makeFilenameFromString(hostName.c_str());
   if(file2.size() != 0)
      file = file + "_" + file2;
   return file;
}

// Constructor
GameRecorderServer::GameRecorderServer(ServerGame *game)
{
   mWriter = NULL;
   mGame = game;
   mMilliSeconds = 0;
   mWriteMaxBitSize = U32_MAX;
   mPackUnpackShipEnergyMeter = true;

   {
      const string &dir = game->getSettings()->getFolderManager()->recordDir;
      mFileName = newRecordingFileName(dir, game->getGameType()->getLevelName(), game->getSettings()->getHostName()) +
            "." + buildGameRecorderExtension();
      string filename = joindir(dir, mFileName);
      FILE *file = fopen(filename.c_str(), "wb");
      if(file)
         mWriter = new WriteBufferThread(file);
   }

   if(mWriter)
   {
      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
      rpcReadyForNormalGhosts_remote(mGhostingSequence);
      setScopeObject(&mNetObj);
      mEventClassCount = NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeEvent);   // Essentially a count of RPCs 
      mEventClassBitSize = getNextBinLog2(mEventClassCount);
      mGhostClassCount = NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeObject);
      mGhostClassBitSize = getNextBinLog2(mGhostClassCount);
      mConnectionParameters.mIsInitiator = false;
      mConnectionParameters.mDebugObjectSizes = false;

      U8 *data = mWriter->getBuffer(4);
      data[0] = CS_PROTOCOL_VERSION;
      data[1] = U8(mGhostClassCount);
      data[2] = U8(mEventClassCount);
      data[3] = U8(mEventClassCount >> 8) | 0x10;
      mWriter->addBuffer(4);
      gameRecorderScoping(this, game);

      s2cSetServerName(game->getSettings()->getHostName());
   }
}

// Destructor
GameRecorderServer::~GameRecorderServer()
{
   if(mWriter)
      delete mWriter;
}


string GameRecorderServer::buildGameRecorderExtension()
{
   string baseRevision = ZAP_GAME_RELEASE;

   // Not a integer, e.g. 019a, then strip off the revision letter
   // Hopefully this pattern never changes
   if(!isInteger(baseRevision.c_str()))
      baseRevision.erase(baseRevision.end() - 1, baseRevision.end());

   // This will create a file extension like 'bf019'
   return "bf" + baseRevision;
}


void GameRecorderServer::idle(U32 MilliSeconds)
{
   if(mWriter == NULL)
      return;

   if(!GhostConnection::isDataToTransmit() && mMilliSeconds + MilliSeconds < (1 << 10) - 200)  // we record milliseconds as 10 bits
   {
      mMilliSeconds += MilliSeconds;
      return;
   }

   GhostPacketNotify notify;
   mNotifyQueueTail = &notify;

   U8 *data = mWriter->getBuffer(16383 + 3);
   BitStream bstream(&data[3], 16383);

   prepareWritePacket();
   GhostConnection::writePacket(&bstream, &notify);
   GhostConnection::packetReceived(&notify);

   mNotifyQueueTail = NULL;

   bstream.zeroToByteBoundary();
   U32 size = bstream.getBytePosition();
   U32 ms = MilliSeconds + mMilliSeconds;
   mMilliSeconds = 0;
   data[0] = U8(size);
   data[1] = U8((size >> 8) & 63) | U8((ms >> 8) << 6);
   data[2] = U8(ms);
   mWriter->addBuffer(bstream.getBytePosition() + 3);
}


}
