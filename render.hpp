#ifndef hogtess_render_hpp_included__
#define hogtess_render_hpp_included__

//#include <vector>
//#include <set>

#include <GL/gl.h>
#include <GL/glu.h>

#include <QtGui>
#include <QGLWidget>
#include <QSize>


class RenderWidget : public QGLWidget
{
   Q_OBJECT

public:
   RenderWidget(   );

   virtual ~RenderWidget();

protected:

   void loadData();

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

//   GLUquadric* ball;
};


#endif // hogtess_render_hpp_included__
