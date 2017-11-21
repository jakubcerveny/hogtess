#ifndef hogtess_render_hpp_included__
#define hogtess_render_hpp_included__

#include <GL/gl.h>
#include <GL/glu.h>

#include <QtGui>
#include <QGLWidget>
#include <QSize>

#include <glm/fwd.hpp>

#include "shader.hpp"


class RenderWidget : public QGLWidget
{
   Q_OBJECT

public:
   RenderWidget(const QGLFormat &format,
                int numElements, int polyOrder, const double* nodes,
                int meshDim, const double* const* meshCoefs,
                int slnDim, const double* const* slnCoefs);

   virtual ~RenderWidget();

protected:

   Program progSurface;
   GLuint vao, tex;

   void compileShaders();
   void shapeInit(const Program &prog);

   virtual void initializeGL();
   virtual void resizeGL(int width, int height);
   virtual void paintGL();

   virtual void mousePressEvent(QMouseEvent *event);
   virtual void mouseMoveEvent(QMouseEvent *event);
   virtual void wheelEvent(QWheelEvent *event);
   virtual void keyPressEvent(QKeyEvent * event);

   int numElements, polyOrder;
   const double *nodes;

   int meshDim, slnDim;
   const double* const* meshCoefs;
   const double* const* slnCoefs;

   QSize curSize;
   double aspect;
   QPoint lastPos;

   bool rotating;
   bool scaling;
   bool translating;

   double rotateX, rotateY;
   double scale;
   double panX, panY;

   bool wireframe;
};


#endif // hogtess_render_hpp_included__
