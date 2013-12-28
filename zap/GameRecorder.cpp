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


static FILE *openRecordingFile(ServerGame *game)
{
   const string &dir = game->getSettings()->getFolderManager()->recordDir;

   makeSureFolderExists(dir);

   Vector<string> files;
   getFilesFromFolder(dir, files);

   S32 max_id = 0;
   for(S32 i = 0; i < files.size(); i++)
   {
      S32 id = stoi(files[i]);
      if(max_id < id)
         max_id = id;
   }

   string file = joindir(dir, itos(max_id + 1));

   string file2 = makeFilenameFromString(game->getGameType()->getLevelName()->getString());
   if(file2.size() != 0)
      file = file + "_" + file2;
   return fopen(file.c_str(), "wb");
}


GameRecorderServer::GameRecorderServer(ServerGame *game)
{
   mFile = NULL;
   mGame = game;
   mMilliSeconds = 0;
   mWriteMaxBitSize = U32_MAX;
   mPackUnpackShipEnergyMeter = true;

   if(!mFile)
      mFile = openRecordingFile(game);

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


GameRecorderServer::~GameRecorderServer()
{
   if(mFile)
      fclose(mFile);
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


#ifndef ZAP_DEDICATED


GameRecorderPlayback::GameRecorderPlayback(ClientGame *game, const char *filename) : GameConnection(game)
{
   mFile = NULL;
   mGame = game;
   mMilliSeconds = 0;
   mSizeToRead = 0;

   if(!mFile)
      mFile = fopen(filename, "rb");

   if(mFile)
   {
      U8 data[4];
      data[0] = 0;
      fread(data, 1, 4, mFile);
      mGhostClassCount = data[1];
      mEventClassCount = U32(data[2]) | (U32(data[3]) << 8);
      if(mEventClassCount & 0x1000)
      {
         mPackUnpackShipEnergyMeter = true;
         mEventClassCount &= ~0x1000;
      }
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


bool GameRecorderPlayback::lostContact() { return false; }


void GameRecorderPlayback::addPendingMove(Move *theMove)
{
   bool nextButton = theMove->fire;
   bool prevButton = theMove->modulePrimary[0] || theMove->modulePrimary[1];

   if(!isButtonHeldDown && (nextButton || prevButton))
   {
      S32 n = nextButton ? 1 : -1;

      const Vector<RefPtr<ClientInfo> > &infos = *(mGame->getClientInfos());
      for(S32 i = 0; i < infos.size(); i++)
         if(infos[i].getPointer() == mClientInfoSpectating.getPointer())
         {
            n += i;
            break;
         }

      if(n < 0)
         n = infos.size() - 1;
      else if(n >= infos.size())
         n = 0;

      if(infos.size() != 0)
         mClientInfoSpectating = infos[n];
   }

   isButtonHeldDown = nextButton || prevButton;
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

   const Vector<RefPtr<ClientInfo> > &infos = *(mGame->getClientInfos());
   if(mClientInfoSpectating.isNull() && infos.size() != 0)
   {
      mClientInfoSpectating = infos[0];
   }
   
   if(mClientInfoSpectating.isValid())
   {
      Ship *ship = mClientInfoSpectating->getShip();
      setControlObject(ship);
      if(ship)
         mGame->newLoadoutHasArrived(*(ship->getLoadout()));
   }
}


static void processPlaybackSelectionCallback(ClientGame *game, U32 index)             
{
   game->getUIManager()->getUI<PlaybackSelectUserInterface>()->processSelection(index);

}


PlaybackSelectUserInterface::PlaybackSelectUserInterface(ClientGame *game) : LevelMenuSelectUserInterface(game)
{
   // Do nothing
}


void PlaybackSelectUserInterface::onActivate()
{
   mMenuTitle = "Choose Recorded Game";

   const string &dir = getGame()->getSettings()->getFolderManager()->recordDir;

   clearMenuItems();
   mLevels.clear();
   getFilesFromFolder(dir, mLevels);
   for(S32 i = 0; i < mLevels.size(); i++)
   {
      addMenuItem(new MenuItem(i, mLevels[i].c_str(), processPlaybackSelectionCallback, ""));
   }

   if(mLevels.size() == 0)
      mMenuTitle = "No recorded games exists";  // TODO: Need better way to display this problem

   MenuUserInterface::onActivate();
}


void PlaybackSelectUserInterface::processSelection(U32 index)
{
   string file = joindir(getGame()->getSettings()->getFolderManager()->recordDir, mLevels[index]);
   getGame()->getUIManager()->activateGameUserInterface();
   getGame()->setConnectionToServer(new GameRecorderPlayback(getGame(), file.c_str()));
}



#endif

}