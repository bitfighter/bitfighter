//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "engineerHelper.h"

#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "UI.h"                  // For margin info -- can we just get rid of this as antiquated?
#include "Color.h"               // For Color def
#include "Colors.h"
#include "ClientGame.h"
#include "JoystickRender.h"
#include "config.h"
#include "gameObjectRender.h"    // For drawSquare

#include "RenderUtils.h"
#include "OpenglUtils.h"

namespace Zap
{

   // TODO: xmacroize to keep in sync

static OverlayMenuItem engineerItemInfo[] = {
   { KEY_1, BUTTON_1, true,  EngineeredTurret,             "Turret",          &Colors::overlayMenuUnselectedItemColor, "", NULL },
   { KEY_2, BUTTON_2, true,  EngineeredForceField,         "Force Field",     &Colors::overlayMenuUnselectedItemColor, "", NULL },
   { KEY_3, BUTTON_3, true,  EngineeredTeleporterEntrance, "Teleporter",      &Colors::overlayMenuUnselectedItemColor, "", NULL },
   { KEY_4, BUTTON_4, false, EngineeredTeleporterExit,     "Teleporter Exit", &Colors::overlayMenuUnselectedItemColor, "", NULL },
};                                                         
                                                           
                                    
static const char *engineerInstructions[] = {
   "Aim at a spot on a wall, and activate the module again.",
   "Aim at a spot on a wall, and activate the module again.",
   "Aim at a spot in open space, and activate the module again.",
   "Aim at a spot in open space, and activate the module again."
};


////////////////////////////////////////
////////////////////////////////////////


static const char *menuTitle = "Choose One:";

// Constructor
EngineerHelper::EngineerHelper() :
   mEngineerItemsDisplayWidth( getWidthOfItems() )
{
   mSelectedIndex = -1;
}


// Destructor
EngineerHelper::~EngineerHelper()
{
   // Do nothing
}


S32 EngineerHelper::getWidthOfItems() const
{
   // With this one, the title is wider than the text (at the moment at least), so we should consider the title width.  This
   // is a bit tricky, however, since the menu items are normally indented, and that indention is added to the menu width
   // we pass.  Therefore, to make everything look nice, we need to subtract that bit off here so we don't end up with a 
   // much wider menu than necessary.  Add the horizMargin to make things look balanced.
   S32 maxItemWidth = getMaxItemWidth(engineerItemInfo, ARRAYSIZE(engineerItemInfo));
   S32 titleWidth = getStringWidth(MENU_FONT_SIZE, menuTitle) - (ITEM_INDENT + 2 * ITEM_HELP_PADDING) +
                                         UserInterface::horizMargin;

   return max(maxItemWidth, titleWidth);
}


HelperMenu::HelperMenuType EngineerHelper::getType() { return EngineerHelperType; }


void EngineerHelper::setSelectedEngineeredObject(U32 objectType)
{
   for(U32 i = 0; i < ARRAYSIZE(engineerItemInfo); i++)
      if(engineerItemInfo[i].itemIndex == objectType)
         mSelectedIndex = i;
}


void EngineerHelper::onActivated()
{
   // Need to do this here because user may have toggled joystick and keyboard modes
   mEngineerButtonsWidth = getButtonWidth(engineerItemInfo, ARRAYSIZE(engineerItemInfo));

   // Before we activate the helper, we need to tell it what its width will be
   setExpectedWidth(getTotalDisplayWidth(mEngineerButtonsWidth, mEngineerItemsDisplayWidth));
   Parent::onActivated();

   mSelectedIndex = -1;
}


void EngineerHelper::render()
{
   S32 yPos = MENU_TOP + MENU_PADDING;
   
   if(isMenuBeingDisplayed())    // Haven't selected an item yet, so show the menu
      drawItemMenu(menuTitle, engineerItemInfo, ARRAYSIZE(engineerItemInfo), NULL, 0, mEngineerItemsDisplayWidth, mEngineerButtonsWidth);

   else     // Have selected a module, need to indicate where to deploy
   {
      S32 xPos = UserInterface::horizMargin;

      glColor(Colors::green);
      drawStringf(xPos, yPos, MENU_FONT_SIZE, "Placing %s.", engineerItemInfo[mSelectedIndex].name);
      yPos += MENU_FONT_SIZE + MENU_FONT_SPACING;
      drawString(xPos, yPos, MENU_FONT_SIZE, engineerInstructions[mSelectedIndex]);
   }
}


bool EngineerHelper::isMenuBeingDisplayed() const
{
   return mSelectedIndex == -1;
}


// Return true if key did something, false if key had no effect
// Runs on client
bool EngineerHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))    // Check for cancel keys
      return true;

   InputCodeManager *inputCodeManager = getGame()->getSettings()->getInputCodeManager();

   if(isMenuBeingDisplayed())    // Menu is being displayed, so interpret keystrokes as menu items
   {
      for(U32 i = 0; i < ARRAYSIZE(engineerItemInfo); i++)
      {
         // Disallow selecting unselectable items
         if(!engineerItemInfo[i].showOnMenu)
            continue;

         if(inputCode == engineerItemInfo[i].key || inputCode == engineerItemInfo[i].button)
         {
            mSelectedIndex = i;
            return true;
         }
      }

      Ship *ship = getGame()->getLocalPlayerShip();

      if(!ship || (inputCode == inputCodeManager->getBinding(BINDING_MOD1) && ship->getModule(0) == ModuleEngineer) ||
                  (inputCode == inputCodeManager->getBinding(BINDING_MOD2) && ship->getModule(1) == ModuleEngineer))
      {
         exitHelper();
         return true;
      }
   }
   else                       // Placing item
   {
      Ship *ship = getGame()->getLocalPlayerShip();

      if(ship && ((inputCode == inputCodeManager->getBinding(BINDING_MOD1) && ship->getModule(0) == ModuleEngineer) ||
                  (inputCode == inputCodeManager->getBinding(BINDING_MOD2) && ship->getModule(1) == ModuleEngineer)))
      {
         EngineerModuleDeployer deployer;

         // Check deployment status on client; will be checked again on server,
         // but server will only handle likely valid placements
         if(deployer.canCreateObjectAtLocation(getGame()->getGameObjDatabase(), ship, engineerItemInfo[mSelectedIndex].itemIndex))
         {
            // Send command to server to deploy
            getGame()->getConnectionToServer()->c2sEngineerDeployObject(engineerItemInfo[mSelectedIndex].itemIndex);
         }
         // If location is bad, show error message
         else
            getGame()->displayErrorMessage(deployer.getErrorMessage().c_str());

         // Normally we'd exit the helper menu here, but we don't since teleport has two parts.
         // We therefore let a server command dictate what we do (see GameConnection::s2cEngineerResponseEvent)

         return true;
      }
   }

   return false;
}


