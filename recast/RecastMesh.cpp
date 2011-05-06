//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "Recast.h"
#include "RecastAlloc.h"
#include "tnlAssert.h"
#include "tnlLog.h"

#include "../zap/Rect.h"     // For Rect
#include <map>                // For zonemap map
#include <vector>             // For piority list
#include <algorithm>          // For sorting the piority list


using namespace TNL;
using namespace std;

struct rcEdge
{
	unsigned short vert[2];
	unsigned short polyEdge[2];
	unsigned short poly[2];
};


static bool buildMeshAdjacency(unsigned short* polys, const int npolys,
                               unsigned short *adjaceny,
							          const int nverts, const int vertsPerPoly)
{
	// Based on code by Eric Lengyel from:
	// http://www.terathon.com/code/edges.php
	
	int maxEdgeCount = npolys*vertsPerPoly;
	unsigned short* firstEdge = (unsigned short*)rcAlloc(sizeof(unsigned short)*(nverts + maxEdgeCount), RC_ALLOC_TEMP);
	if (!firstEdge)      // Allocation went bad
		return false;
	unsigned short* nextEdge = firstEdge + nverts;
	int edgeCount = 0;
	
	rcEdge* edges = (rcEdge*)rcAlloc(sizeof(rcEdge)*maxEdgeCount, RC_ALLOC_TEMP);
	if (!edges)
	{
		rcFree(firstEdge);
		return false;
	}
	
	for (int i = 0; i < nverts; i++)
		firstEdge[i] = RC_MESH_NULL_IDX;
	
   // First process edges where 1st node < 2nd node
	for (int i = 0; i < npolys; ++i)
	{
		unsigned short* t = &polys[i*vertsPerPoly];

      // Skip "missing" polygons
      if(*t == U16_MAX)
         continue;

		for (int j = 0; j < vertsPerPoly; ++j)
		{
			unsigned short v0 = t[j];           // jth vert
			unsigned short v1 = (j+1 >= vertsPerPoly || t[j+1] == RC_MESH_NULL_IDX) ? t[0] : t[j+1];  // j+1th vert
			if (v0 < v1)      
			{
				rcEdge& edge = edges[edgeCount];       // edge connecting v0 and v1
				edge.vert[0] = v0;
				edge.vert[1] = v1;
				edge.poly[0] = (unsigned short)i;      // left poly
				edge.polyEdge[0] = (unsigned short)j;  // ??
				edge.poly[1] = (unsigned short)i;      // right poly, will be recalced later ??
				edge.polyEdge[1] = 0;                  // ??

				// Insert edge
				nextEdge[edgeCount] = firstEdge[v0];   // Next edge on the previous vert now points to the first edge for this vert

				firstEdge[v0] = (unsigned short)edgeCount;  // First edge of this vert 

				edgeCount++;                           // edgeCount never resets -- each edge gets a unique id
			}
		}
	}
	

   // Now process edges where 2nd node is > 1st node
	for (int i = 0; i < npolys; ++i)
	{
		unsigned short* t = &polys[i*vertsPerPoly];

      // Skip "missing" polygons
      if(*t == U16_MAX)
         continue;

		for (int j = 0; j < vertsPerPoly; ++j)
		{
			unsigned short v0 = t[j];
			unsigned short v1 = (j+1 >= vertsPerPoly || t[j+1] == RC_MESH_NULL_IDX) ? t[0] : t[j+1];
			if (v0 > v1)      
			{
				for (unsigned short e = firstEdge[v1]; e != RC_MESH_NULL_IDX; e = nextEdge[e])
				{
					rcEdge& edge = edges[e];
					if (edge.vert[1] == v0 && edge.poly[0] == edge.poly[1])
					{
						edge.poly[1] = (unsigned short)i;
						edge.polyEdge[1] = (unsigned short)j;
						break;
					}
				}
			}
		}
	}
	
	// Store adjacency
	for (int i = 0; i < edgeCount; ++i)
	{
		const rcEdge& e = edges[i];
		if (e.poly[0] != e.poly[1])      // Should normally be the case
		{
			unsigned short* p0 = &adjaceny[e.poly[0]*vertsPerPoly];      // l-poly
			unsigned short* p1 = &adjaceny[e.poly[1]*vertsPerPoly];      // r-poly

			//p0[e.polyEdge[0]] = e.poly[1];
			//p1[e.polyEdge[1]] = e.poly[0];
		}
	}
	
	rcFree(firstEdge);
	rcFree(edges);
	
	return true;
}


