#include "GameRecorderPlayback.h"
#include "tnlBitStream.h"
#include "tnlNetObject.h"
#include "gameType.h"
#include "ServerGame.h"
#include "stringUtils.h"

#include "ClientGame.h"
#include "UIManager.h"
#include "UIGame.h"

#include "version.h"

namespace Zap
{


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

bool GameRecorderPlayback::lostContact() {return false;}
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
      //disconnect(ReasonShutdown, "");
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
}

void PlaybackSelectUserInterface::onActivate()
{
//mLevels
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

PlaybackGameUserInterface::PlaybackGameUserInterface(ClientGame *game) : UserInterface(game)
{
	mGameInterface = game->getUIManager()->getUI<GameUserInterface>();
}



void PlaybackGameUserInterface::onActivate() {}


bool PlaybackGameUserInterface::onKeyDown(InputCode inputCode) {return false;}
void PlaybackGameUserInterface::onKeyUp(InputCode inputCode) {}
void PlaybackGameUserInterface::onTextInput(char ascii) {}

void PlaybackGameUserInterface::idle(U32 timeDelta) {}
void PlaybackGameUserInterface::render() {}



}