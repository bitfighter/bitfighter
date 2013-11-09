//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelDatabase.h"

namespace Zap
{

namespace LevelDatabase
{

bool isLevelInDatabase(U32 databaseId)
{
   return databaseId != NOT_IN_DATABASE;
}

}

} /* namespace Zap */
