//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_VEHICLE_DESIGNER_H_
#define _UI_VEHICLE_DESIGNER_H_

#include "helperMenu.h"
#include "XtankShape.h"    // for XtankDesign, XtankBody, XtankWeapon
#include "tnlVector.h"
#include "Timer.h"
#include <string>


#define COL_NUM_ASSERT static_assert(colCount <= TableRow::MaxTableColumns, "Too many columns!");


namespace Zap
{

enum ColAlignmemt : S32
{
   ALIGN_LEFT = 0,
   ALIGN_CENTER,
   ALIGN_RIGHT,
};


struct TableRow
{
   static const S32 MaxTableColumns = 14;
   const char *cells[MaxTableColumns];
   bool highlighted;       // true = force highlighted row color
};


struct TableColumn
{
   const char *header;
   ColAlignmemt headerAlignment;
   ColAlignmemt dataAlignment;
   S32 headerNudge; // Use this to make bespoke adjustments to the header position
   S32 columnNudge = 0;
};


class ComponentInfo
{
private:
   mutable bool mComputed = false;
   void computeColWidths(S32 fontSize) const;

   mutable S32 mColHeaderWidths[TableRow::MaxTableColumns];
   mutable S32 mColDataWidths[TableRow::MaxTableColumns];
   mutable S32 mColTotalWidths[TableRow::MaxTableColumns];

   virtual void fillRows() const = 0;


protected:
   static const S32 COL_LEN = 24;
   const S32 maxHeaderLines;

   explicit ComponentInfo(S32 maxHeaderLines) : maxHeaderLines(maxHeaderLines) {  /* Do nothing */ }

public:
   virtual S32 getColCount() const = 0;
   virtual S32 getRowCount() const = 0;
   virtual const TableColumn *getColumns() const = 0;
   virtual const TableRow *getRows() const = 0;

   S32 render(S32 left, S32 top, S32 fontSize, S32 rowGap, S32 highlightedRow, const VehicleDesign &design) const;
};



template<typename E>
constexpr E nextEnum(E e) {
   using T = typename std::underlying_type<E>::type;
   T v = static_cast<T>(e);
   v = (v + 1 == static_cast<T>(E::COUNT)) ? 0 : v + 1;
   return static_cast<E>(v);
}

template<typename E>
constexpr E prevEnum(E e) {
   using T = typename std::underlying_type<E>::type;
   T v = static_cast<T>(e);
   v = (v == 0) ? static_cast<T>(E::COUNT) - 1 : v - 1;
   return static_cast<E>(v);
}


// Compute the tallest header (multiline support) for table.
constexpr S32 computeTallestHeader(const TableColumn *cols, S32 columnCount)
{
   S32 maxHeaderLines = 1;
   for(S32 c = 0; c < columnCount; c++)
   {
      const char *hdr = cols[c].header ? cols[c].header : "";
      S32 lines = 1;

      // Scan for \n chars
      for(const char *p = hdr; *p; p++)
         if(*p == '\n')
            lines++;

      if(lines > maxHeaderLines)
         maxHeaderLines = lines;
   }

   return maxHeaderLines;
}


class BodyInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 9;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[VehicleBodyCount];

   BodyInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return VehicleBodyCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class ArmorInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 6;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[XtankArmorCount];

   ArmorInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankArmorCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class EngineInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 8;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[XtankEngineCount];

   EngineInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankEngineCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class TreadInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[XtankTreadCount];

   TreadInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankTreadCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class SuspensionInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[XtankSuspensionCount];

   SuspensionInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankSuspensionCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class BumperInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[XtankBumperCount];

   BumperInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankBumperCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class HeatSinkInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[1];

   HeatSinkInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return 1; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class SpecialInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 6;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[XtankSpecialCount];

   SpecialInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankSpecialCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


class ArmorAllocationInfo : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 colCount = 3;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[VehicleSidesCount];

   ArmorAllocationInfo() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return VehicleSidesCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }

   void updateArmorAllocationMenuItems(XtankArmor armorType, const S32 *armor) const;
};


