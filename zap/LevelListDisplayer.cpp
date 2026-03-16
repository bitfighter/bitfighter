//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelListDisplayer.h"

#include "DisplayManager.h"
#include "FontManager.h"
#include "Renderer.h"
#include "RenderUtils.h"
#include "UI.h"
#include "Colors.h"

namespace Zap
{

LevelListDisplayer::LevelListDisplayer()
{
   mLevelLoadDisplayFadeTimer.setPeriod(1000);
   mLevelLoadDisplay = true;
   mLevelLoadDisplayTotal = 0;
}


void LevelListDisplayer::idle(U32 timeDelta)
{
   if(mLevelLoadDisplayFadeTimer.update(timeDelta))
      clearLevelLoadDisplay();
}


void LevelListDisplayer::showLevelLoadDisplay(bool show, bool fade)
{
   mLevelLoadDisplay = show;

   if(!show)
   {
      if(fade)
         mLevelLoadDisplayFadeTimer.reset();
      else
         mLevelLoadDisplayFadeTimer.clear();
   }
}


void LevelListDisplayer::clearLevelLoadDisplay()
{
   mLevelLoadDisplayNames.clear();
   mLevelLoadDisplayTotal = 0;
}


void LevelListDisplayer::render() const
{
   if(mLevelLoadDisplay || mLevelLoadDisplayFadeTimer.getCurrent() > 0)
   {
      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         FontManager::pushFontContext(MenuContext);
         Renderer::get().setColor(Colors::white, (1.4f - ((F32) (mLevelLoadDisplayNames.size() - i) / 10.f)) *
                                        (mLevelLoadDisplay ? 1 : mLevelLoadDisplayFadeTimer.getFraction()) );
         drawStringf(100, DisplayManager::getScreenInfo()->getGameCanvasHeight() - /*vertMargin*/ 0 - (mLevelLoadDisplayNames.size() - i) * 20,
                     15, "%s", mLevelLoadDisplayNames[i].c_str());

         FontManager::popFontContext();
      }
   }
}


void LevelListDisplayer::addLevelName(const string &levelName)
{
   addProgressListItem("Loaded level " + levelName + "...");
}


void LevelListDisplayer::addProgressListItem(string item)
{
   static const S32 MaxItems = 15;

   mLevelLoadDisplayNames.push_back(item);

   mLevelLoadDisplayTotal++;

   if(mLevelLoadDisplayNames.size() > MaxItems)
      mLevelLoadDisplayNames.erase(0);
}

}
