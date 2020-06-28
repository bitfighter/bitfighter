//------------------------------------------------------------------------------
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef ZAP_APPINTEGRATOR_H_
#define ZAP_APPINTEGRATOR_H_

#include "tnlTypes.h"
#include "tnlVector.h"

#include <string>

using namespace TNL;

namespace Zap
{

using namespace std;

class Timer;

// Abstract class for any application integrator
struct AppIntegrator
{
   AppIntegrator();
   virtual ~AppIntegrator();

   // Pure virtual functions only
   virtual void init() = 0;
   virtual void shutdown() = 0;
   virtual void runCallbacks() = 0;

   virtual void updateState(const string &state) = 0;
   virtual void updateDetails(const string &details) = 0;
};


// Class called from Bitfighter code to run all the IntegratedApp stuff
class AppIntegrationController
{
private:
   static Vector<AppIntegrator*> mAppIntegrators;
   static Timer mCallbackTimer;

public:
	AppIntegrationController();
	virtual ~AppIntegrationController();

	static void init();
   static void shutdown();

   static void idle(U32 deltaTime);
   static void runCallbacks();

   // This method should be generic enough to update as much as possible in any
   // of the applications; consider moving to a struct of some sort
   static void updateState(const string &state, const string &details);
};


#ifdef BF_DISCORD

#include "../discord-rpc/include/discord_rpc.h"

// This struct mirrors the
struct PersistentRichPresence
{
   string state;   /* max 128 bytes */
   string details; /* max 128 bytes */
   S64 startTimestamp;
   S64 endTimestamp;
   string largeImageKey;  /* max 32 bytes */
   string largeImageText; /* max 128 bytes */
   string smallImageKey;  /* max 32 bytes */
   string smallImageText; /* max 128 bytes */
   string partyId;        /* max 128 bytes */
   S32 partySize;
   S32 partyMax;
   string matchSecret;    /* max 128 bytes */
   string joinSecret;     /* max 128 bytes */
   string spectateSecret; /* max 128 bytes */
   S8 instance;
};



class DiscordIntegrator : public AppIntegrator
{
private:
   static string discordClientId;

   S64 mStartTime;
   PersistentRichPresence mPersistentPresence;
   DiscordRichPresence   mRichPresence;

   static void handleDiscordReady(const DiscordUser *connectedUser);
   static void handleDiscordDisconnected(int errcode, const char *message);
   static void handleDiscordError(int errcode, const char *message);
   static void handleDiscordJoin(const char *secret);
   static void handleDiscordSpectate(const char *secret);
   static void handleDiscordJoinRequest(const DiscordUser *request);

public:
   DiscordIntegrator();
   virtual ~DiscordIntegrator();

   void init();
   void shutdown();
   void runCallbacks();

   void updatePresence();

   void updateState(const string &state);
   void updateDetails(const string &details);

   void updateBitfighterState();

};
#endif

} /* namespace Zap */

#endif /* ZAP_APPINTEGRATOR_H_ */
