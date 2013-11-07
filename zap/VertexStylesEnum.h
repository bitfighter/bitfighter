//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _VERTEX_STYLES_ENUM_H_
#define _VERTEX_STYLES_ENUM_H_

namespace Zap
{     

enum VertexStyles
{
   SnappingVertex,         // Vertex that indicates snapping point
   HighlightedVertex,      // Highlighted vertex
   SelectedVertex,         // Vertex itself is selected
   SelectedItemVertex,     // Non-highlighted vertex of a selected item
   UnselectedItemVertex,   // Non-highlighted vertex of a non-selected item
};

};


#endif
