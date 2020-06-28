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

#include "discord_rpc.h"

class DiscordIntegrator : public AppIntegrator
{
private:
   static string discordClientId;

   S64 mStartTime;
   DiscordRichPresence mDiscordPresence;

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

   void updateState(const string &state);
   void updateDetails(const string &details);
};
#endif

} /* namespace Zap */

#endif /* ZAP_APPINTEGRATOR_H_ */
