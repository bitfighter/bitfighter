//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#ifndef _EDITOROBJECT_H_
#define _EDITOROBJECT_H_

#include "BfObject.h"          // We inherit from this -- for BfObject, for now

using namespace std;
using namespace TNL;

namespace Zap
{

// Class with editor methods related to point things

class PointObject : public BfObject
{
   typedef BfObject Parent;

private:
   F32 mRadius;

public:
   explicit PointObject(F32 radius = 1);   // Constructor
   virtual ~PointObject();                 // Destructor

   void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
   F32 getRadius();
};


};
#endif
