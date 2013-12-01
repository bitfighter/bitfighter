//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _VERSION_H_
#define _VERSION_H_

#define ZAP_GAME_NAME "Bitfighter"

// Updated from 6 to 7 for 019
#define MASTER_PROTOCOL_VERSION 7  // Change this when releasing an incompatible cm/sm protocol (must be int)
                                   // MASTER_PROTOCOL_VERSION = 4, client 015a and older (CS_PROTOCOL_VERSION <= 32) can not connect to our new master.
#define CS_PROTOCOL_VERSION 38     // Change this when releasing an incompatible cs protocol (must be int)
// 016 = 33 
// 017[ab] = 35
// 018[a] = 36
// 019 dev = 37
// 019 = 38

#define VERSION_016  3737
#define VERSION_017  4252
#define VERSION_017a 4265
#define VERSION_017b 4537
#define VERSION_018  6059
#define VERSION_018a 6800
#define VERSION_019  8814

#define BUILD_VERSION VERSION_019 // Version of the game according to hg, will be unique every release (must be int)
                                   // Get from "hg summary"

#define ZAP_GAME_RELEASE "019"    // Change this with every release -- for display purposes only, string,
                                   // will also be used for name of installer on windows, so be careful with spaces  

#endif

