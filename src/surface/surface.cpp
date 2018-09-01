#include <glm/gtc/type_ptr.hpp>

#include "surface.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "shape/shape.glsl.hpp"
#include "surface/tesselate.glsl.hpp"
#include "surface/draw.glsl.hpp"
#include "surface/lines.glsl.hpp"


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

   progLines.link(
      VertexShader(version, {shaders::surface::lines}, defs),
      FragmentShader(version, {shaders::surface::lines}, defs));

   // create an empty VAO
   glGenVertexArrays(1, &vao);
}


void SurfaceMesh::tesselate(int level)
{
   numFaces = coefs.numFaces();
   tessLevel = level;

   int faceVerts = sqr(level + 1);
   long vbSize = 4*sizeof(float)*faceVerts*numFaces;

   bufVertices.resize(vbSize);

   progCompute.use();
   glUniform1i(progCompute.uniform("level"), level);
   glUniform1f(progCompute.uniform("invLevel"), 1.0 / level);

   lagrangeUniforms(progCompute, solution.order(), solution.nodes1d());

   coefs.buffer().bind(0);
   bufVertices.bind(1);

   // launch the compute shader
   // TODO: group size 32 in Z
   glDispatchCompute(level+1, level+1, numFaces);

   // wait until we can use the computed vertices
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

   makeQuadFaceIndexBuffers(level);
}


void SurfaceMesh::makeQuadFaceIndexBuffers(int level)
{
   int nTri = 2*sqr(level);
   int nLines = 4*level;

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

   bufIndices.upload(indices, 3*nTri*sizeof(int));

   n = 0;
   for (int i = 0; i < level; i++)
   {
      indices[n++] = i;
      indices[n++] = i+1;

      indices[n++] = (level+1)*level + i;
      indices[n++] = (level+1)*level + i+1;

      indices[n++] = (level+1)*i;
      indices[n++] = (level+1)*(i+1);

      indices[n++] = (level+1)*i + level;
      indices[n++] = (level+1)*(i+1) + level;
   }

   bufLineIndices.upload(indices, 2*nLines*sizeof(int));

   delete [] indices;
}


void SurfaceMesh::draw(const glm::mat4 &mvp, const glm::vec4 &clipPlane,
                       const Buffer &bufPartMat, bool lines)
{
   int nFaceVert = sqr(tessLevel + 1);
   int nFaceTri = 2*sqr(tessLevel);
   int nFaceLines = 4*tessLevel;

   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform1i(progDraw.uniform("nFaceVert"), nFaceVert);
   glUniform4fv(progDraw.uniform("clipPlane"), 1, glm::value_ptr(clipPlane));
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   bufVertices.bind(0);
   bufIndices.bind(1);
   coefs.faceRanks().bind(2);
   bufPartMat.bind(3);

   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(1, 1); // push triangles behind lines

   glBindVertexArray(vao);
   glDrawArraysInstanced(GL_TRIANGLES, 0, 3*nFaceTri, numFaces);

   glDisable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(0, 0);

   if (lines)
   {
      progLines.use();
      glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
      glUniform1i(progLines.uniform("nFaceVert"), nFaceVert);
      glUniform4fv(progDraw.uniform("clipPlane"), 1, glm::value_ptr(clipPlane));

      bufVertices.bind(0);
      bufLineIndices.bind(1);
      coefs.faceRanks().bind(2);
      bufPartMat.bind(3);

      glBindVertexArray(vao);
      glDrawArraysInstanced(GL_LINES, 0, 2*nFaceLines, numFaces);
   }
}

