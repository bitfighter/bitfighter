//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
   
#include "LevelSource.h"
#include "LevelFilesForTesting.h"

#include "ClientGame.h"
#include "ServerGame.h"

#include "stringUtils.h"
#include "TestUtils.h"

#include "tnlNetInterface.h"
#include "gtest/gtest.h"

namespace Zap
{


TEST(TestLevelSource, tests)
{
   Address addr;
   NetInterface net(addr);   // We never use this, but it will initialize TNL to get past an assert


   // These tests aren't really intensive... just basic functionality.  Could be expanded to include more 
   // challenging levels

   pair<Vector<string>, Vector<LevelInfo> > levels = getLevels();

   for(S32 i = 0; i < levels.first.size(); i++)
   {
      SCOPED_TRACE("i = " + itos(i));
      LevelInfo levelInfo;
      LevelSource::getLevelInfoFromCodeChunk(levels.first[i], levelInfo);

      EXPECT_EQ(levelInfo.mLevelName,      levels.second[i].mLevelName);
      EXPECT_EQ(levelInfo.mLevelType,      levels.second[i].mLevelType);
      EXPECT_EQ(levelInfo.minRecPlayers,   levels.second[i].minRecPlayers);
      EXPECT_EQ(levelInfo.maxRecPlayers,   levels.second[i].maxRecPlayers);
      EXPECT_EQ(levelInfo.mScriptFileName, levels.second[i].mScriptFileName);
   }
}


static Vector<string> findAllPlaylists_client(const string &dir)
{
   Vector<string> playlist({ "ClientOne", "ClientTwo", "ClientThree" });
   return playlist;
}


static Vector<string> findAllPlaylists_server(const string &dir)
{
   Vector<string> playlist({ "ServerOne", "ServerTwo", "ServerThree", "ServerFour" });
   return playlist;
}


TEST(TestLevelSource, PlaylistTests)
{
   // Client starts up, should have access to local list of levels
   Vector<string> clientPlaylist = findAllPlaylists_client("");
   Vector<string> serverPlaylist = findAllPlaylists_server("");

   GameSettingsPtr clientSettings = GameSettingsPtr(new GameSettings());
   clientSettings->getFolderManager()->mFindAllPlaylistsFunction = &findAllPlaylists_client;

   GameSettingsPtr serverSettings = GameSettingsPtr(new GameSettings());
   serverSettings->getFolderManager()->mFindAllPlaylistsFunction = &findAllPlaylists_server;

   LevelSourcePtr serverLevelSource(new TestPlaylistLevelSource(serverPlaylist, serverSettings->get()));

   GamePair gamePair(serverSettings, clientSettings, serverLevelSource);

   ClientGame *clientGame = gamePair.getClient(0);
   ServerGame *serverGame = gamePair.server;

   // Ensure we're working with the playlists we think we are
   EXPECT_EQ(serverPlaylist.size(), serverGame->getLevelCount());
   EXPECT_EQ(serverPlaylist[0],     serverGame->getCurrentLevelFileName());

   EXPECT_EQ(clientPlaylist.size(), clientGame->getLocalPlaylists().size());
   EXPECT_EQ(clientPlaylist[0],     clientGame->getLocalPlaylists()[0]);

   gamePair.idle(5,10);    // Initial idle... skipped with the playlist constructor.  This connects client to server.

   // After we connect to the server, we expect to have received a list of playlists from the server,
   // which we should see here:
   EXPECT_EQ(serverPlaylist.size(), clientGame->getServerPlaylists().size());
   EXPECT_EQ(serverPlaylist[0],     clientGame->getServerPlaylists()[0]);

   // But out local playlist list is unchanged
   EXPECT_EQ(clientPlaylist.size(), clientGame->getLocalPlaylists().size());
   EXPECT_EQ(clientPlaylist[0],     clientGame->getLocalPlaylists()[0]);

   // Now we will disconnect, and when we get a list of available playlists, we expect to see those 
   // available on the client, which is what we'd need to see if we were to start hosting a game locally.
   GamePair::disconnectClient(0);

   gamePair.idle(5,10);    // Initial idle... skipped with the playlist constructor

   EXPECT_EQ(clientPlaylist.size(), clientGame->getLocalPlaylists().size());
   EXPECT_EQ(clientPlaylist[0],     clientGame->getLocalPlaylists()[0]);

   // No longer connected, don't care about client's list of playlists on the server, won't test here
}

};

