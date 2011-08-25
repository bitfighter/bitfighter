/*
 * Colors.h
 *
 *  Created on: Jun 7, 2011
 *      Author: dbuck
 */

#ifndef COLORS_H_
#define COLORS_H_

#include "Color.h"

namespace Zap {

namespace Colors {
//public:
   const Color red(1,0,0);
   const Color green(0,1,0);
   const Color blue(0,0,1);
   const Color yellow(1,1,0);
   const Color cyan(0,1,1);
   const Color magenta(1,0,1);
   const Color black(0,0,0);
   const Color white(1,1,1);
   const Color gray50(0.50);
   const Color orange50(1, .50f, 0);       // Rabbit orange
   const Color orange67(1, .67f ,0);      // A more reddish orange
};

}

#endif /* COLORS_H_ */
