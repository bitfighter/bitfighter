// To use this RobotController, add Robot without any parameters in level file.

#include "point.h"
#include "tnlVector.h"

namespace Zap{

class Ship;


class RobotController
{
public:
	Vector<Point> mFlightPlan;
	U16 mFlightPlanTo;

	Point mPrevTarget;

   RobotController();
   void run(Ship *ship);
};

}