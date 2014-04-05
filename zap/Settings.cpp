//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Settings.h"

using namespace TNL;

namespace Zap
{

EnumParser<DisplayMode>        displayModeEnumParser;
EnumParser<ColorEntryMode>     colorEntryModeEnumParser;
EnumParser<YesNo>              yesNoEnumParser;
EnumParser<GoalZoneFlashStyle> goalZoneFlashEnumParser;
EnumParser<RelAbs>             relativeAbsoluteEnumParser;


class EnumInitializer
{
public:
   EnumInitializer() 
   {
#define DISPLAY_MODE_ITEM(value, name) displayModeEnumParser.addItem(name, value);
    DISPLAY_MODES_TABLE
#undef DISPLAY_MODE_ITEM

#define COLOR_ENTRY_MODE_ITEM(value, name) colorEntryModeEnumParser.addItem(name, value);
    COLOR_ENTRY_MODES_TABLE
#undef COLOR_ENTRY_MODE_ITEM

#define YES_NO_ITEM(value, name) yesNoEnumParser.addItem(name, value);
    YES_NO_TABLE
#undef YES_NO_ITEM

#define GOAL_ZONE_FLASH_ITEM(value, name) goalZoneFlashEnumParser.addItem(name, value);
    GOAL_ZONE_FLASH_TABLE
#undef GOAL_ZONE_FLASH_ITEM

#define RELATIVE_ABSOLUTE_ITEM(value, name) relativeAbsoluteEnumParser.addItem(name, value);
    RELATIVE_ABSOLUTE_TABLE
#undef RELATIVE_ABSOLUTE_ITEM
   }
};

static EnumInitializer enumInitializer;      // Static class only exists to run the intializer once


// Templated default - needs to be overriden
template<class DataType> DataType
Evaluator::fromString(const string &val) { TNLAssert(false, "Specialize me!"); return DataType(); }

// Specializations.
// NOTE: All template specializations must be declared in the namespace scope to be
// C++ compliant.  Shame on Visual Studio!
template<> string             Evaluator::fromString(const string &val) { return val;                                    }
template<> S32                Evaluator::fromString(const string &val) { return atoi(val.c_str());                      }
template<> U32                Evaluator::fromString(const string &val) { return atoi(val.c_str());                      }
template<> F32                Evaluator::fromString(const string &val) { return (F32)atof(val.c_str());                 }
template<> U16                Evaluator::fromString(const string &val) { return atoi(val.c_str());                      }
template<> DisplayMode        Evaluator::fromString(const string &val) { return displayModeEnumParser.getVal(val);      }
template<> YesNo              Evaluator::fromString(const string &val) { return yesNoEnumParser.getVal(val);            }
template<> RelAbs             Evaluator::fromString(const string &val) { return relativeAbsoluteEnumParser.getVal(val); }
template<> ColorEntryMode     Evaluator::fromString(const string &val) { return colorEntryModeEnumParser.getVal(val);   }
template<> GoalZoneFlashStyle Evaluator::fromString(const string &val) { return goalZoneFlashEnumParser.getVal(val);    }
template<> Color              Evaluator::fromString(const string &val) { return Color::iniValToColor(val);              }


// Convert various things to strings
string Evaluator::toString(const string &val)      { return val;                                    }
string Evaluator::toString(S32 val)                { return itos(val);                              }
string Evaluator::toString(U32 val)                { return itos(val);                              }
string Evaluator::toString(F32 val)                { return ftos(val);                              }
string Evaluator::toString(YesNo val)              { return yesNoEnumParser.getKey(val);            }
string Evaluator::toString(RelAbs val)             { return relativeAbsoluteEnumParser.getKey(val); }
string Evaluator::toString(DisplayMode val)        { return displayModeEnumParser.getKey(val);      }
string Evaluator::toString(ColorEntryMode val)     { return colorEntryModeEnumParser.getKey(val);   }
string Evaluator::toString(GoalZoneFlashStyle val) { return goalZoneFlashEnumParser.getKey(val);    }
string Evaluator::toString(const Color &color)     { return color.toHexStringForIni();              }

}

