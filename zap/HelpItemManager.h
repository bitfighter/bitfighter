
#ifndef _HELP_ITEM_MANAGER_H_
#define _HELP_ITEM_MANAGER_H_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class HelpItemManager
{

public:

enum HelpItem {
   RepairItemSpottedHelpItem,
   LoadoutChangedLoadoutZoneHelpItem,
   LoadoutChangedNoLoadoutZoneHelpItem,
};

const char **getHelpMessages(HelpItem item);
U8 getAssociatedItem(HelpItem item);

};


}


#endif