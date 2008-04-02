#ifndef GLWIDGET_H
#define GLWIDGET_H

#define POINTLIST       2000
#define EDGELIST        2001
#define SURFACELIST     2002
#define SURFACEMESHLIST 2003
#define SHARPEDGELIST   2004

#include <QGLWidget>
#include <QHash>

#include "helpers.h"
#include "meshutils.h"

class list_t {
 public:
  int nature;        // PDE_UNKNOWN, PDE_BOUNDARY, PDE_BULK, ...
  int type;          // POINTLIST, EDGELIST, SURFACELIST, ...
  int index;         // Boundary condition as defined in input file
  GLuint object;     // GL list index as returned by glGenLists()
  int child;         // Index to the child list (-1 = no child)
  int parent;        // Index to the parent list (-1 = no parent)
  bool selected;     // Currently selected?
  bool visible;      // Currently visible?
};

class GLWidget : public QGLWidget
{
  Q_OBJECT
    
public:
  GLWidget(QWidget *parent = 0);
  ~GLWidget();
  
  QSize minimumSizeHint() const;
  QSize sizeHint() const;
  
  mesh_t *mesh;
  
  int lists;
  list_t *list;
  GLuint makeLists();
  void rebuildLists();
  void rebuildSurfaceLists(void);
  void rebuildEdgeLists(void);
  
  double drawTranslate[3];
  double drawScale;

  bool stateFlatShade;
  bool stateDrawSurfaceMesh;
  bool stateDrawSharpEdges;
  bool stateDrawSurfaceElements;
  bool stateDrawEdgeElements;
  bool stateDrawCoordinates;
  int currentlySelectedBody;
  bool ctrlPressed;
  bool shiftPressed;
  bool altPressed;
  bool bodyEditActive;

  QHash<int, int> boundaryMap;
  QHash<int, int> bodyMap;

public slots:

signals:
  void signalBoundarySelected(list_t*);

protected:
  void initializeGL();
  void paintGL();
  void resizeGL(int, int);
  
  void focusInEvent(QFocusEvent*);
  void mouseDoubleClickEvent(QMouseEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void keyPressEvent(QKeyEvent*);
  void keyReleaseEvent(QKeyEvent*);
  
private:
  Helpers *helpers;
  Meshutils *meshutils;
  
  GLdouble matrix[16];
  GLdouble invmatrix[16];
  void getMatrix();
  
  QPoint lastPos;
  QColor backgroundColor;
  
  GLuint generateSurfaceList(int, double, double, double);
  GLuint generateSurfaceMeshList(int, double, double, double);
  GLuint generateEdgeList(int, double, double, double);
  GLuint generateSharpEdgeList(double, double, double);
  
  GLUquadricObj *quadratic; 
  void drawCoordinates();

};

#endif
