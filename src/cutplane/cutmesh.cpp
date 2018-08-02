#include <glm/gtc/type_ptr.hpp>

#include "cutmesh.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "shape/shape.glsl.hpp"
#include "cutplane/voxelize.glsl.hpp"
#include "cutplane/march.glsl.hpp"
#include "cutplane/draw.glsl.hpp"

#include "tables.hpp"

#define SSBO GL_SHADER_STORAGE_BUFFER


void CutPlaneMesh::initializeGL(int order)
{
   const int version = 430;

   Definitions defs;
   defs("P", std::to_string(order))
       ("PALETTE_SIZE", std::to_string(RGB_Palette_3_Size));

   progVoxelize.link(
      ComputeShader(version,
         {shaders::shape, shaders::cutplane::voxelize}, defs));

   progMarch.link(
      ComputeShader(version, {shaders::cutplane::march}, defs));

   progDraw.link(
      VertexShader(version, {shaders::cutplane::draw}, defs),
      FragmentShader(version, {shaders::cutplane::draw}, defs));

   /*progLines.link(
      VertexShader(version, {shaders::surface::lines}, defs),
      FragmentShader(version, {shaders::surface::lines}, defs));*/

   // adjust the tables
   for (int i = 0, j; i < 256; i++)
   {
      for (j = 0; mcTables.triTable[i][j] != -1; j++) {}
      // store vertex count
      mcTables.triTable[i][15] = j;
   }

   // create an empty VAO
   glGenVertexArrays(1, &vao);
}


void CutPlaneMesh::compute(const VolumeCoefs &coefs,
                           const glm::vec4 &clipPlane, int level)
{
   deleteBuffers();

   numElems = coefs.numElements();
   subdivLevel = level;

   // STEP 1: compute vertices of a 3D subdivision of the elements
   // TODO: do this only for elements whose bounding box is cut

   long vbufSize = 4*sizeof(float)*numElems*cube(level+1);
   std::cout << "Voxel buffer size: "
             << double(vbufSize) / 1024 / 1024 << " MB." << std::endl;

   GLuint voxelBuffer;
   glGenBuffers(1, &voxelBuffer);
   glBindBuffer(SSBO, voxelBuffer);
   glBufferData(SSBO, vbufSize, NULL, GL_STATIC_DRAW);
   glBindBufferBase(SSBO, 1, voxelBuffer);

   glBindBuffer(SSBO, coefs.buffer());
   glBindBufferBase(SSBO, 0, coefs.buffer());

   progVoxelize.use();
   glUniform1i(progVoxelize.uniform("level"), level);
   glUniform1f(progVoxelize.uniform("invLevel"), 1.0 / level);

   lagrangeUniforms(progVoxelize, solution.order(), solution.nodes1d());

   // launch the compute shader
   // TODO: group size 32 in Z
   glDispatchCompute(level+1, level+1, (level+1)*numElems);

   // wait until we can use the computed vertices
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);


   // STEP 2: use marching cubes to extract the mesh of the cut plane

   progMarch.use();
   glUniform1i(progMarch.uniform("level"), level);
   glUniform4fv(progMarch.uniform("clipPlane"), 1, glm::value_ptr(clipPlane));

   glBindBuffer(SSBO, voxelBuffer);
   glBindBufferBase(SSBO, 0, voxelBuffer);

   // buffer for marching cubes tables
   GLuint tableBuffer;
   glGenBuffers(1, &ssboTables);
   glBindBuffer(SSBO, tableBuffer);
   glBufferData(SSBO, sizeof(mcTables), &mcTables, GL_STATIC_DRAW);
   glBindBufferBase(SSBO, 2, tableBuffer);

   // create a shader buffer to store generated triangles (triples of vertices)
   glGenBuffers(1, &ssboVert);
   glBindBuffer(SSBO, ssboVert);
   glBufferData(SSBO, 12*1024*1024/*FIXME*/*sizeof(float), NULL, GL_STATIC_DRAW);
   glBindBufferBase(SSBO, 0, ssboVert);

   // buffer for an atomic uint (the number of generated vertices)
   glGenBuffers(1, &ssboCount);
   glBindBuffer(SSBO, ssboCount);
   glBufferData(SSBO, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
   glBindBufferBase(SSBO, 1, ssboCount);



   glDeleteBuffers(1, &voxelBuffer);
}


void CutPlaneMesh::draw(const glm::mat4 &mvp, bool lines)
{
   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   glBindBuffer(SSBO, triangleBuffer);
   glBindBufferBase(SSBO, 0, triangleBuffer);

   int nv = cube(subdivLevel + 1)*numElems;

   glBindVertexArray(vao);
   glDrawArrays(GL_POINTS, 0, nv);
}


void CutPlaneMesh::deleteBuffers()
{
   GLuint buf[2] = { triangleBuffer, lineBuffer };
   glDeleteBuffers(2, buf);
}
