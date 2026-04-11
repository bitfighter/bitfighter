//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "BanList.h"

#include "tnlTypes.h"
#include "tnlUDP.h"

#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>

#include "gtest/gtest.h"

using namespace std;
using namespace TNL;

namespace Zap
{

static auto cleanTimeNow() {
   // Truncate to whole seconds first to ensure exact minute calculations
   return chrono::system_clock::from_time_t(chrono::system_clock::to_time_t(chrono::system_clock::now()));
}

// Helper: generate an ISO timestamp that is 'minutesAgo' minutes in the past
static string minutesAgoISO(int minutesAgo)
{
   auto now = cleanTimeNow();
   auto then = now - chrono::minutes(minutesAgo);
   time_t thenT = chrono::system_clock::to_time_t(then);
   char buf[sizeof "11111111T111111"];
   strftime(buf, sizeof buf, "%Y%m%dT%H%M%S", localtime(&thenT));
   return string(buf);
}

// Helper: build a ban list line (wildcard address, wildcard nickname)
// Format: IP|nickname|startDateTime|durationMinutes
static string makeBanLine(const string &startISO, int durationMinutes)
{
   return string("*|*|") + startISO + "|" + to_string(durationMinutes);
}

static Address anyAddress()
{
   return Address(IPProtocol, Address::Any, 0);
}


TEST(BanListTest, ActiveBanBlocksUser)
{
   // Ban started 10 minutes ago, duration 60 minutes -> still active -> user SHOULD be banned
   BanList banList("");
   Vector<string> lines;
   lines.push_back(makeBanLine(minutesAgoISO(10), 60));
   banList.loadBanList(lines);

   EXPECT_TRUE(banList.isBanned(anyAddress(), "anyplayer", false));
}


TEST(BanListTest, ExpiredBanAllowsUser)
{
   // Ban started 90 minutes ago, duration 60 minutes -> expired -> user should NOT be banned
   BanList banList("");
   Vector<string> lines;
   lines.push_back(makeBanLine(minutesAgoISO(90), 60));
   banList.loadBanList(lines);

   EXPECT_FALSE(banList.isBanned(anyAddress(), "anyplayer", false));
}


TEST(BanListTest, BanJustActiveAtBoundary)
{
   // Ban started 59 minutes ago, duration 60 minutes -> still active (1 minute left)
   BanList banList("");
   Vector<string> lines;
   lines.push_back(makeBanLine(minutesAgoISO(59), 60));
   banList.loadBanList(lines);

   EXPECT_TRUE(banList.isBanned(anyAddress(), "anyplayer", false));
}


TEST(BanListTest, BanJustExpiredAtBoundary)
{
   // Ban started 61 minutes ago, duration 60 minutes -> just expired
   BanList banList("");
   Vector<string> lines;
   lines.push_back(makeBanLine(minutesAgoISO(61), 60));
   banList.loadBanList(lines);

   EXPECT_FALSE(banList.isBanned(anyAddress(), "anyplayer", false));
}


TEST(BanListTest, EmptyBanListAllowsUser)
{
   BanList banList("");
   EXPECT_FALSE(banList.isBanned(anyAddress(), "anyplayer", false));
}


TEST(BanListTest, AuthenticatedUserSkipsNonAuthenticatedBan)
{
   // Ban is for non-authenticated users only; authenticated user should NOT be blocked
   BanList banList("");
   Vector<string> lines;
   // Use "*NonAuthenticated" for nickname to ban only unauthenticated users
   string line = string("*|*NonAuthenticated|") + minutesAgoISO(10) + "|60";
   lines.push_back(line);
   banList.loadBanList(lines);

   EXPECT_FALSE(banList.isBanned(anyAddress(), "anyplayer", true));   // authenticated -> NOT banned
   EXPECT_TRUE(banList.isBanned(anyAddress(), "anyplayer", false));   // unauthenticated -> IS banned
}


TEST(BanListTest, MultipleExpiredBansAllowUser)
{
   // Multiple expired bans should all allow the user through
   BanList banList("");
   Vector<string> lines;
   lines.push_back(makeBanLine(minutesAgoISO(120), 60));
   lines.push_back(makeBanLine(minutesAgoISO(180), 60));
   banList.loadBanList(lines);

   EXPECT_FALSE(banList.isBanned(anyAddress(), "anyplayer", false));
}


TEST(BanListTest, ActiveBanAmongExpiredBansBlocksUser)
{
   // One active ban among expired bans should still block the user
   BanList banList("");
   Vector<string> lines;
   lines.push_back(makeBanLine(minutesAgoISO(120), 60));  // expired
   lines.push_back(makeBanLine(minutesAgoISO(10), 60));   // active
   lines.push_back(makeBanLine(minutesAgoISO(180), 60));  // expired
   banList.loadBanList(lines);

   EXPECT_TRUE(banList.isBanned(anyAddress(), "anyplayer", false));
}

} // namespace Zap
