//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ANCHOR_POINT_H_
#define _ANCHOR_POINT_H_

#include "Point.h"

namespace Zap
{

using namespace std;


enum AnchorType {
   ScreenAnchor,  
   MapAnchor
};

// This class is the template for most all of our menus...
struct AnchorPoint
{
   Point pos;
   AnchorType anchorType;
   AnchorPoint(const Point &p, AnchorType t) : pos(p) { anchorType = t; }     // Sorry, can't bear to make a .cpp for just this!!!
   AnchorPoint() { anchorType = ScreenAnchor; }     

};



}


#endif   