static const int VERTEX_BUCKET_COUNT = (1<<12);

inline int computeVertexHash(int x, int y)
{
	const unsigned int h1 = 0x8da6b343; // Large multiplicative constants;
	const unsigned int h2 = 0xd8163841; // here arbitrarily chosen primes
	unsigned int n = h1 * x + h2 * y;
	return (int)(n & (VERTEX_BUCKET_COUNT-1));
}

static unsigned short addVertex(unsigned short x, unsigned short y, 
								unsigned short* verts, int* firstVert, int* nextVert, int& nv)
{
	int bucket = computeVertexHash(x, y);
	int i = firstVert[bucket];

   // Try to reuse an existing vertex if there is one in our bucket
	while (i != -1)
	{
		const unsigned short* v = &verts[i*2];
		if (v[0] == x && v[1] == y)
      {
			return (unsigned short)i;     // Found one!  We'll use this one!
      }
		i = nextVert[i]; // next
	}
	
	// Could not find, create new.
	i = nv; nv++;
	unsigned short* v = &verts[i*2];
	v[0] = x;
	v[1] = y;
	nextVert[i] = firstVert[bucket];
	firstVert[bucket] = i;
	
	return (unsigned short)i;
}

inline int prev(int i, int n) { return i-1 >= 0 ? i-1 : n-1; }
inline int next(int i, int n) { return i+1 < n ? i+1 : 0; }

inline int area2(const int* a, const int* b, const int* c)
{
	return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]);
}

//	Exclusive or: true iff exactly one argument is true.
//	The arguments are negated to ensure that they are 0/1
//	values.  Then the bitwise Xor operator may apply.
//	(This idea is due to Michael Baldwin.)
inline bool xorb(bool x, bool y)
{
	return !x ^ !y;
}

// Returns true iff c is strictly to the left of the directed
// line through a to b.
inline bool left(const int* a, const int* b, const int* c)
{
	return area2(a, b, c) < 0;
}

inline bool leftOn(const int* a, const int* b, const int* c)
{
	return area2(a, b, c) <= 0;
}

inline bool collinear(const int* a, const int* b, const int* c)
{
	return area2(a, b, c) == 0;
}

//	Returns true iff ab properly intersects cd: they share
//	a point interior to both segments.  The properness of the
//	intersection is ensured by using strict leftness.
bool intersectProp(const int* a, const int* b, const int* c, const int* d)
{
	// Eliminate improper cases.
	if (collinear(a,b,c) || collinear(a,b,d) ||
		collinear(c,d,a) || collinear(c,d,b))
		return false;
	
	return xorb(left(a,b,c), left(a,b,d)) && xorb(left(c,d,a), left(c,d,b));
}

// Returns T iff (a,b,c) are collinear and point c lies 
// on the closed segement ab.
static bool between(const int* a, const int* b, const int* c)
{
	if (!collinear(a, b, c))
		return false;
	// If ab not vertical, check betweenness on x; else on y.
	if (a[0] != b[0])
		return	((a[0] <= c[0]) && (c[0] <= b[0])) || ((a[0] >= c[0]) && (c[0] >= b[0]));
	else
		return	((a[1] <= c[1]) && (c[1] <= b[1])) || ((a[1] >= c[1]) && (c[1] >= b[1]));
}

// Returns true iff segments ab and cd intersect, properly or improperly.
static bool intersect(const int* a, const int* b, const int* c, const int* d)
{
	return (intersectProp(a, b, c, d) ||
           between(a, b, c) || between(a, b, d) ||
		 	  between(c, d, a) || between(c, d, b));
}


static int countPolyVerts(const unsigned short* p, const int nvp)
{
	for (int i = 0; i < nvp; ++i)
		if (p[i] == RC_MESH_NULL_IDX)
			return i;
	return nvp;
}

inline bool uleft(const unsigned short* a, const unsigned short* b, const unsigned short* c)
{
	return ((int)b[0] - (int)a[0]) * ((int)c[1] - (int)a[1]) -
		    ((int)c[0] - (int)a[0]) * ((int)b[1] - (int)a[1]) >= 0;        // <= 0 for CW triangles, >= 0 for CCW triangles
}


