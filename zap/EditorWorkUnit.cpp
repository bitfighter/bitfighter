//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "EditorWorkUnit.h"

#include "Level.h"
#include "UIEditor.h"

namespace Zap { namespace Editor 
{


// Constructor
EditorWorkUnit::EditorWorkUnit(EditorUserInterface *editor, EditorAction action)
{
   mAction = action;
}


// Destructor
EditorWorkUnit::~EditorWorkUnit()
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorWorkUnitCreate::EditorWorkUnitCreate(EditorUserInterface *editor,
                                           const BfObject *bfObject) : 
   Parent(editor, ActionCreate)
{
   mCreatedObject = bfObject->clone();
}


// Destructor
EditorWorkUnitCreate::~EditorWorkUnitCreate()
{
   //delete mCreatedObject;
}


void EditorWorkUnitCreate::undo(EditorUserInterface *editor)
{
   editor->getLevel()->deleteObject(mCreatedObject->getSerialNumber());

   editor->doneDeletingObjects();
}


void EditorWorkUnitCreate::merge(const EditorWorkUnit *workUnit)
{
   TNLAssert(false, "Not implemented!");
}


void EditorWorkUnitCreate::redo(EditorUserInterface *editor)
{
   editor->getLevel()->addToDatabase(mCreatedObject->clone());

   editor->doneAddingObjects(mCreatedObject->getSerialNumber());
}


S32 EditorWorkUnitCreate::getSerialNumber() const
{
   return mCreatedObject->getSerialNumber();
}


const BfObject *EditorWorkUnitCreate::getObject() const
{
   return mCreatedObject;
}


EditorAction EditorWorkUnitCreate::getAction() const
{
   return ActionCreate;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorWorkUnitDelete::EditorWorkUnitDelete(EditorUserInterface *editor, const BfObject *bfObject) : 
   Parent(editor, ActionDelete)
{
   mDeletedObject = bfObject->clone();

   TNLAssert(mDeletedObject->getSerialNumber() == bfObject->getSerialNumber(), "Expect same serial number!");
}


// Destructor
EditorWorkUnitDelete::~EditorWorkUnitDelete()
{
   //delete mDeletedObject;
}


void EditorWorkUnitDelete::undo(EditorUserInterface *editor)
{
   DatabaseObject *obj = mDeletedObject->clone();
   TNLAssert(((BfObject *)obj)->getSerialNumber() == mDeletedObject->getSerialNumber(), "Expected clone to keep same serial number!");

   editor->getLevel()->addToDatabase(obj);

   editor->doneAddingObjects(mDeletedObject->getSerialNumber());
}


void EditorWorkUnitDelete::redo(EditorUserInterface *editor)
{
   editor->getLevel()->deleteObject(mDeletedObject->getSerialNumber());   

   editor->doneDeletingObjects();
}


void EditorWorkUnitDelete::merge(const EditorWorkUnit *workUnit)
{
   TNLAssert(false, "Not implemented!");
}


S32 EditorWorkUnitDelete::getSerialNumber() const
{
   return mDeletedObject->getSerialNumber();
}


const BfObject *EditorWorkUnitDelete::getObject() const
{
   return NULL; // ?
}


EditorAction EditorWorkUnitDelete::getAction() const
{
   return ActionDelete;
}


////////////////////////////////////////
////////////////////////////////////////
//
//// Helper function -- iterates through all objects in list and saves their geometries for posterity
//static void getGeometries(const Vector<shared_ptr<BfObject> > &objects, Vector<Vector<Point> > &geoms)
//{
//   geoms.reserve(objects.size());
//
//   Vector<Point> points;
//   for(S32 i = 0; i < objects.size(); i++)
//   {
//      const Vector<Point> *pointsPtr = objects[i]->getGeometry().getOutline();
//      points.clear();
//      points.reserve(pointsPtr->size());
//
//      for(S32 i = 0; i < pointsPtr->size(); i++)
//         points.push_back(pointsPtr->get(i));
//         
//      geoms.push_back(points);
//   }
//}


// Constructor
EditorWorkUnitChange::EditorWorkUnitChange(EditorUserInterface *editor,
                                           const BfObject *origObject,
                                           const BfObject *changedObject) : 
   Parent(editor, ActionChange)
{
   mOrigObject.set(origObject->clone());
   mChangedObject.set(changedObject->clone());
}


// Destructor
EditorWorkUnitChange::~EditorWorkUnitChange()
{
   //delete mOrigObject;
   //delete mChangedObject;
}


void EditorWorkUnitChange::undo(EditorUserInterface *editor)
{
   editor->getLevel()->swapObject(mChangedObject->getSerialNumber(), mOrigObject);

   editor->doneChangingGeoms(mOrigObject->getSerialNumber());
}


void EditorWorkUnitChange::redo(EditorUserInterface *editor)
{
   editor->getLevel()->swapObject(mOrigObject->getSerialNumber(), mChangedObject);

   editor->doneChangingGeoms(mChangedObject->getSerialNumber());
}


void EditorWorkUnitChange::merge(const EditorWorkUnit *workUnit)
{
   delete mChangedObject;
   mChangedObject.set(workUnit->getObject()->clone());
}


S32 EditorWorkUnitChange::getSerialNumber() const
{
   return mChangedObject->getSerialNumber();
}


const BfObject *EditorWorkUnitChange::getObject() const
{
   return mChangedObject.getPointer();
}


EditorAction EditorWorkUnitChange::getAction() const
{
   return ActionChange;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorWorkUnitGroup::EditorWorkUnitGroup(EditorUserInterface *editor, const Vector<EditorWorkUnit *> &workUnits) :
   Parent(editor, ActionChange)
{
   mWorkUnits = workUnits;
}


// Destructor
EditorWorkUnitGroup::~EditorWorkUnitGroup()
{
   mWorkUnits.deleteAndClear();
}


void EditorWorkUnitGroup::undo(EditorUserInterface *editor)
{
   // Undo in reverse order in case on item depends on another
   for(S32 i = mWorkUnits.size() - 1; i >= 0 ; i--)
      mWorkUnits[i]->undo(editor);
}


void EditorWorkUnitGroup::redo(EditorUserInterface *editor)
{
   for(S32 i = 0; i < mWorkUnits.size(); i++)
      mWorkUnits[i]->redo(editor);
}


void EditorWorkUnitGroup::merge(const EditorWorkUnit *workUnit)
{
   TNLAssert(false, "Not implemented!");
}


S32 EditorWorkUnitGroup::getWorkUnitCount() const
{
   return mWorkUnits.size();
}


S32 EditorWorkUnitGroup::getWorkUnitObjectSerialNumber(S32 index) const
{
   return mWorkUnits[index]->getSerialNumber();
}


S32 EditorWorkUnitGroup::getSerialNumber() const
{
   TNLAssert(false, "No object associated with a GroupWorkUnit!");
   return 0;
}


const BfObject *EditorWorkUnitGroup::getObject() const
{
   TNLAssert(false, "Not implemented!");
   return 0;
}


void EditorWorkUnitGroup::mergeTransactions(const Vector<EditorWorkUnit *> &newWorkUnits)
{
   for(S32 i = 0; i < mWorkUnits.size(); i++)
      mWorkUnits[i]->merge(newWorkUnits[i]);
}


EditorAction EditorWorkUnitGroup::getAction() const
{
   return ActionGroup;
}


} };  // Nested namespace
