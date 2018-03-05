//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _EDITOR_UNDO_MANAGER_H_
#define _EDITOR_UNDO_MANAGER_H_


#include "EditorWorkUnit.h"

#include "tnlVector.h"
#include <memory>

using namespace TNL;

namespace Zap { 

// Forward declaration of Zap objects must be in Zap namespace but not in Editor namespace
class BfObject;
class Level;
   
namespace Editor
{


class EditorUndoManager
{
public:
   enum ChangeIdentifier {
      ChangeIdMergeWalls,
      ChangeIdNone
   };

private:
   S32 mUndoLevel;
   S32 mSavedAtLevel;

   Vector<EditorWorkUnit *> mActions;
   Vector<EditorWorkUnit *> mTransactionActions;
   EditorUserInterface *mEditor;

   BfObject *mOrigObject;
   ChangeIdentifier mChangeIdentifier;

   bool mInTransaction;
   bool mInMergeAction;

   void fixupActionList();

public:
   EditorUndoManager(EditorUserInterface *editor);             // Constructor
   virtual ~EditorUndoManager();    // Destructor

   void setLevel();

   void saveAction(Level *level, EditorAction action, const BfObject *bfObject);
   void saveAction(EditorAction action, const BfObject *origObject, const BfObject *changedObject);

   // For two-step change actions
   void saveChangeAction_before(const BfObject *origObject);
   void saveChangeAction_after(Level* level, const BfObject *changedObject);

   void saveCreateActionAndMergeWithNextUndoState();

   void undo();
   void redo();

   void startTransaction();
   void endTransaction(ChangeIdentifier ident = ChangeIdNone);
   void rollbackTransaction(Level *level);
   bool inTransaction() const;
   bool noMoreCanDoRedo() const;

   void discardAction();

   void clearAll();
   void levelSaved();
   bool needToSave() const;

   bool undoAvailable() const;
   bool redoAvailable() const;
};



} } // Nested namespace

#endif
