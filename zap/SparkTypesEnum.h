//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SPARK_TYPE_ENUM_H_
#define _SPARK_TYPE_ENUM_H_

namespace Zap { namespace UI {

   // Different types of sparks
   enum SparkType
   {
      SparkTypePoint,
      SparkTypeLine,
      SparkTypeCount
   };

   enum TrailProfile {
      ShipProfile,
      CloakedShipProfile,
      TurboShipProfile,
      SeekerProfile
   };

}  } // Nested namespace

#endif