// Does no merging, returns length^2 of segment that will be merged, or -1 if merge is invalid
static int getPolyMergeValue_work(unsigned short* pa, unsigned short* pb,
							             const unsigned short* verts, int& ea, int& eb,
							             const int na, const int nb)
{
	// Check if the polygons share an edge.
	ea = -1;
	eb = -1;
	
	for (int i = 0; i < na; ++i)
	{
		unsigned short va0 = pa[i];               // va0 --> 0th vertex of pa (polygon-a)
		unsigned short va1 = pa[(i+1) % na];      // va1 --> 1st vertex of pa (polygon-a)

      // Get the vertices in ascending order, will make comparing easier later
		if (va0 > va1)
			rcSwap(va0, va1);

		for (int j = 0; j < nb; ++j)
		{
			unsigned short vb0 = pb[j];            // vb0 --> 0th vertex of pb (polygon-b)
			unsigned short vb1 = pb[(j+1) % nb];   // vb1 --> 1st vertex of pb (polygon-b)

         // Get the vertices in ascending order
			if (vb0 > vb1)
				rcSwap(vb0, vb1);

			if (va0 == vb0 && va1 == vb1)    // Are the vertex pairs the same?  If so, we've found a common segment!
			{
				ea = i;     // The ith edge of a matches...
				eb = j;     // ...the jth edge of b.  These values will be returned to caller.
				break;
			}
		}
	}
	
	// No common edge, cannot merge.
	if (ea == -1 || eb == -1)
		return -1;
	
   // We can merge, and we know that ea and eb represent the same line on different triangles; ea, eb are "edge indexes"

	// Check to see if the merged polygon would be convex
	unsigned short va, vb, vc;
	
	va = pa[(ea+na-1) % na];      // vertex just prior to ea
	vb = pa[ea];                  // vertex at start of segment pointed to by ea 
	vc = pb[(eb+2) % nb];         // vertex following end of segment pointed to by eb

   // Do the test    (see http://critterai.org/nmgen_polygen for pictoral explanation)
	if (!uleft(&verts[va*2], &verts[vb*2], &verts[vc*2]))
		return -1;
	
   // And again...

	va = pb[(eb+nb-1) % nb];      // vertex just prior to eb
	vb = pb[eb];                  // vertex at start of eb
	vc = pa[(ea+2) % na];         // vertex following end of segment pointed to by ea

   // Do the test
	if (!uleft(&verts[va*2], &verts[vb*2], &verts[vc*2]))
		return -1;
	
   // Now we know that resulting polygon will be convex
   
	va = pa[ea];                  // vertex at beginning of ea
	vb = pa[(ea+1)%na];           // vertex at end of ea
	
	int dx = (int)verts[va*2+0] - (int)verts[vb*2+0];     // dx of edge ea
	int dy = (int)verts[va*2+1] - (int)verts[vb*2+1];     // dy of edge ea
	
	return dx*dx + dy*dy;         // length of edge being merged along
}


// Wraps getPolyMergeValue_work, slightly optimized for triangles
static int getTriMergeValue(unsigned short* pa, unsigned short* pb, const unsigned short* verts)
{
   int ea, eb;
   return getPolyMergeValue_work(pa, pb, verts, ea, eb, 3, 3);
}


// Wraps getPolyMergeValue_work, counts vertices and checks if merged poly would be too big
static int getPolyMergeValue(unsigned short* pa, unsigned short* pb,
							        const unsigned short* verts, int& ea, int& eb,
							        const int nvp)
{
	const int na = countPolyVerts(pa, nvp);      // na --> number of verts in a
	const int nb = countPolyVerts(pb, nvp);      // nb --> number of verts in b

   // If the merged polygon would be too big, do not merge.
	if (na+nb-2 > nvp)
		return -1;

   return getPolyMergeValue_work(pa, pb, verts, ea, eb, na, nb);
}
	

static void mergePolys(unsigned short* pa, unsigned short* pb, int ea, int eb,
					   unsigned short* tmp, const int nvp)
{
	const int na = countPolyVerts(pa, nvp);
	const int nb = countPolyVerts(pb, nvp);

	// Merge polygons
	memset(tmp, 0xff, sizeof(unsigned short)*nvp);     // Clear the temp poly
	int n = 0;
	// Add pa
	for (int i = 0; i < na-1; ++i)
		tmp[n++] = pa[(ea+1+i) % na];
	// Add pb
	for (int i = 0; i < nb-1; ++i)
		tmp[n++] = pb[(eb+1+i) % nb];
	
	memcpy(pa, tmp, sizeof(unsigned short)*nvp);
}


