//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _QUICK_CHAT_MESSAGES_H_
#define _QUICK_CHAT_MESSAGES_H_

// Define these here so they're out of the way

#define DEFAULT_QUICK_CHAT_MESSAGE_TABLE                                                                                           \
   QUICK_CHAT_SECTION(1, GlobalMessageType, KEY_G, BUTTON_6, "Global")                                                             \
      QUICK_CHAT_MESSAGE(1, 1, GlobalMessageType, KEY_A, BUTTON_1,    "No Problem",            "No Problemo.")                     \
      QUICK_CHAT_MESSAGE(1, 2, GlobalMessageType, KEY_T, BUTTON_2,    "Thanks",                "Thanks.")                          \
      QUICK_CHAT_MESSAGE(1, 3, GlobalMessageType, KEY_X, KEY_UNKNOWN, "You idiot!",            "You idiot!")                       \
      QUICK_CHAT_MESSAGE(1, 4, GlobalMessageType, KEY_E, BUTTON_3,    "Duh",                   "Duh.")                             \
      QUICK_CHAT_MESSAGE(1, 5, GlobalMessageType, KEY_C, KEY_UNKNOWN, "Crap",                  "Ah Crap!")                         \
      QUICK_CHAT_MESSAGE(1, 6, GlobalMessageType, KEY_D, BUTTON_4,    "Damnit",                "Dammit!")                          \
      QUICK_CHAT_MESSAGE(1, 7, GlobalMessageType, KEY_S, BUTTON_5,    "Shazbot",               "Shazbot!")                         \
      QUICK_CHAT_MESSAGE(1, 8, GlobalMessageType, KEY_Z, BUTTON_6,    "Doh",                   "Doh!")                             \
                                                                                                                                   \
   QUICK_CHAT_SECTION(2, TeamMessageType, KEY_D, BUTTON_5, "Defense")                                                              \
      QUICK_CHAT_MESSAGE(2, 1, TeamMessageType, KEY_G, KEY_UNKNOWN,   "Defend Our Base",       "Defend our base.")                 \
      QUICK_CHAT_MESSAGE(2, 2, TeamMessageType, KEY_D, BUTTON_1,      "Defending Base",        "Defending our base.")              \
      QUICK_CHAT_MESSAGE(2, 3, TeamMessageType, KEY_Q, BUTTON_2,      "Is Base Clear?",        "Is our base clear?")               \
      QUICK_CHAT_MESSAGE(2, 4, TeamMessageType, KEY_C, BUTTON_3,      "Base Clear",            "Base is secured.")                 \
      QUICK_CHAT_MESSAGE(2, 5, TeamMessageType, KEY_T, BUTTON_4,      "Base Taken",            "Base is taken.")                   \
      QUICK_CHAT_MESSAGE(2, 6, TeamMessageType, KEY_N, BUTTON_5,      "Need More Defense",     "We need more defense.")            \
      QUICK_CHAT_MESSAGE(2, 7, TeamMessageType, KEY_E, BUTTON_6,      "Enemy Attacking Base",  "The enemy is attacking our base.") \
      QUICK_CHAT_MESSAGE(2, 8, TeamMessageType, KEY_A, KEY_UNKNOWN,   "Attacked",              "We are being attacked.")           \
                                                                                                                                   \
   QUICK_CHAT_SECTION(3, TeamMessageType, KEY_F, BUTTON_4, "Flag")                                                                 \
      QUICK_CHAT_MESSAGE(3, 1, TeamMessageType, KEY_F, BUTTON_1,      "Get enemy flag",        "Get the enemy flag.")              \
      QUICK_CHAT_MESSAGE(3, 2, TeamMessageType, KEY_R, BUTTON_2,      "Return our flag",       "Return our flag to base.")         \
      QUICK_CHAT_MESSAGE(3, 3, TeamMessageType, KEY_S, BUTTON_3,      "Flag secure",           "Our flag is secure.")              \
      QUICK_CHAT_MESSAGE(3, 4, TeamMessageType, KEY_H, BUTTON_4,      "Have enemy flag",       "I have the enemy flag.")           \
      QUICK_CHAT_MESSAGE(3, 5, TeamMessageType, KEY_E, BUTTON_5,      "Enemy has flag",        "The enemy has our flag!")          \
      QUICK_CHAT_MESSAGE(3, 6, TeamMessageType, KEY_G, BUTTON_6,      "Flag gone",             "Our flag is not in the base!")     \
                                                                                                                                   \
   QUICK_CHAT_SECTION(4, TeamMessageType, KEY_S, KEY_UNKNOWN, "Incoming Enemies - Direction")                                      \
      QUICK_CHAT_MESSAGE(4, 1, TeamMessageType, KEY_S, KEY_UNKNOWN,   "Incoming South",        "*** INCOMING SOUTH ***")           \
      QUICK_CHAT_MESSAGE(4, 2, TeamMessageType, KEY_E, KEY_UNKNOWN,   "Incoming East",         "*** INCOMING EAST  ***")           \
      QUICK_CHAT_MESSAGE(4, 3, TeamMessageType, KEY_W, KEY_UNKNOWN,   "Incoming West",         "*** INCOMING WEST  ***")           \
      QUICK_CHAT_MESSAGE(4, 4, TeamMessageType, KEY_N, KEY_UNKNOWN,   "Incoming North",        "*** INCOMING NORTH ***")           \
      QUICK_CHAT_MESSAGE(4, 5, TeamMessageType, KEY_V, KEY_UNKNOWN,   "Incoming Enemies",      "Incoming enemies!")                \
                                                                                                                                   \
   QUICK_CHAT_SECTION(5, TeamMessageType, KEY_V, BUTTON_3, "Quick")                                                                \
      QUICK_CHAT_MESSAGE(5, 1, TeamMessageType, KEY_J, KEY_UNKNOWN,   "Capture the objective", "Capture the objective.")           \
      QUICK_CHAT_MESSAGE(5, 2, TeamMessageType, KEY_O, KEY_UNKNOWN,   "Go on the offensive",   "Go on the offensive.")             \
      QUICK_CHAT_MESSAGE(5, 3, TeamMessageType, KEY_A, BUTTON_1,      "Attack!",               "Attack!")                          \
      QUICK_CHAT_MESSAGE(5, 4, TeamMessageType, KEY_W, BUTTON_2,      "Wait for signal",       "Wait for my signal to attack.")    \
      QUICK_CHAT_MESSAGE(5, 5, TeamMessageType, KEY_V, BUTTON_3,      "Help!",                 "Help!")                            \
      QUICK_CHAT_MESSAGE(5, 6, TeamMessageType, KEY_E, BUTTON_4,      "Regroup",               "Regroup.")                         \
      QUICK_CHAT_MESSAGE(5, 7, TeamMessageType, KEY_G, BUTTON_5,      "Going offense",         "Going offense.")                   \
      QUICK_CHAT_MESSAGE(5, 8, TeamMessageType, KEY_Z, BUTTON_6,      "Move out",              "Move out.")                        \
                                                                                                                                   \
   QUICK_CHAT_SECTION(6, TeamMessageType, KEY_R, BUTTON_2, "Reponses")                                                             \
      QUICK_CHAT_MESSAGE(6, 1, TeamMessageType, KEY_A, BUTTON_1,      "Acknowledged",          "Acknowledged.")                    \
      QUICK_CHAT_MESSAGE(6, 2, TeamMessageType, KEY_N, BUTTON_2,      "No",                    "No.")                              \
      QUICK_CHAT_MESSAGE(6, 3, TeamMessageType, KEY_Y, BUTTON_3,      "Yes",                   "Yes.")                             \
      QUICK_CHAT_MESSAGE(6, 4, TeamMessageType, KEY_S, BUTTON_4,      "Sorry",                 "Sorry.")                           \
      QUICK_CHAT_MESSAGE(6, 5, TeamMessageType, KEY_T, BUTTON_5,      "Thanks",                "Thanks.")                          \
      QUICK_CHAT_MESSAGE(6, 6, TeamMessageType, KEY_D, BUTTON_6,      "Don't know",            "I don't know.")                    \
                                                                                                                                   \
   QUICK_CHAT_SECTION(7, GlobalMessageType, KEY_T, BUTTON_1, "Taunts")                                                             \
      QUICK_CHAT_MESSAGE(7, 1, GlobalMessageType, KEY_R, KEY_UNKNOWN, "Rawr",                  "RAWR!")                            \
      QUICK_CHAT_MESSAGE(7, 2, GlobalMessageType, KEY_C, BUTTON_1,    "Come get some!",        "Come get some!")                   \
      QUICK_CHAT_MESSAGE(7, 3, GlobalMessageType, KEY_D, BUTTON_2,    "Dance!",                "Dance!");                          \
      QUICK_CHAT_MESSAGE(7, 4, GlobalMessageType, KEY_X, BUTTON_3,    "Missed me!",            "Missed me!")                       \
      QUICK_CHAT_MESSAGE(7, 5, GlobalMessageType, KEY_W, BUTTON_4,    "I've had worse...",     "I've had worse...")                \
      QUICK_CHAT_MESSAGE(7, 6, GlobalMessageType, KEY_Q, BUTTON_5,    "How'd THAT feel?",      "How'd THAT feel?")                 \
      QUICK_CHAT_MESSAGE(7, 7, GlobalMessageType, KEY_E, BUTTON_6,    "Yoohoo!",               "Yoohoo!")                          \

#endif