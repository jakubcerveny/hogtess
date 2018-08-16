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


RenderWidget::RenderWidget(const QGLFormat &format,
                           const Solution &solution,
                           SurfaceCoefs &surfaceCoefs,
                           VolumeCoefs &volumeCoefs)

   : QGLWidget(format, (QWidget*) 0)

   , solution(solution)
   , surfaceCoefs(surfaceCoefs)
   , volumeCoefs(volumeCoefs)

   , surfaceMesh(solution)
   , cutPlaneMesh(solution)

   , rotateX(0.), rotateY(0.)
   , zoom(0.)
   , panX(0.), panY(0.)

   , tessLevel(8)
   , wireframe(false)
   , lines(true)

   , clipMode(0)
   , clipX(0), clipY(0), clipZ(0)
   , clipPlane(1, 0, 0, 0)
{
   grabKeyboard();
}


void RenderWidget::initializeGL()
{
   std::cout << "OpenGL version: " << glGetString(GL_VERSION)
             << ", renderer: " << glGetString(GL_RENDERER) << std::endl;

   GLint major, minor;
   glGetIntegerv(GL_MAJOR_VERSION, &major);
   glGetIntegerv(GL_MINOR_VERSION, &minor);

   if ((10*major + minor) < 43)
   {
      throw std::runtime_error(
         "OpenGL version 4.3 or higher is required to run this program.");
   }

   surfaceMesh.initializeGL(solution.order());
   cutPlaneMesh.initializeGL(solution.order());

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_CULL_FACE);
   glEnable(GL_MULTISAMPLE);

   updateSurfMesh();
}


void RenderWidget::updateSurfMesh()
{
   if (!surfaceCoefs.numFaces())
   {
      surfaceCoefs.extract(solution);
   }
   std::cout << "Tesselating surface (level " << tessLevel << ")." << std::endl;
   surfaceMesh.tesselate(surfaceCoefs, tessLevel);

   updateCutMesh();
}


void RenderWidget::updateClipPlane()
{
   const double speed = 2;
   double phi = speed*clipY*M_PI/180;
   double theta = speed*clipX*M_PI/180;

   clipPlane.x = cos(phi)*cos(theta);
   clipPlane.y = sin(phi);
   clipPlane.z = -sin(theta);
   clipPlane.w = -0.005 * clipZ;
}


void RenderWidget::updateCutMesh()
{
   if (clipMode == 1)
   {
      if (!volumeCoefs.numElements())
      {
         volumeCoefs.extract(solution);
      }
      updateClipPlane();
      cutPlaneMesh.compute(volumeCoefs, clipPlane, tessLevel);
   }
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

   // set up the projection matrix
   glm::dmat4 proj = glm::perspective(glm::radians(30.0), aspect, 0.001, 10.0);

   // set up the model view matrix
   double scale = std::exp(zoom);
   glm::dmat4 view(1.0);
   view = glm::translate(view, glm::dvec3(0, 0, -2));
   view = glm::scale(view, glm::dvec3(scale, scale, scale));
   view = glm::rotate(view, glm::radians(rotateX), glm::dvec3(1, 0, 0));
   view = glm::rotate(view, glm::radians(rotateY), glm::dvec3(0, 1, 0));
   view = glm::rotate(view, glm::radians(-90.0), glm::dvec3(1, 0, 0));

   // final transformation matrix, round to floats
   glm::mat4 mvp = proj*view;

   // draw tesselated surface
   if (clipMode == 1) {
      updateClipPlane();
      glEnable(GL_CLIP_DISTANCE0);
   }
   else {
      glDisable(GL_CLIP_DISTANCE0);
   }
   surfaceMesh.draw(mvp, clipPlane, lines);

   // draw cut plane
   glDisable(GL_CLIP_DISTANCE0);
   if (clipMode == 1)
   {
      cutPlaneMesh.draw(mvp, lines);
   }
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
       const double speed = 0.75;
       rotateX += speed * deltaY;
       rotateY += speed * deltaX;
    }
    else if (event->buttons() & Qt::RightButton)
    {
       const double speed = 0.01;
       zoom -= speed * deltaY;
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
   const double speed = 0.1;
   zoom += speed * event->delta() / 120.0;
   updateGL();
}


void RenderWidget::keyPressEvent(QKeyEvent * event)
{
   int dir = (event->modifiers() & Qt::ShiftModifier) ? -1 : 1;

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
         if (tessLevel > 2) { tessLevel -= 2; }
         updateSurfMesh();
         break;

      case Qt::Key_Plus:
         tessLevel += 2;
         updateSurfMesh();
         break;

      case Qt::Key_I:
         clipMode = (clipMode + 1) % 2;
         updateCutMesh();
         break;

      case Qt::Key_M:
         lines = !lines;
         break;

      case Qt::Key_X:
         clipX += dir;
         updateCutMesh();
         break;

      case Qt::Key_Y:
         clipY += dir;
         updateCutMesh();
         break;

      case Qt::Key_Z:
         clipZ += dir;
         updateCutMesh();
         break;

      case Qt::Key_W:
         wireframe = !wireframe;
         break;
   }
   updateGL();
}

