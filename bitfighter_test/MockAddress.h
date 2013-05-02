#ifndef MOCKADDRESS_H_
#define MOCKADDRESS_H_

#include "tnlUDP.h"

using namespace TNL;

/**
 * A mock address which is always valid
 */
class MockAddress : public Address {
   public:
   bool isValid() { return true; }
};

#endif /* MOCKADDRESS_H_ */
