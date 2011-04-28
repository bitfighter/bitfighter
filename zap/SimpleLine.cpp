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

#include "SimpleLine.h"

// Constructor
SimpleLine::SimpleLine()
{
   //MenuUserInterface mAttrMenu = TestMenuUserInterface();
}


void SimpleLine::initialize()
{
   mVertSelected.resize(2);     // One spot for mPos, one for mDest
   unselectVerts();             // Set all verts to be unselected
}


void SimpleLine::renderDock()
{
   Color itemColor = getEditorRenderColor();

   glColor(itemColor);
   drawFilledSquare(getVert(0), 5);    // Draw origin of item to give user something to grab on the dock

   // Add a label
   F32 xpos = getVert(0).x;
   F32 ypos = getVert(0).y + 6;
   glColor(EditorUserInterface::DOCK_LABEL_COLOR);
   UserInterface::drawStringc(xpos, ypos, EditorUserInterface::DOCK_LABEL_SIZE, getOnDockName());
   
   if(mLitUp)
   {
      glColor(EditorUserInterface::HIGHLIGHT_COLOR);
      drawSquare(getVert(0), 8);
   }
}


static inline void labelSimpleLineItem(Point pos, F32 labelSize, const char *itemLabelTop, const char *itemLabelBottom)
{
   UserInterface::drawStringc(pos.x, pos.y + labelSize + 2, labelSize, itemLabelTop);
   UserInterface::drawStringc(pos.x, pos.y + 2 * labelSize + 5, labelSize, itemLabelBottom);
}


void SimpleLine::initializeEditor(F32 gridSize) 
{
   setVert(Point(0,0), 0);
   setVert(Point(1,0) * gridSize, 1);
}


// TODO: Put in editor ??
static const Color INSTRUCTION_TEXTCOLOR(1,1,1);
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 ATTR_TEXTSIZE = 10;                                          // called attrSize in the editor
static const Color ACTIVE_SPECIAL_ATTRIBUTE_COLOR = Color(.6, .6, .6);    
static const Color INACTIVE_SPECIAL_ATTRIBUTE_COLOR = Color(.6, .6, .6);      // already in editor, called inactiveSpecialAttributeColor


// Draw arrow that serves as the core of SimpleLine items in the editor
// Subclasses will fill in the rest
void SimpleLine::renderEditor(F32 currentScale)
{
   Point pos = getVert(0);
   Point dest = getVert(1);

   for(S32 i = 1; i >= 0; i--)
   {
      // Draw heavy colored line with colored core
      glLineWidth(i ? gLineWidth4 : gDefaultLineWidth);                
      glColor(getEditorRenderColor(), .15);


      F32 ang = pos.angleTo(dest);
      const F32 al = 15;                // Length of arrow-head, in editor units (15 pixels)
      const F32 angoff = .5;            // Pitch of arrow-head prongs

      glBegin(GL_LINES);
         glVertex2f(dest.x, dest.y);    // Draw arrowhead
         glVertex2f(dest.x - cos(ang + angoff) * al, dest.y - sin(ang + angoff) * al);
         glVertex2f(dest.x, dest.y);
         glVertex2f(dest.x - cos(ang - angoff) * al, dest.y - sin(ang - angoff) * al);

         // Draw highlight on 2nd pass if item is selected, but not while it's being edited
         if(!i && (mSelected || mLitUp) /*&& !isBeingEdited  <== passed into render method*/)
            glColor(getEditorRenderColor());

         glVertex(pos);                 // Draw connecting line
         glVertex(dest);
      glEnd();
   }

   renderEditorItem(currentScale);

   // Label item with message about what happens if user presses enter
   if(isSelected())
   {
      //glColor(INSTRUCTION_TEXTCOLOR);
      //UserInterface::drawStringf_2pt(pos, dest, INSTRUCTION_TEXTSIZE, -22, getEditMessage(0));
      //UserInterface::drawStringf_2pt(pos, dest, INSTRUCTION_TEXTSIZE, -22 - INSTRUCTION_TEXTSIZE - 2, getEditMessage(1));


      //glColor((gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::GoFastSnap)) ? ACTIVE_SPECIAL_ATTRIBUTE_COLOR : INACTIVE_SPECIAL_ATTRIBUTE_COLOR);
      //UserInterface::drawStringf_2pt(pos, dest, ATTR_TEXTSIZE, 10, "Speed: %d", mSpeed);

      //glColor((gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::GoFastSpeed)) ? ACTIVE_SPECIAL_ATTRIBUTE_COLOR : INACTIVE_SPECIAL_ATTRIBUTE_COLOR);
      //UserInterface::drawStringf_2pt(pos, dest, ATTR_TEXTSIZE, -2, "Snapping: %s", boolattr ? "On" : "Off");
   }

   // Label any selected or highlighted vertices
   if(vertSelected(0) || (mLitUp && isVertexLitUp(0)))         // "From" vertex
   {
      F32 alpha = 1;
      glColor(getDrawColor(), alpha);
      drawSquare(pos, 7);

      labelSimpleLineItem(pos, EditorUserInterface::DOCK_LABEL_SIZE, getOnScreenName(), getOriginBottomLabel());
   }
   else if(vertSelected(1) || (mLitUp && isVertexLitUp(1)))    // "To" vertex
   {
      F32 alpha = 1;
      glColor(getDrawColor(), alpha);
      drawSquare(dest, 7);

      labelSimpleLineItem(dest, EditorUserInterface::DOCK_LABEL_SIZE, getOnScreenName(), getDestinationBottomLabel());
   }
}


// Constructor
GoFastEditorAttributeMenuUserInterface::GoFastEditorAttributeMenuUserInterface()
{
   setMenuID(PlayerUI);
   menuItems.push_back(new CounterMenuItem("Speed", 100, 5, 1, 2000, "", "Really slow", "", KEY_UNKNOWN, KEY_UNKNOWN, Color(.35)));
   menuItems.push_back(new YesNoMenuItem("Snapping", true, NULL, "", KEY_UNKNOWN, KEY_UNKNOWN, Color(.35)));
}
//CounterMenuItem(const string &title, S32 value, S32 step, S32 minVal, S32 maxVal, const string &units, const string &minMsg, const string &help, 
//                KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN, Color color = Color(1,1,1)) :
//YesNoMenuItem(string title, bool currOption, void (*callback)(U32), string help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN, Color color = Color(1,1,1)) :


void GoFastEditorAttributeMenuUserInterface::onEscape()
{
   reactivatePrevUI();
}

//extern EditorUserInterface gEditorUserInterface;

void GoFastEditorAttributeMenuUserInterface::render()
{
   // Draw the editor screen
   gEditorUserInterface.render();     // better way than global?

 /*  if(mRenderInstructions)
      renderMenuInstructions();*/
   
   S32 gap = 3;
   Point offset = gEditorUserInterface.getCurrentOffset();
   Point center = (object->getVert(0) + object->getVert(1)) * gEditorUserInterface.getCurrentScale() / 2 + offset;

   S32 count = menuItems.size();
   S32 yStart = center.y - count * (ATTR_TEXTSIZE + gap) - 10;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * (ATTR_TEXTSIZE + gap);
      menuItems[i]->render(center.x, y, ATTR_TEXTSIZE, selectedIndex == i);
   }
}

