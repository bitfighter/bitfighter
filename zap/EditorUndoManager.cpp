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
   mChangeIdentifier = ChangeIdNone;
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
   // Handle special case... if we're in a MergeAction, and the first part was creating the object, and
   // now we want to delete the object, we'll just back it out and call the whole thing a wash, as if the
   // create/delete cycle never occurred.  An example of where this happens is when you create an object
   // by dragging from the dock (creating a create action that expects to be merged with a move action),
   // but instead drag the item back to the dock; now we have a create merging with a delete, and the 
   // whole things should just go away.
   if(mInMergeAction && action == ActionDelete)
   {
      mInMergeAction = false;
      return;
   }


   TNLAssert(!mInMergeAction, "Should not be in the midst of a merge action!");

   // Pick which action list we should be working with
   Vector<EditorWorkUnit *> *actionList = mInTransaction ? &mTransactionActions : &mActions;

   if(!mInTransaction)
      fixupActionList();

   if(action == Editor::ActionCreate)
      actionList->push_back(new EditorWorkUnitCreate(mLevel, mEditor, bfObject)); 
   else if(action == Editor::ActionDelete)
      actionList->push_back(new EditorWorkUnitDelete(mLevel, mEditor, bfObject));
   else 
      TNLAssert(false, "Action not implemented!");

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
   mChangeIdentifier = ChangeIdNone;
}


bool EditorUndoManager::noMoreCanDoRedo()
{
   return mUndoLevel == mActions.size();
}


void EditorUndoManager::undo()
{
   TNLAssert(!mInTransaction, "No undo in transactions!");

   // Nothing to undo!  (will probably never happen, as this is checked by caller)
   if(mUndoLevel == 0)
      return;

   mUndoLevel--;
   mActions[mUndoLevel]->undo();

   mChangeIdentifier = ChangeIdNone;
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


// For the moment, we'll assume that the objects will be in the same order
static bool transactionObjectsEqual(const Vector<EditorWorkUnit *> &first, const EditorWorkUnit *second)
{
   TNLAssert(dynamic_cast<const EditorWorkUnitGroup *>(second), "Expected a group transaction here!");

   if(first.size() != static_cast<const EditorWorkUnitGroup *>(second)->getWorkUnitCount())
      return false;

   for(S32 i = 0; i < first.size(); i++)
      if(first[i]->getSerialNumber() != static_cast<const EditorWorkUnitGroup *>(second)->getWorkUnitObjectSerialNumber(i))
         return false;

   return true;
}


void EditorUndoManager::endTransaction(ChangeIdentifier ident)    // ident defaults to ChangeIdNone
{
   TNLAssert(mInTransaction, "Should be in a transaction!");

   mInTransaction = false;

   if(mTransactionActions.size() == 0)
   {
      mChangeIdentifier = ChangeIdNone;
      return;
   }


   // If the changeIdentifier matches our previous transaction, and the objects are the same, then we
   // can merge this transaction with the previous one.  To merge, we'll replace the changed objects
   // of the previous transaction with those from this one, without creating a new undo state.
   if(ident != ChangeIdNone && ident == mChangeIdentifier && transactionObjectsEqual(mTransactionActions, mActions.last()))
   {
      TNLAssert(dynamic_cast<EditorWorkUnitGroup *>(mActions.last()), "Expected a WorkUnitGroup!");
      static_cast<EditorWorkUnitGroup *>(mActions.last())->mergeTransactions(mTransactionActions);
   }
   else
   {
      EditorWorkUnitGroup *group = new EditorWorkUnitGroup(mLevel, mEditor, mTransactionActions);

      fixupActionList();
      mActions.push_back(group);
      mUndoLevel = mActions.size();
   }


   mChangeIdentifier = ident;

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
