#include <glm/gtc/type_ptr.hpp>

#include "surface.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "shape/shape.glsl.hpp"
#include "surface/compute.glsl.hpp"
#include "surface/draw.glsl.hpp"


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

   // create an empty VAO
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

   // launch the compute shader
   glDispatchCompute(level+1, level+1, coefs.numFaces());

   // wait until we can use the computed vertices
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}


void SurfaceMesh::draw(const glm::mat4 &mvp, bool lines)
{
   int nFaceVert = sqr(tessLevel+1);

   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform1i(progDraw.uniform("nFaceVert"), nFaceVert);
   glUniform3fv(progDraw.uniform("palette"),
                RGB_Palette_3_Size, (const float*) RGB_Palette_3);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexBuffer);

   glBindVertexArray(vao);
   glDrawArraysInstanced(GL_POINTS, 0, nFaceVert, numFaces);
}

