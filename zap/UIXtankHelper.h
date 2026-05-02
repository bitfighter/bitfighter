//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_XTANK_HELPER_H_
#define _UI_XTANK_HELPER_H_

#include "helperMenu.h"
#include "XtankShape.h"    // for XtankDesign, XtankBody, XtankWeapon
#include "tnlVector.h"
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
   static const S32 MaxTableColumns = 12;
   const char *cells[MaxTableColumns];
   bool highlighted;       // true = force highlighted row color
};


struct TableColumn
{
   const char *header;
   ColAlignmemt headerAlignment;
   ColAlignmemt dataAlignment;
   S32 headerNudge; // Use this to make bespoke adjustments to the header position
};


class ComponentInfo
{
private:
   bool mComputed = false;
   void computeColWidths(S32 fontSize);

   S32 mColHeaderWidths[TableRow::MaxTableColumns];
   S32 mColDataWidths[TableRow::MaxTableColumns];
   S32 mColTotalWidths[TableRow::MaxTableColumns];

   virtual void fillRows() = 0;


protected:
   static const S32 COL_LEN = 24;

public:
   virtual S32 getColCount() const = 0;
   virtual S32 getRowCount() const = 0;
   virtual TableColumn *getColumns() = 0;
   virtual TableRow *getRows() = 0;

   S32 render(S32 left, S32 top, S32 fontSize, S32 rowGap, S32 highlightedRow);
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


class BodyInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 9;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",           ALIGN_LEFT,   ALIGN_CENTER, 0 },
      { "Body",          ALIGN_LEFT,   ALIGN_LEFT,   0 },
      { "Weight",        ALIGN_CENTER, ALIGN_RIGHT,  6 },
      { "Weight\nLimit", ALIGN_CENTER, ALIGN_RIGHT,  6 },
      { "Avail.\nSpace", ALIGN_CENTER, ALIGN_RIGHT,  6 },
      { "Drag",          ALIGN_CENTER, ALIGN_RIGHT,  0 },
      { "Handl-\ning",   ALIGN_CENTER, ALIGN_CENTER, 0 },
      { "Turrets",       ALIGN_CENTER, ALIGN_CENTER, 0 },
      { "Cost",          ALIGN_CENTER, ALIGN_RIGHT,  4 },
   };
   TableRow rows[VehicleBodyCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return VehicleBodyCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class ArmorInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 6;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",    ALIGN_LEFT,   ALIGN_CENTER, 0 },
      { "Armor",  ALIGN_LEFT,   ALIGN_LEFT,   0 },
      { "Class",  ALIGN_LEFT,   ALIGN_LEFT,   0 },
      { "Weight", ALIGN_CENTER, ALIGN_CENTER, 0 },
      { "Space",  ALIGN_CENTER, ALIGN_CENTER, 0 },
      { "Cost",   ALIGN_CENTER, ALIGN_RIGHT,  5 },
   };
   TableRow rows[XtankArmorCount];
   S32 getColCount() const override { return colCount; }

   S32 getRowCount() const override { return XtankArmorCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class EngineInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 8;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",         ALIGN_LEFT,   ALIGN_CENTER, 0 },
      { "Engine",      ALIGN_LEFT,   ALIGN_LEFT,   0 },
      { "Power",       ALIGN_CENTER, ALIGN_RIGHT,  10 },
      { "Weight",      ALIGN_CENTER, ALIGN_RIGHT,  8 },
      { "Space",       ALIGN_CENTER, ALIGN_RIGHT,  8 },
      { "Fuel\nCost",  ALIGN_CENTER, ALIGN_CENTER, 0 },
      { "Fuel\nCap.",  ALIGN_CENTER, ALIGN_RIGHT,  8 },
      { "Cost",        ALIGN_CENTER, ALIGN_RIGHT,  8 },
   };
   TableRow rows[XtankEngineCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankEngineCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class TreadInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",      ALIGN_LEFT,   ALIGN_CENTER, 0 },
      { "Tread",    ALIGN_LEFT,   ALIGN_LEFT,   0 },
      { "Friction", ALIGN_CENTER, ALIGN_RIGHT,  0 },
      { "Cost",     ALIGN_CENTER, ALIGN_RIGHT,  6 },
   };
   TableRow rows[XtankTreadCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankTreadCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class SuspensionInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",        ALIGN_LEFT,   ALIGN_CENTER, 0 },
      { "Suspension", ALIGN_LEFT,   ALIGN_LEFT,   0 },
      { "Handling",   ALIGN_CENTER, ALIGN_CENTER, 3 },
      { "Cost",       ALIGN_CENTER, ALIGN_RIGHT,  6 },
   };
   TableRow rows[XtankSuspensionCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankSuspensionCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class BumperInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",    ALIGN_LEFT, ALIGN_CENTER,  0 },
      { "Bumper", ALIGN_LEFT, ALIGN_LEFT,    0 },
      { "Bounce", ALIGN_CENTER, ALIGN_RIGHT, 2 },
      { "Cost",   ALIGN_RIGHT, ALIGN_RIGHT,  2 },
   };
   TableRow rows[XtankBumperCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return XtankBumperCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class HeatSinkInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 4;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Heat\nSinks",   ALIGN_LEFT, ALIGN_CENTER,  0 },
      { "Firing\nBonus", ALIGN_CENTER, ALIGN_RIGHT, 6 },
      { "Weight",        ALIGN_CENTER, ALIGN_RIGHT, 6 },
      { "Cost",          ALIGN_CENTER, ALIGN_RIGHT, 6 },
   };
   TableRow rows[1];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return 1; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


