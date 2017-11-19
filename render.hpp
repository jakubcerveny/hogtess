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
   RenderWidget(int numElements, int polyOrder,
                int meshDim, double* meshCoefs[3],
                int slnDim, double* slnCoefs[3]);

   virtual ~RenderWidget();

protected:

   Program progSurface;
   GLuint vao, tex;

   void compileShaders();
   void loadData();
   void shapeInit(const Program &prog);

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

   bool rotating;
   bool scaling;
   bool translating;

   double rotateX, rotateY;
   double scale;
   double panX, panY;
};


#endif // hogtess_render_hpp_included__
