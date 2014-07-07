//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "TestUtils.h"

#include "../master/master.h"
#include "../master/MasterServerConnection.h"
#include "masterConnection.h"
#include "ClientGame.h"
#include "UIManager.h"

namespace Zap
{

using namespace Master;
TEST(MasterTest, Sanity)
{
   // First off, can we get a master server going here?
   MasterSettings masterSettings("");     // Don't read from an INI file
   MasterServer master(&masterSettings);

   Address addr;

   ClientGame *clientGame = newClientGame();

//   clientGame->setConnectionToMaster(new Zap::MasterServerConnection(clientGame)); // uses Zap::MasterServerConnection
//   clientGame->getConnectionToMaster()->connectLocal((NetInterface*) clientGame->getNetInterface(), master.getNetInterface());
//   EXPECT_TRUE(clientGame->isConnectedToMaster());
//
//   Zap::MasterServerConnection *clientConnection = clientGame->getConnectionToMaster();
//   EXPECT_TRUE(clientConnection != NULL);
//   Master::MasterServerConnection *masterConnection = dynamic_cast<Master::MasterServerConnection *>(clientConnection->getRemoteConnectionObject());
//   EXPECT_TRUE(masterConnection != NULL);

   delete clientGame;
}
	
};