void EngineerHelper::exitHelper()
{
   if(mSelectedIndex != -1 && engineerItemInfo[mSelectedIndex].itemIndex == EngineeredTeleporterExit)
   {
      GameConnection *gameConnection = getGame()->getConnectionToServer();

      if(gameConnection)
         gameConnection->c2sEngineerInterrupted(EngineeredTeleporterExit);
   }

   Parent::exitHelper();
}


S32 EngineerHelper::getAnimationTime() const
{
   if(isMenuBeingDisplayed())
      return Parent::getAnimationTime();
   
   return 0;      // Returning 0 will cause menu to disappear immediately
}


// Basically draws a red box where the ship is pointing
void EngineerHelper::renderDeploymentMarker(const Ship *ship)
{
   static Point deployPosition, deployNormal;      // Reusable containers

   U32 item = engineerItemInfo[mSelectedIndex].itemIndex;

   // Only render wall mounted items (not teleport)
   if(mSelectedIndex != -1 &&
         EngineerModuleDeployer::findDeployPoint(ship, item, deployPosition, deployNormal))
   {
      EngineerModuleDeployer deployer;
      bool canDeploy = deployer.canCreateObjectAtLocation(getGame()->getGameObjDatabase(), ship, item);

      switch(engineerItemInfo[mSelectedIndex].itemIndex)
      {
         case EngineeredTurret:
         case EngineeredForceField:
            glColor(canDeploy ? Colors::green : Colors::red);
            drawSquare(deployPosition, 5);
            break;

         case EngineeredTeleporterEntrance:
         case EngineeredTeleporterExit:
            renderTeleporterOutline(deployPosition, 75.f, canDeploy ? Colors::green : Colors::red);
            break;

         default:
            TNLAssert(false, "Unexpected value!");
      }
   }
}


const char *EngineerHelper::getCancelMessage() const
{
   return "Engineered item not deployed";
}


// When a menu is not active, we'll allow players to enter chat
bool EngineerHelper::isChatDisabled() const
{
   return isMenuBeingDisplayed();
}


// Static method, for testing
InputCode EngineerHelper::getInputCodeForOption(EngineerBuildObjects obj, bool keyBut)
{
   InputCode code = Parent::getInputCodeForOption(&engineerItemInfo[0], ARRAYSIZE(engineerItemInfo), obj, keyBut);

   TNLAssert(code != KEY_UNKNOWN, "InputCode not found!");

   return code;
}

};

