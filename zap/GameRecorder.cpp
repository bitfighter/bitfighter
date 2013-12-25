#include "GameRecorder.h"
#include "tnlBitStream.h"
#include "tnlNetObject.h"
#include "gameType.h"
#include "ServerGame.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#endif

#include "version.h"

namespace Zap
{

static void gameRecorderScoping(GameRecorderServer *conn, ServerGame *game)
{
   GameType *gt = game->getGameType();
   if(gt)
      conn->objectInScope(gt);


   const Vector<DatabaseObject *> &gameObjects = *(game->getGameObjDatabase()->findObjects_fast());
   for(S32 i=0; i < gameObjects.size(); i++)
   {
      BfObject *obj = dynamic_cast<BfObject *>(gameObjects[i]);
      if(obj && obj->isGhostable())
         conn->objectLocalScopeAlways(obj);
   }
}

GameRecorderServer::GameRecorderServer(ServerGame *game)
{
   mFile = NULL;
   mGame = game;
   mMilliSeconds = 0;
   mWriteMaxBitSize = U32_MAX;

   if(!mFile)
      mFile = fopen("record.bin", "wb");

   if(mFile)
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

      U8 data[4];
      data[0] = CS_PROTOCOL_VERSION;
      data[1] = U8(mGhostClassCount);
      data[2] = U8(mEventClassCount);
      data[3] = U8(mEventClassCount >> 8);
      fwrite(data, 1, 4, mFile);
   }
   gameRecorderScoping(this, game);
}
GameRecorderServer::~GameRecorderServer()
{
   if(mFile)
      fclose(mFile);
}



void GameRecorderServer::idle(U32 MilliSeconds)
{
   if(!mFile)
      return;

   if(!GhostConnection::isDataToTransmit() && mMilliSeconds + MilliSeconds < (1 << 10) - 200)  // we record milliseconds as 12 bits
   {
      mMilliSeconds += MilliSeconds;
      return;
   }

   GhostPacketNotify notify;
   mNotifyQueueTail = &notify;

   U8 data[16384 + 3 - 1];  // 16 KB on stack memory (no memory allocation/deallocation speed cost)
   BitStream bstream(&data[3], sizeof(data) - 3);

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
   fwrite(data, 1, bstream.getBytePosition() + 3, mFile);
}


#ifndef ZAP_DEDICATED


GameRecorderPlayback::GameRecorderPlayback(ClientGame *game) : GameConnection(game)
{
   mFile = NULL;
   mMilliSeconds = 0;
   mSizeToRead = 0;

   if(!mFile)
      mFile = fopen("record.bin", "rb");

   if(mFile)
   {
      U8 data[4];
      data[0] = 0;
      fread(data, 1, 4, mFile);
      mGhostClassCount = data[1];
      mEventClassCount = U32(data[2]) | (U32(data[3]) << 8);
      if(data[0] != CS_PROTOCOL_VERSION || 
         mEventClassCount > NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeEvent) || 
         mGhostClassCount > NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeObject))
      {
         fclose(mFile); // Wrong version, warn about this problem?
      }

      setGhostFrom(false);
      setGhostTo(true);
      mEventClassBitSize = getNextBinLog2(mEventClassCount);
      mGhostClassBitSize = getNextBinLog2(mGhostClassCount);
   }
   mConnectionState = Connected;
   mConnectionParameters.mIsInitiator = true;
   mConnectionParameters.mDebugObjectSizes = false;
}
GameRecorderPlayback::~GameRecorderPlayback()
{
   if(mFile)
      fclose(mFile);
}

bool GameRecorderPlayback::lostContact() {return false;}
void GameRecorderPlayback::addPendingMove(Move *theMove)
{
   /*BfObject *obj = getControlObject();
   if(obj)
   {
      U32 time = theMove->time;
      *theMove = obj->getCurrentMove();
      theMove->time = time;
   }*/
}

void GameRecorderPlayback::updateTimers(U32 MilliSeconds)
{
   Parent::updateTimers(MilliSeconds);
   if(!mFile)
   {
      disconnect(ReasonShutdown, "");
      return;
   }

   U8 data[16384 - 1];  // 16 KB on stack memory (no memory allocation/deallocation speed cost)

   mMilliSeconds -= S32(MilliSeconds);

   while(mMilliSeconds < 0)
   {
      if(mSizeToRead != 0)
      {
         fread(data, 1, mSizeToRead, mFile);
         BitStream bstream(data, mSizeToRead);
         GhostConnection::readPacket(&bstream);
         mSizeToRead = 0;
      }

      data[1] = data[0] = 0;
      fread(data, 1, 3, mFile);

      U32 size = (U32(data[1] & 63) << 8) + data[0];
      mMilliSeconds += S32((U32(data[1] >> 6) << 8) + data[2]);

      if(size == 0 || size >= sizeof(data))
      {
         fclose(mFile);
         mFile = NULL;
         return;
      }

      mSizeToRead = size;

   }

   if(getControlObject() == NULL) // Maybe we need a better way to choose which ship to spectate
   {
      fillVector.clear();
      mClientGame->getGameObjDatabase()->findObjects(PlayerShipTypeNumber, fillVector);
      if(fillVector.size())
         setControlObject(dynamic_cast<BfObject*>(fillVector[0]));
   }
}


#endif

}