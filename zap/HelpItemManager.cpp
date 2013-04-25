#include "HelpItemManager.h"

#include "BfObject.h"      // For TypeNumbers

using namespace TNL;

namespace Zap 
{

static const S32 MAX_MESSAGES = 8;     // Excluding sentinel item

struct HelpItems {
   U8 associatedItem;
   const char *helpMessages[MAX_MESSAGES + 1];
};

static HelpItems helpItems[] = {
   { RepairItemTypeNumber, { "Repair item heals your ship", NULL } },
   { UnknownTypeNumber,    { "You've selected a new ship configuration", "Find a Loadout Zone to make the changes", NULL } },
   { UnknownTypeNumber,    { "You've selected a new ship configuration", "This level has no Loadout Zones", "So you are basically screwed", NULL } },
};


const char **HelpItemManager::getHelpMessages(HelpItem item)
{
   return helpItems[item].helpMessages;
}


U8 HelpItemManager::getAssociatedItem(HelpItem item)
{
   return helpItems[item].associatedItem;
}


}