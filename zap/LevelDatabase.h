//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef LEVELDATABASE_H_
#define LEVELDATABASE_H_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

namespace LevelDatabase
{
   static const U32 NOT_IN_DATABASE = 0;

   bool isLevelInDatabase(U32 databaseId);
};

} /* namespace Zap */

#endif /* LEVELDATABASE_H_ */
