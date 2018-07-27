#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <vector>

#include <QString>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render.hpp"
#include "utility.hpp"

const double PanSpeed = 0.005;
const double RotateSpeed = 0.4;


RenderWidget::RenderWidget(const QGLFormat &format,
                           const Solution &solution,
                           const double *nodalPoints,
                           SurfaceCoefs &surfaceCoefs,
                           VolumeCoefs &volumeCoefs)

   : QGLWidget(format, (QWidget*) 0)

   , solution(solution)
   , surfaceCoefs(surfaceCoefs)
   , volumeCoefs(volumeCoefs)

   , surfaceMesh(nodalPoints)

   , rotating(false)
   , scaling(false)
   , translating(false)

   , rotateX(0.), rotateY(0.)
   , scale(1.0)
   , panX(0.), panY(0.)

   , wireframe(true)
   , tessLevel(8)
{
   grabKeyboard();
}

RenderWidget::~RenderWidget()
{
}

void RenderWidget::initializeGL()
{
   std::cout << "OpenGL version: " << glGetString(GL_VERSION)
             << ", renderer: " << glGetString(GL_RENDERER) << std::endl;

   GLint major, minor;
   glGetIntegerv(GL_MAJOR_VERSION, &major);
   glGetIntegerv(GL_MINOR_VERSION, &minor);

   if (major*10 + minor < 43) {
      throw std::runtime_error(
         "OpenGL version 4.3 or higher is required to run this program.");
   }

   surfaceMesh.initializeGL(solution.order());

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_MULTISAMPLE);

   updateCoefs();
   updateMeshes();
}

void RenderWidget::updateCoefs()
{
   surfaceCoefs.extract(solution);
}

void RenderWidget::updateMeshes()
{
   surfaceMesh.tesselate(surfaceCoefs, tessLevel);
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
   glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

   // setup the model-view-projection transform
   glm::mat4 proj = glm::perspective(glm::radians(45.0), aspect, 0.001, 100.0);
   glm::mat4 view(1.0);
   view = glm::translate(view, glm::vec3(0, 0, -1.5));
   view = glm::scale(view, glm::vec3(scale, scale, scale));
   view = glm::rotate(view, glm::radians(rotateX), glm::vec3(1, 0, 0));
   view = glm::rotate(view, glm::radians(rotateY), glm::vec3(0, 1, 0));
   glm::mat4 MVP = proj * view;

   // draw tesselated surface
   surfaceMesh.draw(MVP, true);
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
       rotateX += deltaY;
       rotateY += deltaX;
    }
    else if (event->buttons() & Qt::RightButton)
    {
       //scale += deltaY;
       scale -= 0.002 * deltaY;
       scale = std::max(scale, 0.0001f);
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
         tessLevel--;
         updateMeshes();
         break;

      case Qt::Key_Plus:
         tessLevel++;
         updateMeshes();
         break;

      case Qt::Key_W:
         wireframe = !wireframe;
         break;
   }
   updateGL();
}

