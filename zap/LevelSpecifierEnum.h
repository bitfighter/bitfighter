//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_SPECIFIER_ENUM_H_
#define _LEVEL_SPECIFIER_ENUM_H_

namespace Zap
{
   enum LevelSpecifier {
      NEXT_LEVEL = -1,
      REPLAY_LEVEL = -2,
      PREVIOUS_LEVEL = -3,
      RANDOM_LEVEL = -4,
      FIRST_LEVEL = 0,
   };
};

#endif 
