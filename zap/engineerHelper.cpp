//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "engineerHelper.h"
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "UIGame.h"
#include "Color.h"                  // For Color def
#include "Colors.h"
#include "ClientGame.h"
#include "JoystickRender.h"
#include "config.h"
#include "gameObjectRender.h"       // For drawSquare

#include "ship.h"

#include "OpenglUtils.h"

namespace Zap
{


EngineerConstructionItemInfo engineerItemInfo[] = {
      { EngineeredTurret,             KEY_1, BUTTON_1, true,  "Turret",          "", "Aim at a spot on a wall, and activate the module again." },
      { EngineeredForceField,         KEY_2, BUTTON_2, true,  "Force Field",     "", "Aim at a spot on a wall, and activate the module again." },
      { EngineeredTeleporterEntrance, KEY_3, BUTTON_3, true,  "Teleporter",      "", "Aim at a spot in open space, and activate the module again." },
      { EngineeredTeleporterExit,     KEY_4, BUTTON_4, false, "Teleporter Exit", "", "Aim at a spot in open space, and activate the module again." },
};


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EngineerHelper::EngineerHelper()
{
   mSelectedItem = -1;
   mEngineerCostructionItemInfos = Vector<EngineerConstructionItemInfo>(engineerItemInfo, ARRAYSIZE(engineerItemInfo));
}


// Destructor
EngineerHelper::~EngineerHelper()
{
   // Do nothing
}


HelperMenu::HelperMenuType EngineerHelper::getType() { return EngineerHelperType; }


void EngineerHelper::setSelectedEngineeredObject(U32 objectType)
{
   for(S32 i = 0; i < mEngineerCostructionItemInfos.size(); i++)
      if(mEngineerCostructionItemInfos[i].mObjectType == objectType)
         mSelectedItem = i;
}


void EngineerHelper::onMenuShow()
{
   Parent::onMenuShow();

   mSelectedItem = -1;
}


static Point deployPosition, deployNormal;

void EngineerHelper::render()
{
   S32 yPos = MENU_TOP;
   const S32 fontSize = 15;
   const Color engineerMenuHeaderColor (Colors::red);

   if(isMenuBeingDisplayed())    // Haven't selected an item yet, so show the menu
   {
      S32 xPos = getLeftEdgeOfMenuPos();

      drawMenuBorderLine(xPos, yPos, engineerMenuHeaderColor);

      glColor(engineerMenuHeaderColor);
      drawString(xPos, yPos, fontSize, "What do you want to Engineer?");
      yPos += fontSize + 10;

      GameSettings *settings = getGame()->getSettings();
      InputMode inputMode = settings->getInputCodeManager()->getInputMode();

      bool showKeys = settings->getIniSettings()->showKeyboardKeys || inputMode == InputModeKeyboard;

      for(S32 i = 0; i < mEngineerCostructionItemInfos.size(); i++)
      {
         // Don't show an option that shouldn't be shown!
         if(!mEngineerCostructionItemInfos[i].showOnMenu)
            continue;

         // Draw key controls for selecting the object to be created
         U32 joystickIndex = Joystick::SelectedPresetIndex;

         if(inputMode == InputModeJoystick)     // Only draw joystick buttons when in joystick mode
            JoystickRender::renderControllerButton(F32(xPos + (showKeys ? 0 : 20)), (F32)yPos, 
                                                   joystickIndex, mEngineerCostructionItemInfos[i].mButton, false);

         if(showKeys)
         {
            glColor(Colors::white);     // Render key in white
            JoystickRender::renderControllerButton((F32)xPos + 20, (F32)yPos, 
                                                   joystickIndex, mEngineerCostructionItemInfos[i].mKey, false);
         }

         glColor(0.1, 1.0, 0.1);     
         S32 x = drawStringAndGetWidth(xPos + 50, yPos, fontSize, mEngineerCostructionItemInfos[i].mName); 

         glColor(.2, .8, .8);    
         drawString(xPos + x, yPos, fontSize, mEngineerCostructionItemInfos[i].mHelp);      // The help string, if there is one

         yPos += fontSize + 7;
      }

      yPos += 2;

      drawMenuBorderLine(xPos, yPos - fontSize - 2, engineerMenuHeaderColor);
      yPos += 8;

      drawMenuCancelText(xPos, yPos, engineerMenuHeaderColor, fontSize);
   }
   else     // Have selected a module, need to indicate where to deploy
   {
      S32 xPos = UserInterface::horizMargin;
      glColor(Colors::green);
      drawStringf(xPos, yPos, fontSize, "Placing %s.", mEngineerCostructionItemInfos[mSelectedItem].mName);
      yPos += fontSize + 7;
      drawString(xPos, yPos, fontSize, mEngineerCostructionItemInfos[mSelectedItem].mInstruction);
   }
}


bool EngineerHelper::isMenuBeingDisplayed()
{
   return mSelectedItem == -1;
}


// Return true if key did something, false if key had no effect
// Runs on client
bool EngineerHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))    // Check for cancel keys
      return true;

   InputCodeManager *inputCodeManager = getGame()->getSettings()->getInputCodeManager();
   GameConnection *gc = getGame()->getConnectionToServer();

   if(isMenuBeingDisplayed())    // Menu is being displayed, so interpret keystrokes as menu items
   {
      for(S32 i = 0; i < mEngineerCostructionItemInfos.size(); i++)
      {
         // Disallow selecting unselectable items
         if(!mEngineerCostructionItemInfos[i].showOnMenu)
            continue;

         if(inputCode == mEngineerCostructionItemInfos[i].mKey || inputCode == mEngineerCostructionItemInfos[i].mButton)
         {
            mSelectedItem = i;
            return true;
         }
      }

      Ship *ship = dynamic_cast<Ship *>(gc->getControlObject());
      TNLAssert(ship, "Will this ever be true?  If not, we can replace with static cast/assert. If this does assert, "
                      "please document and remove this assert.");

      if(!ship || (inputCode == inputCodeManager->getBinding(InputCodeManager::BINDING_MOD1) && ship->getModule(0) == ModuleEngineer) ||
                  (inputCode == inputCodeManager->getBinding(InputCodeManager::BINDING_MOD2) && ship->getModule(1) == ModuleEngineer))
      {
         exitHelper();
         return true;
      }
   }
   else                       // Placing item
   {
      Ship *ship = dynamic_cast<Ship *>(gc->getControlObject());
      if(ship && ((inputCode == inputCodeManager->getBinding(InputCodeManager::BINDING_MOD1) && ship->getModule(0) == ModuleEngineer) ||
                  (inputCode == inputCodeManager->getBinding(InputCodeManager::BINDING_MOD2) && ship->getModule(1) == ModuleEngineer)))
      {
         EngineerModuleDeployer deployer;

         // Check deployment status on client; will be checked again on server,
         // but server will only handle likely valid placements
         if(deployer.canCreateObjectAtLocation(getGame()->getGameObjDatabase(), ship, mEngineerCostructionItemInfos[mSelectedItem].mObjectType))
         {
            // Send command to server to deploy, and deduct energy
            if(gc)
            {
               gc->c2sEngineerDeployObject(mEngineerCostructionItemInfos[mSelectedItem].mObjectType);
               S32 energyCost = Game::getModuleInfo(ModuleEngineer)->getPrimaryPerUseCost();
               ship->creditEnergy(-energyCost);    // Deduct energy from engineer
            }
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
   if(mSelectedItem != -1 && mEngineerCostructionItemInfos[mSelectedItem].mObjectType == EngineeredTeleporterExit)
   {
      GameConnection *gameConnection = getGame()->getConnectionToServer();

      if(gameConnection)
         gameConnection->c2sEngineerInterrupted(EngineeredTeleporterExit);
   }

   Parent::exitHelper();
}


// Basically draws a red box where the ship is pointing
void EngineerHelper::renderDeploymentMarker(Ship *ship)
{
   // Only render wall mounted items (not teleport)
   if(mSelectedItem != -1 &&
         EngineerModuleDeployer::findDeployPoint(ship, mEngineerCostructionItemInfos[mSelectedItem].mObjectType, deployPosition, deployNormal))
   {
      EngineerModuleDeployer deployer;
      bool canDeploy = deployer.canCreateObjectAtLocation(getGame()->getGameObjDatabase(), ship, mEngineerCostructionItemInfos[mSelectedItem].mObjectType);

      switch(mEngineerCostructionItemInfos[mSelectedItem].mObjectType)
      {
         case EngineeredTurret:
         case EngineeredForceField:
            if(canDeploy)
               glColor(Colors::green);
            else
               glColor(Colors::red);

            drawSquare(deployPosition, 5);
            break;

         case EngineeredTeleporterEntrance:
         case EngineeredTeleporterExit:
            if(canDeploy)
               renderTeleporterOutline(deployPosition, 75.f, Colors::green);
            else
               renderTeleporterOutline(deployPosition, 75.f, Colors::red);
            break;

         default:
            break;
      }
   }
}


const char *EngineerHelper::getCancelMessage()
{
   return "Engineered item not deployed";
}


// When a menu is not active, we'll allow players to enter chat
bool EngineerHelper::isChatDisabled()
{
   return isMenuBeingDisplayed();
}

};

