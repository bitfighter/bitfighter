//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "EditorUndoManager.h"

#include "BfObject.h"


namespace Zap { namespace Editor 
{


// Constructor
EditorUndoManager::EditorUndoManager()
{
   clearAll();
}


// Destructor
EditorUndoManager::~EditorUndoManager()
{
   mActions.deleteAndClear();
   delete mOrigObject;
}


void EditorUndoManager::setLevel(boost::shared_ptr<Level> level, EditorUserInterface *editor)
{
   mLevel = level;
   mEditor = editor;
   mActions.deleteAndClear();    // Old actions won't work with new level!
   mUndoLevel = 0;
}


// If we do a new action in the midst a set of undo/redo actions, we need to truncate the action
// list so that the new action is the new top; previously redoable actions will be lost to history
void EditorUndoManager::fixupActionList()
{
   if(mUndoLevel < mActions.size())
   {
      for(S32 i = mActions.size() - 1; i >= mUndoLevel; i--)
         mActions.erase(i);

      mSavedAtLevel = -1;
   }
}


void EditorUndoManager::saveAction(EditorAction action, const BfObject *bfObject)
{
   TNLAssert(!mInMergeAction, "Should not be in the midst of a merge action!");

   Vector<EditorWorkUnit *> *actionList = mInTransaction ? &mTransactionActions : &mActions;

   if(!mInTransaction)
      fixupActionList();

   if(action == Editor::ActionCreate)
      actionList->push_back(new EditorWorkUnitCreate(mLevel, mEditor, bfObject)); 
   else if(action == Editor::ActionDelete)
      actionList->push_back(new EditorWorkUnitDelete(mLevel, mEditor, bfObject));
   else 
      TNLAssert(false, "Action not implemented!")

   if(!mInTransaction)
      mUndoLevel = mActions.size();
}


void EditorUndoManager::saveAction(EditorAction action, const BfObject *origObject, const BfObject *changedObject)
{
   Vector<EditorWorkUnit *> *actionList = mInTransaction ? &mTransactionActions : &mActions;

   if(!mInTransaction)
      fixupActionList();

   if(action == Editor::ActionChange)
   {
      if(mInMergeAction)
      {
         mInMergeAction = false;
         actionList->push_back(new Editor::EditorWorkUnitCreate(mLevel, mEditor, changedObject));
      }
      else
         actionList->push_back(new Editor::EditorWorkUnitChange(mLevel, mEditor, origObject, changedObject));

   }
   else
      TNLAssert(false, "Action not implemented!");

   if(!mInTransaction)
      mUndoLevel = mActions.size();
}


void EditorUndoManager::saveChangeAction_before(const BfObject *origObject)
{
   TNLAssert(!mOrigObject, "Expect this to be NULL here!");
   mOrigObject = origObject->clone();
}


void EditorUndoManager::saveChangeAction_after(const BfObject *changedObject)
{
   TNLAssert(mOrigObject, "Expect this not to be NULL here!");
   TNLAssert(mOrigObject->getSerialNumber() == changedObject->getSerialNumber(), "Different object!");

   if(mInMergeAction)
   {
      mInMergeAction = false;
      saveAction(ActionCreate, changedObject);
   }
   else
      saveAction(ActionChange, mOrigObject, changedObject);

   delete mOrigObject;
   mOrigObject = NULL;
}


// This is a create action, but we won't save it until we get the next action to save... we anticipate that one will
// be a change action, in which case we'll transform that change action into a create.  This is useful when dragging items
// from the dock, and we want to create the new item in a location that won't be determined until later when the user
// stops dragging.
void EditorUndoManager::saveCreateActionAndMergeWithNextUndoState()
{
   mInMergeAction = true;
}


// Discard the most recent action
void EditorUndoManager::discardAction()
{
   if(mUndoLevel == mActions.size())
      mUndoLevel--;

   mActions.erase(mActions.size() - 1);
}


void EditorUndoManager::clearAll()
{
   mActions.clear();
   mTransactionActions.clear();

   mUndoLevel = 0;
   mSavedAtLevel = 0;
   mInTransaction = false;
   mInMergeAction = false;
   mOrigObject = NULL;
}


bool EditorUndoManager::noMoreCanDoRedo()
{
   return mUndoLevel == mActions.size();
}


void EditorUndoManager::undo()
{
   TNLAssert(!mInTransaction, "No undo in transactions!");

   // Nothing to undo!
   if(mUndoLevel == 0)
      return;

   mUndoLevel--;
   mActions[mUndoLevel]->undo();
}


void EditorUndoManager::redo()
{
   TNLAssert(!mInTransaction, "No redo in transactions!");

   if(mUndoLevel >= mActions.size())
      return;

   mActions[mUndoLevel]->redo();
   mUndoLevel++;
}


bool EditorUndoManager::undoAvailable()
{
   return mUndoLevel > 0;
}


bool EditorUndoManager::redoAvailable()
{
   return mUndoLevel <= mActions.size();
}


void EditorUndoManager::startTransaction()
{
   TNLAssert(!mInTransaction, "No nested transactions!");

   mInTransaction = true;
}


void EditorUndoManager::endTransaction()
{
   TNLAssert(mInTransaction, "Should be in a transaction!");

   mInTransaction = false;

   if(mTransactionActions.size() == 0)
      return;

   EditorWorkUnitGroup *group = new EditorWorkUnitGroup(mLevel, mEditor, mTransactionActions);

   fixupActionList();
   mActions.push_back(group);
   mUndoLevel = mActions.size();

   mTransactionActions.clear();
}


void EditorUndoManager::rollbackTransaction()
{
   TNLAssert(mInTransaction, "Should be in a transaction!");

   mInTransaction = false;

   if(mTransactionActions.size() == 0)
      return;

   endTransaction();
   undo();
}


bool EditorUndoManager::inTransaction()
{
   return mInTransaction;
}


void EditorUndoManager::levelSaved()
{
   mSavedAtLevel = mUndoLevel;
}


bool EditorUndoManager::needToSave() const
{
   return mUndoLevel != mSavedAtLevel;
}


} };  // Nested namespace
