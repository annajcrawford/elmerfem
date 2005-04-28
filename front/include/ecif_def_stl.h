/***********************************************************************
*
*       ELMER, A Computational Fluid Dynamics Program.
*
*       Copyright 1st April 1995 - , Center for Scientific Computing,
*                                    Finland.
*
*       All rights reserved. No part of this program may be used,
*       reproduced or transmitted in any form or by any means
*       without the written permission of CSC.
*
*                Address: Center for Scientific Computing
*                         Tietotie 6, P.O. BOX 405
*                         02101 Espoo, Finland
*                         Tel.     +358 0 457 2001
*                         Telefax: +358 0 457 2302
*                         EMail:   Jari.Jarvinen@csc.fi
************************************************************************/

/***********************************************************************
Program:    ELMER Front
Module:     ecif_def_stl.h
Language:   C++
Date:       01.10.98
Version:    1.00
Author(s):  Martti Verho
Revisions:

Abstract:   All STL-based container types are defined here.

************************************************************************/

#ifndef _ECIF_STL_
#define _ECIF_STL_

#include "ecif_def.h"

// STL object comparison functions
extern bool operator<(const Ids1& p1, const Ids1& p2);
extern bool operator==(const Ids1& p1, const Ids1& p2);

extern bool operator<(const Ids2& p1, const Ids2& p2);
extern bool operator==(const Ids2& p1, const Ids2& p2);

extern bool operator<(const Ids3& p1, const Ids3& p2);
extern bool operator==(const Ids3& p1, const Ids3& p2);



// New Standard library-stuff (VC++6.0, Latest in Unix etc)
#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

typedef std::basic_string<char> string;

//*** STL datastructures with names making their purpose a bit clearer (we hope).
typedef std::list<BodyElement*> BodyElementList;
typedef std::list<BoundaryPoint*> BoundaryPointList;
typedef std::list<BodyElement*> EdgeList;
typedef std::list<int> IdList;
typedef std::list<MeshCornerElement*> MeshCornerElementList;
typedef std::list<char*> NameList;
typedef std::list<Parameter*> ParameterList;
typedef std::list<GcPoint*> PointList;
typedef std::list<Point3*> Point3List;
typedef std::list<BodyElement*> VertexList;

typedef std::map<int, BodyElement*, std::less<int> > BodyElementTable;
typedef std::multimap<int, BodyElement*, std::less<int> > MultiBodyElementTable;
typedef std::map<int, BodyElementLoop*, std::less<int> > BodyElementLoopTable;
typedef std::map<int, BodyForce*, std::less<int> > BodyForceTable;
typedef std::map<int, Body*, std::less<int> > BodyTable;
typedef std::map<int, RGBfloat*, std::less<int> > ColorTable;
typedef std::map<string, Color4*, std::less<string> > RGBColorTable;
typedef std::map<string, char*, std::less<string> > MatcValueTable;
typedef std::map<int, IdList*, std::less<int> > IdListTable;
typedef std::map<int, int, std::less<int> > IdNumberTable;
typedef std::map<int, int, std::less<int> > IdTable;
typedef std::multimap<int, int, std::less<int> > MultiIdNumberTable;
typedef std::multimap<int, int, std::less<int> > MultiIdTable;
typedef std::map<int, char*, std::less<int> > NameTable;
typedef std::map<int, Parameter*, std::less<int> > ParameterTable;
typedef std::map<int, GcPoint*, std::less<int> > PointTable;
typedef std::map<GcPoint*, BodyElement*, std::less<GcPoint*> > Point2VertexTable;
typedef std::map<int, Process*, std::less<int> > ProcessTable;

typedef std::map<int, PointList*, std::less<int> > PointHashTable;

typedef std::vector<int> IdArray;
typedef std::vector<ModelObject*> ModelObjectArray;
typedef std::vector<BodyPair*> BodyPairArray;
typedef std::vector<colorIndices> ColorIndexArray;
typedef std::vector<double**> ParamVectorArray;
typedef std::vector<AdjacentHalf**> AdjacentPairArray;
typedef std::vector<SplitCombineInfo*> SplitCombineInfoArray;

typedef std::set<int, std::less<int> > IdsSet;
typedef std::set<Ids1, std::less<Ids1> > Ids1Set;
typedef std::set<Ids2, std::less<Ids2> > Ids2Set;
typedef std::set<Ids3, std::less<Ids3> > Ids3Set;
typedef std::set<string, std::less<string> > NameSet;

typedef std::stack <int  > IdsStack;
typedef std::stack <Ids1 > Ids1Stack;
typedef std::stack <Ids2 > Ids2Stack;

struct IgesDirectoryEntry;
typedef std::map<int, IgesDirectoryEntry*, std::less<int> > IgesDirectory;


extern int find2(Ids2Set& id_set, int key1);
extern int find3(Ids3Set& id_set, int key1, int key2);
extern void purgeNameTable(NameTable& table);
extern void purgeNameList(NameList& list);

extern const char* getMatcString(MatcValueTable& table, const char* key);
extern void storeMatcString(MatcValueTable& table, const char* key, const char* value);
extern void copyMatcValueTable(MatcValueTable& source, MatcValueTable& target);
extern void purgeMatcValueTable(MatcValueTable& table);


void reallocate_array(int old_size, int new_size, int*& array, int default_value);
void reallocate_array(int old_size, int new_size, char**& array, char* default_value);
void reallocate_array(int old_size, int new_size, int**& array, int* default_value);
void reallocate_array(int old_size, int new_size, BoundBox**& array);
void reallocate_array(int old_size, int new_size, BodyElementTable**& array);
void reallocate_array(int old_size, int new_size, IdList**& array);
void reallocate_array(int old_size, int new_size, IdArray**& array);

template <class T >
void reallocate_array_impl1(int old_size, int new_size, T*& array, T default_value);

template <class T >
void reallocate_array_impl2(int old_size, int new_size, T**& array, T* default_value);

template <class T >
void reallocate_array_impl3(int old_size, int new_size, T**& array);


#endif