class ArmorAllocationInfo : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 colCount = 3;
   COL_NUM_ASSERT;

      TableColumn columns[colCount] =
   {
      { "Key",   ALIGN_LEFT, ALIGN_CENTER,   0 },
      { "Side",  ALIGN_LEFT, ALIGN_LEFT,     0 },
      { "Units", ALIGN_CENTER, ALIGN_CENTER, 0 },
   };
   TableRow rows[VehicleSidesCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return VehicleSidesCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }

   void updateArmorAllocationMenuItems(XtankArmor armorType, const S32 *armor);
};


// WeaponInfo: row 0 = "None", rows 1..XtankWeapon::COUNT = actual weapons.
class WeaponInfo2 : public ComponentInfo
{
private:
   void fillRows() override;

public:
   static const S32 rowCount = (S32)XtankWeapon::COUNT + 1;
   static const S32 colCount = 5;
   COL_NUM_ASSERT;


   TableColumn columns[colCount] =
   {
      { "Key",    ALIGN_LEFT, ALIGN_CENTER,  0 },
      { "Weapon", ALIGN_LEFT, ALIGN_LEFT,    0 },
      { "Delay",  ALIGN_CENTER, ALIGN_RIGHT, 6 },
      { "Weight", ALIGN_CENTER, ALIGN_RIGHT, 10 },
      { "Cost",   ALIGN_RIGHT, ALIGN_RIGHT,  0 },
   };
   TableRow rows[rowCount];

   S32 getColCount() const override { return colCount; }
   S32 getRowCount() const override { return rowCount; }
   TableColumn *getColumns() override { return columns; }
   TableRow *getRows() override { return rows; }
};


struct LabelWidth
{
   const char *label;
   S32 width;
};


class UIXtankHelper : public HelperMenu
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
      ArmorAllocationInfo mArmorAllocationInfo;

      // Holds the design being built during the selection process.
      XtankDesign mDesignInProgress;

      // Snapshot of the design when the helper was opened — restored on ESC.
      XtankDesign mOriginalDesign;

      // Current phase (see phase constants above).
      Phase mPhase;

	  S32 mWeaponSlot;  // Currently active weapon slot in weapon assignment phase

      // Pre-built overlay item arrays — rebuilt in onActivated().
      Vector<OverlayMenuItem> mBodyItems;
      Vector<OverlayMenuItem> mEngineItems;
      Vector<OverlayMenuItem> mTreadItems;
      Vector<OverlayMenuItem> mArmorItems;
      Vector<OverlayMenuItem> mArmorSidesItems;  // 6 items: Front/Back/Left/Right/Top/Bottom
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

      // Carousel transition animation state.
      Timer mTransitionTimer;     // Tracks transition progress (0-1)
      Phase mTransitionFromPhase;   // Previous phase before transition (-1 = no transition)
      bool mTransitioningForward;    // true = forward, false = backward

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


      // Draw the tab bar along the bottom and the single active panel above it.
      void renderFloatingMenus();

      // Draw the bottom tab strip.
      void renderTabBar(F32 t);

      // Returns the tab label for a given phase.
      LabelWidth getTabLabel(Phase phase) const;

      // Draw a single panel
      // centerFraction: 0.0=fully adjacent/background, 1.0=fully center/active
      void renderCard(S32 left, S32 top, S32 right, S32 bot, Phase phase, F32 centerFraction);
      void renderItemTable(const Vector<OverlayMenuItem> *items, S32 top, S32 left, bool isActive, Phase phase, S32 highlightedIndex, F32 grayLevel);
      void renderHeatSinkPanel(S32 top, S32 left, F32 alpha, S32 colTwoX);


      // Draw the title of the Weapons panel
      void renderWeaponPanelTitle(S32 titlex, S32 titley, S32 right, S32 size, bool isCenter);

      ComponentInfo *getCurrentComponentInfo(bool isCenter);


      // Returns the item vector for a given phase.
      const Vector<OverlayMenuItem> *getItemsForPhase(Phase phase) const;

      // Returns the item count for the current phase (used by UP/DOWN cycling).
      S32 currentPhaseItemCount();

      void renderArmorStats(S32 left, S32 y, S32 fontSize, F32 alpha) const;
      void renderSpecialsStats(S32 left, S32 y, F32 alpha) const;

   public:
      UIXtankHelper();
      virtual ~UIXtankHelper();

      HelperMenu::HelperMenuType getType();

      void onActivated();
      void render();
      void idle(U32 delta); // Update transition animation
      bool processInputCode(InputCode inputCode);

      void activateHelp(UIManager *uiManager);

      // Fills phaseAtSlot[] and weaponSlotAtSlot[] for `size` display slots centered on
      // the current phase/weapon-slot, using a linear clamped virtual sequence.
      // Exposed as public/static so it can be tested without a live UIXtankHelper instance.
      //static void computeCarouselSlots(Phase currentPhase, S32 currentWeaponSlot,
      //   S32 slotsInUse, Phase phaseAtSlot[],
      //   S32 weaponSlotAtSlot[], S32 size);

};


class VehiclePreviewRenderer
{
   public:
      // Draw the floating preview panel on the right side of the screen.
      static void renderPreviewPanel(const XtankDesign &preview, Phase designPhase, S32 activeWaponSlot);
      static S32 renderWeaponList(Phase designPhase, S32 activeWeaponSlot, S32 left, S32 cx, S32 y, const XtankDesign &preview, S32 fontSz, S32 lineGap);

      // Draw the carousel position dots + arrows inside the preview panel.
      //static void renderCarouselDots(S32 cx, S32 y);

      // Draw combined effective vehicle stats (speed/reverse/accel/turn/fire-rate)
      // derived from the full current preview design.
      static void renderFullBuildStats(S32 cx, S32 y, const XtankDesign &preview);

};


} /* namespace Zap */
#endif /* _UI_XTANK_HELPER_H_ */
