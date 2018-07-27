#include "surface.hpp"
#include "utility.hpp"
#include "shape.hpp"
#include "palette.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "shaders/shape.glsl.hpp"
#include "shaders/compute-surface.glsl.hpp"
#include "shaders/surface.glsl.hpp"


void SurfaceMesh::initializeGL(int order)
{
   const int version = 430;

   Definitions defs;
   defs("P", std::to_string(order))
       ("PALETTE_SIZE", std::to_string(RGB_Palette_3_Size));

   ShaderSource::list computeSurface{
      shaders::shape,
      shaders::surface::compute
   };

   progCompute.link(
      ComputeShader(version, computeSurface, defs));

   progDraw.link(
      VertexShader(version, {shaders::surface::draw}, defs),
      FragmentShader(version, {shaders::surface::draw}, defs));

   glGenVertexArrays(1, &vao);
}


void SurfaceMesh::tesselate(const SurfaceCoefs &coefs, int level)
{
   glDeleteBuffers(1, &vertexBuffer);

   numFaces = coefs.numFaces();
   tessLevel = level;

   int faceVerts = sqr(level + 1);
   long vertBufSize = 4*sizeof(float)*faceVerts*numFaces;

   glGenBuffers(1, &vertexBuffer);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glBufferData(GL_SHADER_STORAGE_BUFFER, vertBufSize, NULL, GL_STATIC_DRAW);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexBuffer);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, coefs.buffer());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, coefs.buffer());

   progCompute.use();
   glUniform1i(progCompute.uniform("level"), level);
   glUniform1f(progCompute.uniform("invLevel"), 1.0 / level);
   lagrangeShapeInit(progCompute, coefs.order(), nodalPoints);

   // launch the compute shader and wait for completion
   glDispatchCompute(level+1, level+1, coefs.numFaces());
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void SurfaceMesh::draw(const glm::mat4 &MVP, bool lines)
{
   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("MVP"),
                      1, GL_FALSE, glm::value_ptr(MVP));
   glUniform3fv(progDraw.uniform("palette"),
                RGB_Palette_3_Size, (const float*) RGB_Palette_3);

   glBindVertexArray(vao);
   glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);

   glDrawArrays(GL_POINTS, 0, numFaces*sqr(tessLevel+1));
}


/*void SurfaceMesh::deleteBuffers()
{
   GLuint buf[3] = { vertexBuffer, indexBuffer, indexBufferLines };
   glDeleteBuffers(3, buf);
   vertexBuffer = indexBuffer = indexBufferLines = 0;
}*/