// WeaponInfo: row 0 = "None", rows 1..XtankWeapon::COUNT = actual weapons.
class WeaponInfo2 : public ComponentInfo
{
private:
   void fillRows() const override;

public:
   static const S32 rowCount = (S32)XtankWeapon::COUNT;     // Includes NONE row
   static const S32 colCount = 13;
   COL_NUM_ASSERT;

   static const TableColumn columns[colCount];
   mutable TableRow rows[rowCount];

   WeaponInfo2() : ComponentInfo(computeTallestHeader(columns, colCount)) {}

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return rowCount; }
   const TableColumn *getColumns() const override { return columns; }
   const TableRow *getRows() const override { return rows; }
};


struct LabelWidth
{
   const char *label;
   S32 width;
};


class VehicleDesignerUserInterface : public HelperMenu
{
   typedef HelperMenu Parent;

   private:
      BodyInfo            mBodyInfo;
      ArmorInfo           mArmorInfo;
      EngineInfo          mEngineInfo;
      TreadInfo           mTreadInfo;
      SuspensionInfo      mSuspensionInfo;
      BumperInfo          mBumperInfo;
      HeatSinkInfo        mHeatSinkInfo;
      WeaponInfo2         mWeaponInfo;
      SpecialInfo         mSpecialsInfo;
      ArmorAllocationInfo mArmorAllocationInfo;

      VehicleDesign mDesignInProgress;
      VehicleDesign mOriginalDesign;
      Phase mPhase;

      S32 mWeaponSlot;

      Vector<OverlayMenuItem> mBodyItems;
      Vector<OverlayMenuItem> mEngineItems;
      Vector<OverlayMenuItem> mTreadItems;
      Vector<OverlayMenuItem> mArmorItems;
      Vector<OverlayMenuItem> mArmorSidesItems;
      Vector<OverlayMenuItem> mSuspensionItems;
      Vector<OverlayMenuItem> mBumperItems;
      Vector<OverlayMenuItem> mSpecialsItems;
      Vector<OverlayMenuItem> mHeatSinkItems;
      Vector<OverlayMenuItem> mWeaponItems;

      S32 mBodyButtonsWidth;
      S32 mBodyItemsDisplayWidth;
      S32 mEngineButtonsWidth;
      S32 mEngineItemsDisplayWidth;
      S32 mTreadButtonsWidth;
      S32 mTreadItemsDisplayWidth;
      S32 mArmorButtonsWidth;
      S32 mArmorItemsDisplayWidth;
      S32 mArmorSidesButtonsWidth;
      S32 mArmorSidesItemsDisplayWidth;
      S32 mSuspensionButtonsWidth;
      S32 mSuspensionItemsDisplayWidth;
      S32 mBumperButtonsWidth;
      S32 mBumperItemsDisplayWidth;
      S32 mSpecialsButtonsWidth;
      S32 mSpecialsItemsDisplayWidth;
      S32 mHeatSinkButtonsWidth;
      S32 mHeatSinkItemsDisplayWidth;
      S32 mWeaponButtonsWidth;
      S32 mWeaponItemsDisplayWidth;
      S32 mWeaponMountButtonsWidth;

      // Index of the currently highlighted (preview) item in the active phase's
      // item list.  Wraps within the valid range.  Cycled by UP/DOWN arrow keys.
      S32 mHighlightedIndex;

      // Panel-swap transition: scroll the new content up from the bottom.
      Timer mTransitionTimer;
      Phase mFromPhase;     // Phase being scrolled out
      S32 mFromPanelTop;    // panelBotForPhase(mFromPhase) captured at transition start
      S32 mToPanelTop;      // panelBotForPhase(mPhase) captured at transition start
      Phase mTransitionFromPhase;
      bool mTransitioningForward;


      void buildBodyItems();
      void buildEngineItems();
      void buildTreadItems();
      void buildArmorItems();
      void buildArmorSidesItems();  // 4 static items (Front/Back/Left/Right) for point allocation
      void buildSuspensionItems();
      void buildBumperItems();
      void buildSpecialsItems();
      void buildHeatSinkItems();
      void buildWeaponItems();
      S32 countWeaponsOnMount(XtankMountLocation mount, S32 excludeSlot) const;

      void normalizeWeaponPanels();
      void updateItemColors(Vector<OverlayMenuItem> &items);  // Highlight selected item

