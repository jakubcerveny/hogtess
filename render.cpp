#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <vector>

#include <QString>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "render.hpp"

#include "shaders/surface.glsl.hpp"

const double PanSpeed = 0.005;
const double RotateSpeed = 0.4;
//const double BallRadius = 0.005;


RenderWidget::RenderWidget()
  : QGLWidget((QWidget*) 0)

  , rotating(false)
  , scaling(false)
  , translating(false)

  , rotateX(0.), rotateY(0.)
  , scale(0.)
  , panX(0.), panY(0.)
{
   loadData();
//   ball = gluNewQuadric();
   grabKeyboard();
}

RenderWidget::~RenderWidget()
{
//   gluDeleteQuadric(ball);
}

void RenderWidget::loadData()
{
}

void RenderWidget::initShaders()
{
   progSurface.link(
       VertexShader(shaders::surface),
       TessControlShader(shaders::surface),
       TessEvalShader(shaders::surface),
       FragmentShader(shaders::surface));
}

void RenderWidget::initializeGL()
{
   std::cout << "OpenGL version: " << glGetString(GL_VERSION)
             << ", renderer: " << glGetString(GL_RENDERER) << std::endl;

   GLint major, minor;
   glGetIntegerv(GL_MAJOR_VERSION, &major);
   glGetIntegerv(GL_MINOR_VERSION, &minor);

   if (major < 4) {
      throw std::runtime_error(
         "OpenGL version 4.0 or higher required to run this program.");
   }

   initShaders();

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   static glm::vec3 vertices[4] = {
      {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}
   };

   glGenBuffers(1, &buffer);
   glBindBuffer(GL_ARRAY_BUFFER, buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void RenderWidget::resizeGL(int width, int height)
{
   glViewport(0, 0, width, height);
   curSize = QSize(width, height);
   aspect = (double) width / height;
}

void RenderWidget::paintGL()
{
   glClearColor(1, 1, 1, 1);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glm::mat4 projection = glm::perspective(45.0, aspect, 0.001, 100.0);

   glm::mat4 modelView(1.0);
   modelView = glm::translate(modelView, glm::vec3(PanSpeed*panX, -PanSpeed*panY, -3));
   double s = std::pow(1.01, -scale);
   modelView = glm::scale(modelView, glm::vec3(s, s, s));
   modelView = glm::translate(modelView, glm::vec3(-0.5, -0.5, -0.5));

   glm::mat4 MVP = projection * modelView;

/*   glLoadIdentity();
   glTranslated(PanSpeed*panX, -PanSpeed*panY, -3);
   glRotated(RotateSpeed*rotateY, 1, 0, 0);
   glRotated(RotateSpeed*rotateX, 0, 1, 0);
   double s = std::pow(1.01, -scale);
   glScaled(s, s, s);
   glRotated(-90, 1, 0, 0);
   glTranslated(-0.5, -0.5, -0.5);*/

   progSurface.use();

   glUniformMatrix4fv(progSurface.uniform("MVP"), 1, GL_FALSE,
                      glm::value_ptr(MVP));

   GLint aPosition(progSurface.attribute("aPosition"));
   glEnableVertexAttribArray(aPosition);
   glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE,
                         sizeof(glm::vec3), (const void*) 0);

   glBindBuffer(GL_ARRAY_BUFFER, buffer);
   glDrawArrays(GL_QUADS, 0, 4);

   //glPatchParameteri(GL_PATCH_VERTICES, 4);


   // status
/*   glColor3f(0, 0, 0);
   renderText(3, curSize.height()-3,
              QString().sprintf("File: %s, Element: %d",
                                fileNames[curFile].c_str(), curElement));*/
}

void RenderWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void RenderWidget::mouseMoveEvent(QMouseEvent *event)
{
    double deltaX(event->x() - lastPos.x());
    double deltaY(event->y() - lastPos.y());

    bool leftButton();
    bool rightButton(event->buttons() & Qt::RightButton);

    if (event->buttons() & Qt::LeftButton)
    {
       rotateX += deltaX;
       rotateY += deltaY;
    }
    else if (event->buttons() & Qt::RightButton)
    {
       scale += deltaY;
    }
    else if (event->buttons() & Qt::MiddleButton)
    {
       panX += deltaX;
       panY += deltaY;
    }

    lastPos = event->pos();
    updateGL();
}

void RenderWidget::wheelEvent(QWheelEvent *event)
{
//    position(2) *= pow(1.04, (float) -event->delta() / 120);
    updateGL();
}

void RenderWidget::keyPressEvent(QKeyEvent * event)
{
   switch (event->key())
   {
   case Qt::Key_Q:
      parentWidget()->close();
      break;

   case Qt::Key_Left:
      break;

   case Qt::Key_Right:
      break;

   case Qt::Key_Minus:
      break;

   case Qt::Key_Plus:
      break;

   case Qt::Key_I:
      break;
   }
   updateGL();
}

