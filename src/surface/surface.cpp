#include <glm/gtc/type_ptr.hpp>

#include "surface.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "shape/shape.glsl.hpp"
#include "surface/tesselate.glsl.hpp"
#include "surface/draw.glsl.hpp"


void SurfaceMesh::initializeGL(int order)
{
   const int version = 430;

   Definitions defs;
   defs("P", std::to_string(order))
       ("PALETTE_SIZE", std::to_string(RGB_Palette_3_Size));

   ShaderSource::list computeSurface{
      shaders::shape,
      shaders::surface::tesselate
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
   deleteBuffers();

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

   lagrangeUniforms(progCompute, coefs.order(), nodalPoints);

   // launch the compute shader
   // TODO: group size 32 in Z
   glDispatchCompute(level+1, level+1, numFaces);

   // wait until we can use the computed vertices
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

   makeQuadFaceIndexBuffer(level);
}


void SurfaceMesh::makeQuadFaceIndexBuffer(int level)
{
   int nTri = 2*sqr(level);
   int *indices = new int[3*nTri];

   int n = 0;
   for (int i = 0; i < level; i++)
   for (int j = 0; j < level; j++)
   {
      int a = i*(level + 1) + j;
      int b = (i + 1)*(level + 1) + j;

      if ((i < level/2) ^ (j < level/2))
      {
         indices[n++] = a;
         indices[n++] = b;
         indices[n++] = b+1;

         indices[n++] = b+1;
         indices[n++] = a+1;
         indices[n++] = a;
      }
      else
      {
         indices[n++] = a+1;
         indices[n++] = a;
         indices[n++] = b;

         indices[n++] = b;
         indices[n++] = b+1;
         indices[n++] = a+1;
      }
   }

   glGenBuffers(1, &indexBuffer);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
   glBufferData(GL_SHADER_STORAGE_BUFFER, 3*nTri*sizeof(int), indices,
                GL_STATIC_DRAW);

   delete [] indices;
}


void SurfaceMesh::draw(const glm::mat4 &mvp, bool lines)
{
   int nFaceVert = sqr(tessLevel + 1);
   int nFaceTri = 2*sqr(tessLevel);

   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform1i(progDraw.uniform("nFaceVert"), nFaceVert);
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexBuffer);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexBuffer);

   glBindVertexArray(vao);
   glDrawArraysInstanced(GL_TRIANGLES, 0, 3*nFaceTri, numFaces);
}


void SurfaceMesh::deleteBuffers()
{
   GLuint buf[3] = { vertexBuffer, indexBuffer, lineBuffer };
   glDeleteBuffers(3, buf);
}
