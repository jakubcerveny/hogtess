#include <fstream> // debug

#include "surface.hpp"
#include "utility.hpp"
#include "shape.hpp"


#include "shaders/shape.glsl.hpp"
#include "shaders/compute-surface.glsl.hpp"


void SurfaceMesh::compileShaders(int order)
{
   const int version = 430;

   Definitions defs;
   defs("P", std::to_string(order));

   ShaderSource::list computeSurface{
      shaders::shape,
      shaders::computeSurface
   };

   progCompute.link(ComputeShader(version, computeSurface, defs));
}


void SurfaceMesh::tesselate(const SurfaceCoefs &coefs, int level)
{
   deleteBuffers();

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, coefs.buffer());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, coefs.buffer());

   int faceVerts = sqr(level + 1);
   long vertBufSize = 4*sizeof(float)*faceVerts*coefs.numFaces();

   glGenBuffers(1, &vertexBuffer);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glBufferData(GL_SHADER_STORAGE_BUFFER, vertBufSize, NULL, GL_STATIC_DRAW);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexBuffer);

   progCompute.use();
   glUniform1i(progCompute.uniform("level"), level);
   glUniform1f(progCompute.uniform("invLevel"), 1.0 / level);
   lagrangeShapeInit(progCompute, coefs.order(), nodalPoints);

   // launch the compute shader and wait for completion
   glDispatchCompute(level+1, level+1, coefs.numFaces());
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   // test
   float* debug = new float[vertBufSize/sizeof(float)];
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, vertBufSize, debug);

   std::ofstream f("test.m");
   f << "A=[\n";
   for (int i = 0; i < faceVerts*coefs.numFaces(); i++)
   {
      f << debug[i*4 + 0] << " ";
      f << debug[i*4 + 1] << " ";
      f << debug[i*4 + 2] << "\n";
   }
   f << "];\n";

   delete [] debug;
}


void SurfaceMesh::deleteBuffers()
{
   GLuint buf[3] = { vertexBuffer, indexBuffer, indexBufferLines };
   glDeleteBuffers(3, buf);
   vertexBuffer = indexBuffer = indexBufferLines = 0;
}