      void navigateForward(bool changePhase);             // Commit + move to next phase (carousel forward)
      void navigateBackward(bool changePhase);            // Move to previous phase (carousel back)
      void navigate();                    // Common functionalityu for prev 2 functions

      void applyDesign();                 // Finalise and propagate the chosen design

      void reduceArmor(S32 side);

      // Returns a human-readable side label ("Front", "Rear", "Left", "Right", or "Center")
      // for a given turret slot on a given body, based on the turret's x/y mount position.
      static const char *getTurretSideLabel(S32 bodyIdx, S32 slot);

      // Restore mHighlightedIndex from mDesignInProgress when entering a phase.
      S32 getHighlightedIndexForPhase() const;

      // Returns the menu-panel display width for a given phase.
      S32 widthForPhase(Phase phase) const;

      // Returns the bottom y-coordinate of the panel for a given phase,
      // sized to exactly contain the content (items + help text).
      S32 panelBotForPhase(Phase phase) const;


      // Draw the tab bar along the bottom and the single active panel above it.
      void renderFloatingMenus();

      // Draw the bottom tab strip.
      void renderTabBar();

      // Returns the tab label for a given phase.
      LabelWidth getTabLabel(Phase phase) const;

      // Draw a single panel
      // centerFraction: 0.0=fully adjacent/background, 1.0=fully center/active
      void renderCard(S32 left, S32 top, S32 right, S32 bot, Phase phase, F32 centerFraction);
      S32 renderItemTable(const Vector<OverlayMenuItem> *items, S32 top, S32 left, bool isActive, Phase phase, S32 highlightedIndex, F32 grayLevel);
      S32 renderHeatSinkPanel(S32 top, S32 left, F32 alpha, S32 colTwoX);

      void renderHelpText(S32 x, S32 y) const;


      // Draw the title of the Weapons panel
      void renderWeaponPanelTitle(S32 titlex, S32 titley, S32 right, S32 size, bool isCenter);

      ComponentInfo *getCurrentComponentInfo(bool isCenter);


      // Returns the item vector for a given phase.
      const Vector<OverlayMenuItem> *getItemsForPhase(Phase phase) const;

      // Returns the item count for the current phase (used by UP/DOWN cycling).
      S32 currentPhaseItemCount();

      void renderArmorStats(S32 left, S32 y, S32 fontSize, F32 alpha) const;
      void renderSpecialsStats(S32 left, S32 y, F32 alpha) const;
      void renderWeaponStats(S32 left, S32 y, F32 alpha) const;
      const char *getSpecialsHelpString(S32 specialIndex) const;


   public:
      VehicleDesignerUserInterface();
      virtual ~VehicleDesignerUserInterface();

      HelperMenu::HelperMenuType getType();

      void onActivated();
      void render();
      void idle(U32 delta); // Update transition animation
      bool processInputCode(InputCode inputCode);

      void activateHelp(UIManager *uiManager);

      // Fills phaseAtSlot[] and weaponSlotAtSlot[] for `size` display slots centered on
      // the current phase/weapon-slot, using a linear clamped virtual sequence.
      // Exposed as public/static so it can be tested without a live VehicleDesignerUserInterface instance.
      //static void computeCarouselSlots(Phase currentPhase, S32 currentWeaponSlot,
      //   S32 slotsInUse, Phase phaseAtSlot[],
      //   S32 weaponSlotAtSlot[], S32 size);

};


class VehiclePreviewRenderer
{
   public:
      // Draw the floating preview panel on the right side of the screen.
      static void renderPreviewPanel(const VehicleDesign &preview, Phase designPhase, S32 activeWaponSlot);
      static S32 renderWeaponList(Phase designPhase, S32 activeWeaponSlot, S32 left, S32 cx, S32 y, const VehicleDesign &preview, S32 fontSz, S32 lineGap);

      // Draw the carousel position dots + arrows inside the preview panel.
      //static void renderCarouselDots(S32 cx, S32 y);

      // Draw combined effective vehicle stats (speed/reverse/accel/turn/fire-rate)
      // derived from the full current preview design.
      static void renderFullBuildStats(S32 cx, S32 y, const VehicleDesign &preview);

};


} /* namespace Zap */
#endif /* _UI_VEHICLE_DESIGNER_H_ */