// Tracks which polys can be merged, and what their ranked priority is
struct mergePriority {
   int i, j, priority;
   mergePriority(int i, int j, int priority) { this->i = i; this->j = j, this->priority = priority; }    // Constructor
};


// Sorts in ascending order; highest priority items will be at the end of the list
bool prioritySort(const mergePriority& a, const mergePriority& b)
{
   return a.priority < b.priority;
}


// nvp = max number of points per polygon

bool rcBuildPolyMesh(int nvp, int* verts, int vertCount, int *tris, int ntris, rcPolyMesh& mesh)
{
	int maxVertices = vertCount;
	int maxTris = ntris;    // Was vertCount - 2
	
	mesh.verts = (unsigned short*)rcAlloc(sizeof(unsigned short)*maxVertices*2, RC_ALLOC_PERM);
	if (!mesh.verts)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'mesh.verts' (%d).", maxVertices);
		return false;
	}
	mesh.polys = (unsigned short*)rcAlloc(sizeof(unsigned short)*(ntris + 1)*nvp, RC_ALLOC_PERM);
	if (!mesh.polys)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'mesh.polys' (%d).", maxTris*nvp*2*2);
		return false;
	}

   mesh.adjacency = (unsigned short*)rcAlloc(sizeof(unsigned short)*maxTris*nvp*2*2, RC_ALLOC_PERM);
	if (!mesh.adjacency)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'mesh.adjacency' (%d).", maxTris*nvp*2*2);
		return false;
	}


	mesh.nverts = 0;
	mesh.npolys = 0;
	mesh.nvp = nvp;
	mesh.maxpolys = maxTris;
	
	memset(mesh.verts, 0, sizeof(unsigned short)*maxVertices*2);
	//memset(mesh.polys, 0xff, sizeof(unsigned short)*maxTris*nvp*2);
	
	rcScopedDelete<int> nextVert = (int*)rcAlloc(sizeof(int)*maxVertices, RC_ALLOC_TEMP);
	if (!nextVert)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'nextVert' (%d).", maxVertices);
		return false;
	}
	memset(nextVert, 0, sizeof(int)*maxVertices);
	
	rcScopedDelete<int> firstVert = (int*)rcAlloc(sizeof(int)*VERTEX_BUCKET_COUNT, RC_ALLOC_TEMP);
	if (!firstVert)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'firstVert' (%d).", VERTEX_BUCKET_COUNT);
		return false;
	}
	for (int i = 0; i < VERTEX_BUCKET_COUNT; ++i)
		firstVert[i] = -1;
	
	rcScopedDelete<int> indices = (int*)rcAlloc(sizeof(int)*vertCount, RC_ALLOC_TEMP);
	if (!indices)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'indices' (%d).", vertCount);
		return false;
	}

   // + 1 reserves a bit of space at the end for a temp workspace
	//rcScopedDelete<unsigned short> polys = (unsigned short*)rcAlloc(sizeof(unsigned short)*(ntris + 1)*nvp, RC_ALLOC_TEMP);
	unsigned short *polys = polys = mesh.polys;
   //	rcScopedDelete<unsigned short> tmpPoly = (unsigned short*)rcAlloc(sizeof(unsigned short)*nvp, RC_ALLOC_TEMP);
	if (!polys)
	{
		logprintf(LogConsumer::LogError, "rcBuildPolyMesh: Out of memory 'polys' (%d).", ntris * nvp);
		return false;
	}
	unsigned short* tmpPoly = &polys[ntris * nvp];     // Use a little reserved space at the end for a temp poly


	for (int j = 0; j < vertCount; ++j)
		indices[j] = j;
			
	// Add vertices that we were handed to our internal index system, merging any duplicates that we find
	for (int j = 0; j < vertCount; ++j)    // Iterate over each vertex
	{
		const int* v = &verts[j*2];         // v now points to the 2-integer block of memory representing a single vertex (x,y)
		indices[j] = addVertex((unsigned short)v[0], (unsigned short)v[1], mesh.verts, firstVert, nextVert, mesh.nverts);
	}


   // NOTE: firstVert is an indexed list of the first vertex in each triangle
   //       nextVert is the same for the second vertex
	// Build initial polygons from triangles -- one triangle goes into each polygon
	int npolys = 0;
	memset(polys, 0xff, ntris * nvp * sizeof(unsigned short));     // Max coord = U16_MAX

	for (int j = 0; j < ntris; ++j)
	{
		int* t = &tris[j*3];

		if (indices[t[0]] != indices[t[1]] && indices[t[0]] != indices[t[2]] && indices[t[1]] != indices[t[2]])           // Ensure no dupes
		{
			polys[npolys*nvp+0] = (unsigned short)indices[t[0]];
			polys[npolys*nvp+1] = (unsigned short)indices[t[1]];
			polys[npolys*nvp+2] = (unsigned short)indices[t[2]];

			npolys++;
		}
	}


   // Calc bounds of each triangle so we can more quickly eliminate non-neighboring zone pairs
   Vector<Zap::Rect> polyExtents;
   
   for(int i = 0; i < npolys; i++)
   {
      int minx = 0xFFFF, miny = 0xFFFF, maxx = 0, maxy = 0;
      unsigned short* p = &polys[i*nvp];
      
      for (int j = 0; j < 3; j++)               // Triangles generally have 3 verts
      {
         int x = (int)mesh.verts[p[j]*2+0];
         int y = (int)mesh.verts[p[j]*2+1];

         if(x < minx) minx = x;
         if(y < miny) miny = y;
         if(x > maxx) maxx = x;
         if(y > maxy) maxy = y;
      }

      polyExtents.push_back(Zap::Rect(F32(minx), F32(miny), F32(maxx), F32(maxy)));
   }
  

   vector<mergePriority> mergePriorityList;     // Ranked list of which zones we'll merge, and in which order
   map<int,int> zoneRemap;                      // For keeping track of zones whose numbers change

   for (int i = 0; i < npolys-1; i++)
	{
		unsigned short* pi = &polys[i*nvp];       // pi --> pointer to the ith poly

		for (int j = i + 1; j < npolys; j++)
      {
         if(!polyExtents[i].intersectsOrBorders(polyExtents[j]))     // If bounding boxes don't overlap, move on
            continue;

         unsigned short* pj = &polys[j*nvp];    // pj --> pointer to the jth poly

         int len = getTriMergeValue(pi, pj, mesh.verts);

         if(len > 0)
            mergePriorityList.push_back(mergePriority(i,j,len));
      }
   }

   // Sort mergePriorityList so the highest priority merges come at the end; then we can just work backwards
   sort(mergePriorityList.begin(), mergePriorityList.end(), prioritySort);   


	// Merge adjacent polygons
	if (nvp > 3)      // If nvp == 3, we're already there; each polygon will be a copy of an input triangle 
	{
		for(;;)
		{

         if(mergePriorityList.size() == 0)                  // No more mergeable polygons... time to stop
            break;

			// Find best polygons to merge
         mergePriority mergees = mergePriorityList.back();  // Retrieve last element -- this will be our next merge candidate
         mergePriorityList.pop_back();                      // Remove last element from list

 
         int i = mergees.i;
         int j = mergees.j;

         // Figure out if polys i & j have been remapped elsewhere...
         while(zoneRemap[i] > 0)
            i = zoneRemap[i] - 1;

         while(zoneRemap[j] > 0)
            j = zoneRemap[j] - 1;

         unsigned short* pa = &polys[i * nvp];      
         unsigned short* pb = &polys[j * nvp];

         int ea, eb;
         int len = getPolyMergeValue(pa, pb, mesh.verts, ea, eb, nvp);

         if(len > 0)      // Make sure poly is still mergeable -- previous mergings may have rendered this pair invalid
         {
				mergePolys(pa, pb, ea, eb, tmpPoly, nvp);
				memset(pb, 0xffff, sizeof(unsigned short)*nvp);    // Erase mergee
            zoneRemap[j] = i + 1;                              // Record that zone j is now part of zone i
            npolys--;
         }
		}
	}

   // Why aren't we just building this in mesh.polys... then we can skip this copy!
   //memcpy(mesh.polys, polys, ntris*nvp*sizeof(unsigned short));
   mesh.npolys = ntris;


	if (mesh.nverts > 0xffff)
	{
		logprintf(LogConsumer::LogError, "rcMergePolyMeshes: The resulting mesh has too many vertices %d (max %d). Data can be corrupted.", mesh.nverts, 0xffff);
	}
	if (mesh.npolys > 0xffff)
	{
		logprintf(LogConsumer::LogError, "rcMergePolyMeshes: The resulting mesh has too many polygons %d (max %d). Data can be corrupted.", mesh.npolys, 0xffff);
	}
	
	return true;
}
