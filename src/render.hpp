#ifndef hogtess_render_hpp_included__
#define hogtess_render_hpp_included__

#include <GL/gl.h>
#include <GL/glu.h>

#include <QtGui>
#include <QGLWidget>
#include <QSize>

#include "input/input.hpp" // NOTE: RenderWidget knows nothing about MFEM
#include "surface/surface.hpp"
#include "shader.hpp"


/** High order FE solution visualization class.
 */
class RenderWidget : public QGLWidget
{
   Q_OBJECT

public:
   RenderWidget(const QGLFormat &format,
                const Solution &solution,
                const double *nodalPoints,
                SurfaceCoefs &surfaceCoefs,
                VolumeCoefs &volumeCoefs);

   virtual ~RenderWidget() {}

protected:
   const Solution &solution;
   SurfaceCoefs &surfaceCoefs;
   VolumeCoefs &volumeCoefs;

   SurfaceMesh surfaceMesh;

   void updateCoefs();
   void updateMeshes();

   virtual void initializeGL();
   virtual void resizeGL(int width, int height);
   virtual void paintGL();

   virtual void mousePressEvent(QMouseEvent *event);
   virtual void mouseMoveEvent(QMouseEvent *event);
   virtual void wheelEvent(QWheelEvent *event);
   virtual void keyPressEvent(QKeyEvent * event);

   QSize curSize;
   double aspect;
   QPoint lastPos;

   double rotateX, rotateY;
   double zoom;
   double panX, panY;

   int tessLevel;
   bool wireframe, lines;
};


#endif // hogtess_render_hpp_included__
