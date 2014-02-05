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

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIManager.h"
#endif

#include "version.h"

#include <algorithm>

namespace Zap
{

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
   mFile = NULL;
   mGame = game;
   mMilliSeconds = 0;
   mWriteMaxBitSize = U32_MAX;
   mPackUnpackShipEnergyMeter = true;

   if(!mFile)
	{
		const string &dir = game->getSettings()->getFolderManager()->recordDir;
      mFileName = newRecordingFileName(dir, game->getGameType()->getLevelName(), game->getSettings()->getHostName()) +
            "." + buildGameRecorderExtension();
		string filename = joindir(dir, mFileName);
		mFile = fopen(filename.c_str(), "wb");
	}

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
      data[3] = U8(mEventClassCount >> 8) | 0x10;
      fwrite(data, 1, 4, mFile);
      gameRecorderScoping(this, game);

      s2cSetServerName(game->getSettings()->getHostName());
   }
}

// Destructor
GameRecorderServer::~GameRecorderServer()
{
   if(mFile)
      fclose(mFile);
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
   if(!mFile)
      return;

   if(!GhostConnection::isDataToTransmit() && mMilliSeconds + MilliSeconds < (1 << 10) - 200)  // we record milliseconds as 10 bits
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


}
