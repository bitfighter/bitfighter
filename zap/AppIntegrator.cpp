//------------------------------------------------------------------------------
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "AppIntegrator.h"

#include "Timer.h"
#include "Intervals.h"

#include "tnlLog.h"

#include <cstdio>
#include <cstring>

#ifdef BF_DISCORD
#  include "GameManager.h"
#  include "ClientGame.h"
#  include "UIManager.h"
#  include "UIGame.h"
#  include "UIEditor.h"
#endif


namespace Zap
{

using namespace std;


////////////////////////////////////////
////////////////////////////////////////

AppIntegrator::AppIntegrator()
{
   // Do nothing
}


AppIntegrator::~AppIntegrator()
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

Vector<AppIntegrator*> AppIntegrationController::mAppIntegrators = Vector<AppIntegrator*>();
Timer AppIntegrationController::mCallbackTimer = Timer();

AppIntegrationController::AppIntegrationController()
{
   // Do nothing
}


AppIntegrationController::~AppIntegrationController()
{
   // Do nothing
}


void AppIntegrationController::init()
{
   // Instantiate and add any integrators here
#ifdef BF_DISCORD
   mAppIntegrators.push_back(new DiscordIntegrator());
#endif

   // Run init() on all the integrators
   for(int i = 0; i < mAppIntegrators.size(); i++)
      mAppIntegrators[i]->init();

   // Start callback timer
   mCallbackTimer.reset(TWO_SECONDS);
}


void AppIntegrationController::idle(U32 deltaTime)
{
   if(mCallbackTimer.update(deltaTime))
   {
      // Check for 3rd party app callbacks
      runCallbacks();

      // Restart timer
      mCallbackTimer.reset();
   }
}


void AppIntegrationController::shutdown()
{
   // Run shutdown() on all the integrators
   for(int i = 0; i < mAppIntegrators.size(); i++)
      mAppIntegrators[i]->shutdown();

   // Clean up
   for(int i = 0; i < mAppIntegrators.size(); i++)
   {
      AppIntegrator *app = mAppIntegrators[i];

      if(app != NULL)
         delete app;
   }
}


void AppIntegrationController::runCallbacks()
{
   for(int i = 0; i < mAppIntegrators.size(); i++)
      mAppIntegrators[i]->runCallbacks();
}


void AppIntegrationController::updateState(const string &state, const string &details)
{

   for(int i = 0; i < mAppIntegrators.size(); i++)
   {
      if(state != "")
         mAppIntegrators[i]->updateState(state);

      if(details != "")
         mAppIntegrators[i]->updateDetails(details);
   }
}


#ifdef BF_DISCORD

////////////////////////////////////////
////////////////////////////////////////
// Discord

// Statics
// This is Bitfighter's discord ID
string DiscordIntegrator::discordClientId = "620788608743243776";


DiscordIntegrator::DiscordIntegrator()
{
   // Do nothing
   mStartTime = 0;
   mDiscordPresence = DiscordRichPresence();
}

DiscordIntegrator::~DiscordIntegrator()
{
   // Do nothing
}

void DiscordIntegrator::init()
{
   mStartTime = time(0);
   // Guarantee memory
   memset(&mDiscordPresence, 0, sizeof(mDiscordPresence));

   // Set function handlers for various discord actions
   DiscordEventHandlers handlers;
   memset(&handlers, 0, sizeof(handlers));

   handlers.ready = handleDiscordReady;
   handlers.disconnected = handleDiscordDisconnected;
   handlers.errored = handleDiscordError;
   handlers.joinGame = handleDiscordJoin;
   handlers.spectateGame = handleDiscordSpectate;
   handlers.joinRequest = handleDiscordJoinRequest;

   Discord_Initialize(discordClientId.c_str(), &handlers, 1, NULL);


   // Now set our presence to be in game!
   mDiscordPresence.state = "";
   mDiscordPresence.details = "";

   mDiscordPresence.startTimestamp = mStartTime;
//   mDiscordPresence.endTimestamp = time(0) + 5 * 60;
   mDiscordPresence.largeImageKey = "ship_blue";
   mDiscordPresence.largeImageText = "BITFIGHTER";
//   mDiscordPresence.smallImageKey = "ship_blue";
//   mDiscordPresence.smallImageText = "ptb-small";

   // There are other memebers you can set, see DiscordRichPresence struct in
   // discord_rpc.h

   // Always call this if changing anything about the rich-presence
   Discord_UpdatePresence(&mDiscordPresence);
}

void DiscordIntegrator::shutdown()
{
   Discord_Shutdown();
}

void DiscordIntegrator::runCallbacks()
{
   updateBitfighterState();

   // Actually run the callbacks
   Discord_RunCallbacks();
}


void DiscordIntegrator::updateBitfighterState()
{
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();

   // No client game?
   if(clientGames->size() == 0)
      return;

   UIManager *uiManager = clientGames->get(0)->getUIManager();

   // Let's update the rich presence based on what UI we're in
   static UserInterface *currentImportantUI = NULL;
   static UserInterface *newImportantUI = NULL;

   string state = "";
   if(uiManager->isCurrentUI<EditorUserInterface>() || uiManager->cameFrom<EditorUserInterface>())
   {
      state = "Editing a map";
      newImportantUI = uiManager->getUI<EditorUserInterface>();
   }
   else if(uiManager->isCurrentUI<GameUserInterface>() || uiManager->cameFrom<GameUserInterface>())
   {
      state = "In battle";
      newImportantUI = uiManager->getUI<GameUserInterface>();
   }
   else
   {
      state = "In menus";
      newImportantUI = uiManager->getUI<MainMenuUserInterface>();
   }

   // Update state only on UI change
   if(newImportantUI != currentImportantUI)
   {
      currentImportantUI = newImportantUI;

      updateState(state);
   }
}


void DiscordIntegrator::updateState(const string &state)
{
   mDiscordPresence.state = state.c_str();
   Discord_UpdatePresence(&mDiscordPresence);
}

void DiscordIntegrator::updateDetails(const string &details)
{
   mDiscordPresence.details = details.c_str();
   Discord_UpdatePresence(&mDiscordPresence);
}


// Discord Callbacks
void DiscordIntegrator::handleDiscordReady(const DiscordUser *connectedUser)
{
   logprintf("Discord: connected to user %s#%s - %s", connectedUser->username,
         connectedUser->discriminator, connectedUser->userId);
}

void DiscordIntegrator::handleDiscordDisconnected(int errcode, const char *message)
{
//   logprintf("Discord: disconnected (%d: %s)", errcode, message);
}

void DiscordIntegrator::handleDiscordError(int errcode, const char *message)
{
//   logprintf("Discord: error (%d: %s)", errcode, message);
}

void DiscordIntegrator::handleDiscordJoin(const char *secret)
{
//   logprintf("Discord: join (%s)", secret);
}

void DiscordIntegrator::handleDiscordSpectate(const char *secret)
{
//   logprintf("Discord: spectate (%s)", secret);
}

void DiscordIntegrator::handleDiscordJoinRequest(const DiscordUser *request)
{
//   logprintf("Discord: join request from %s#%s - %s", request->username,
//         request->discriminator, request->userId);
}

#endif  // BF_DISCORD

} /* namespace Zap */
