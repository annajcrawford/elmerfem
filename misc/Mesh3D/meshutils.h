#ifndef MESHUTILS_H
#define MESHUTILS_H

#include <math.h>
#include "meshtype.h"
#include "helpers.h"

class Meshutils
{
 public:
  Meshutils();
  ~Meshutils();

  void clearMesh(mesh_t *mesh);
  void findBoundaryElementEdges(mesh_t*);
  void findBoundaryElementParents(mesh_t*);
  void findBoundaryElementNormals(mesh_t*);
  void findSharpEdges(mesh_t*, double);
  int divideBoundaryBySharpEdges(mesh_t*);
  double* boundingBox(mesh_t*);
};
#endif // #ifndef MESHUTILS_H
