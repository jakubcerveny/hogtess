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

#include "shaders/shape.glsl.hpp"
#include "shaders/surface.glsl.hpp"

const double PanSpeed = 0.005;
const double RotateSpeed = 0.4;


RenderWidget::RenderWidget(const QGLFormat &format,
      int numElements, int polyOrder, const double *nodes,
      int meshDim, const double* const* meshCoefs,
      int slnDim, const double* const* slnCoefs)

  : QGLWidget(format, (QWidget*) 0)

  , numElements(numElements)
  , polyOrder(polyOrder)
  , nodes(nodes)

  , meshDim(meshDim)
  , slnDim(slnDim)

  , meshCoefs(meshCoefs)
  , slnCoefs(slnCoefs)

  , rotating(false)
  , scaling(false)
  , translating(false)

  , rotateX(0.), rotateY(0.)
  , scale(0.)
  , panX(0.), panY(0.)
{
   grabKeyboard();
}

RenderWidget::~RenderWidget()
{
}

void RenderWidget::compileShaders()
{
   const int version = 400;

   ShaderSource::list surface{
      shaders::shape,
      shaders::surface
   };

   progSurface.link(
       VertexShader(version, surface),
       TessControlShader(version, surface),
       TessEvalShader(version, surface),
       FragmentShader(version, surface),
       {0, "inPosition"});
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

   compileShaders();

   glGenVertexArrays(1, &vao); // create an empty VAO

   /*static glm::vec4 coefs[P+1][P+1] =
   {
      { {2*0.0, 0.0, 0, 1},
        {2*0.5,-0.1, 0, 1},
        {2*1.0, 0.0, 0, 1} },
      { {2*0.2, 0.5, 0, 1},
        {2*0.6, 0.5, 0, 1},
        {2*1.1, 0.5, 0, 1} },
      { {2*0.0, 1.0, 0, 1},
        {2*0.5, 1.0, 0, 1},
        {2*1.2, 1.2, 0, 1} }
   };*/

   int p1 = polyOrder + 1;
   int nc = p1 * p1;

   glm::vec4 *coefs = new glm::vec4[numElements * nc];

   for (int i = 0; i < numElements; i++)
   {
      for (int j = 0; j < nc; j++)
      {
         int index = i*nc + j;
         glm::vec4 &vec = coefs[index];
         vec.x = meshCoefs[0][index];
         vec.y = meshCoefs[1][index];
         vec.z = 0;
         vec.w = 1;//slnCoefs[0][index];
      }
   }

   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_RECTANGLE, tex);
   glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, nc, numElements,
                0, GL_RGBA, GL_FLOAT, coefs);
   delete [] coefs;

   glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   //glEnable(GL_DEPTH_TEST);
   //glEnable(GL_CULL_FACE);
   glEnable(GL_MULTISAMPLE);

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void RenderWidget::resizeGL(int width, int height)
{
   glViewport(0, 0, width, height);
   curSize = QSize(width, height);
   aspect = (double) width / height;
}

void RenderWidget::shapeInit(const Program &prog)
{
   int p1 = polyOrder+1;
   double weights[p1];
   float fnodes[p1], fweights[p1];

   for (int i = 0; i <= polyOrder; i++)
   {
      weights[i] = 1.0;
   }
   for (int i = 0; i <= polyOrder; i++)
   {
      for (int j = 0; j < i; j++)
      {
         double xij = nodes[i] - nodes[j];
         weights[i] *=  xij;
         weights[j] *= -xij;
      }
   }
   for (int i = 0; i <= polyOrder; i++)
   {
      fnodes[i] = nodes[i];
      fweights[i] = 1.0 / weights[i];
   }

   glUniform1fv(prog.uniform("lagrangeNodes"), p1, fnodes);
   glUniform1fv(prog.uniform("lagrangeWeights"), p1, fweights);
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
   //modelView = glm::translate(modelView, glm::vec3(-0.5, -0.5, -0.5));

   glm::mat4 MVP = projection * modelView;

   progSurface.use();

   glUniformMatrix4fv(progSurface.uniform("MVP"), 1, GL_FALSE,
                      glm::value_ptr(MVP));

   glUniform2f(progSurface.uniform("screenSize"),
               curSize.width(), curSize.height());

   glUniform1i(progSurface.uniform("sampler"), 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_RECTANGLE, tex);

   shapeInit(progSurface);

   glBindVertexArray(vao);
   glPatchParameteri(GL_PATCH_VERTICES, 4);
   glDrawArrays(GL_PATCHES, 0, 4*numElements);
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

