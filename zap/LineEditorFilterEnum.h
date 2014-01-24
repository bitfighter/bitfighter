//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LINE_EDITOR_FILTER_ENUM_H_
#define _LINE_EDITOR_FILTER_ENUM_H_

namespace Zap
{
   enum LineEditorFilter {
      allAsciiFilter,          // any ascii character
      digitsOnlyFilter,        // 0-9
      numericFilter,           // 0-9, -, .
      fileNameFilter,          // Legal chars in a filename; all but \/:*?"<>|
      nickNameFilter           // No "s, and don't let name start with spaces
   };
};

#endif 
