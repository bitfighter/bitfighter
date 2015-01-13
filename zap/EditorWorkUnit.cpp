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
EditorWorkUnit::EditorWorkUnit(boost::shared_ptr<Level> level, EditorUserInterface *editor, EditorAction action)
{
   mLevel = level;
   mEditor = editor;
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
EditorWorkUnitCreate::EditorWorkUnitCreate(const boost::shared_ptr<Level> &level, 
                                           EditorUserInterface *editor,
                                           const BfObject *bfObject) : 
   Parent(level, editor, ActionCreate)
{
   mCreatedObject = bfObject->clone();
}


// Destructor
EditorWorkUnitCreate::~EditorWorkUnitCreate()
{
   delete mCreatedObject;
}


void EditorWorkUnitCreate::undo()
{
   mLevel->deleteObject(mCreatedObject->getSerialNumber());

   if(mEditor)
      mEditor->doneDeletingObjects();
}


void EditorWorkUnitCreate::merge(const EditorWorkUnit *workUnit)
{
   TNLAssert(false, "Not implemented!");
}


void EditorWorkUnitCreate::redo()
{
   mLevel->addToDatabase(mCreatedObject->clone());

   if(mEditor)
      mEditor->doneAddingObjects(mCreatedObject->getSerialNumber());
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
EditorWorkUnitDelete::EditorWorkUnitDelete(const boost::shared_ptr<Level> &level, 
                                           EditorUserInterface *editor,
                                           const BfObject *bfObject) : 
   Parent(level, editor, ActionDelete)
{
   mDeletedObject = bfObject->clone();

   TNLAssert(mDeletedObject->getSerialNumber() == bfObject->getSerialNumber(), "Expect same serial number!");
}


// Destructor
EditorWorkUnitDelete::~EditorWorkUnitDelete()
{
   delete mDeletedObject;
}


void EditorWorkUnitDelete::undo()
{
   mLevel->addToDatabase(mDeletedObject->clone());

   if(mEditor)
      mEditor->doneAddingObjects(mDeletedObject->getSerialNumber());
}


void EditorWorkUnitDelete::redo()
{
   mLevel->deleteObject(mDeletedObject->getSerialNumber());   

   if(mEditor)
      mEditor->doneDeletingObjects();
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

// Helper function -- iterates through all objects in list and saves their geometries for posterity
static void getGeometries(const Vector<boost::shared_ptr<BfObject> > &objects, Vector<Vector<Point> > &geoms)
{
   geoms.reserve(objects.size());

   Vector<Point> points;
   for(S32 i = 0; i < objects.size(); i++)
   {
      const Vector<Point> *pointsPtr = objects[i]->getGeometry().getOutline();
      points.clear();
      points.reserve(pointsPtr->size());

      for(S32 i = 0; i < pointsPtr->size(); i++)
         points.push_back(pointsPtr->get(i));
         
      geoms.push_back(points);
   }
}


// Constructor
EditorWorkUnitChange::EditorWorkUnitChange(const boost::shared_ptr<Level> &level, 
                                           EditorUserInterface *editor,
                                           const BfObject *origObject,
                                           const BfObject *changedObject) : 
   Parent(level, editor, ActionChange)
{
   mOrigObject = origObject->clone();
   mChangedObject = changedObject->clone();
}


// Destructor
EditorWorkUnitChange::~EditorWorkUnitChange()
{
   delete mOrigObject;
   delete mChangedObject;
}


void EditorWorkUnitChange::undo()
{
   mLevel->swapObject(mChangedObject->getSerialNumber(), mOrigObject);

   if(mEditor)
      mEditor->doneChangingGeoms(mOrigObject->getSerialNumber());
}


void EditorWorkUnitChange::redo()
{
   mLevel->swapObject(mOrigObject->getSerialNumber(), mChangedObject);

   if(mEditor)
      mEditor->doneChangingGeoms(mChangedObject->getSerialNumber());
}


void EditorWorkUnitChange::merge(const EditorWorkUnit *workUnit)
{
   delete mChangedObject;
   mChangedObject = workUnit->getObject()->clone();
}


S32 EditorWorkUnitChange::getSerialNumber() const
{
   return mChangedObject->getSerialNumber();
}


const BfObject *EditorWorkUnitChange::getObject() const
{
   return mChangedObject;
}


EditorAction EditorWorkUnitChange::getAction() const
{
   return ActionChange;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorWorkUnitGroup::EditorWorkUnitGroup(const boost::shared_ptr<Level> &level, 
                                         EditorUserInterface *editor,
                                         const Vector<EditorWorkUnit *> &workUnits) :
   Parent(level, editor, ActionChange)
{
   mWorkUnits = workUnits;
}


// Destructor
EditorWorkUnitGroup::~EditorWorkUnitGroup()
{
   mWorkUnits.deleteAndClear();
}


void EditorWorkUnitGroup::undo()
{
   // Undo in reverse order in case on item depends on another
   for(S32 i = mWorkUnits.size() - 1; i >= 0 ; i--)
      mWorkUnits[i]->undo();

   //if(mEditor)
   //   mEditor->doneChangingGeoms(mOrigObject->getSerialNumber());
}


void EditorWorkUnitGroup::redo()
{
   for(S32 i = 0; i < mWorkUnits.size(); i++)
      mWorkUnits[i]->redo();

   //if(mEditor)
   //   mEditor->doneChangingGeoms(mChangedObject->getSerialNumber());
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
