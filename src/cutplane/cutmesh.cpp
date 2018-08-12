#include <glm/gtc/type_ptr.hpp>

#include "cutmesh.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "shape/shape.glsl.hpp"
#include "cutplane/voxelize.glsl.hpp"
#include "cutplane/march.glsl.hpp"
#include "cutplane/draw.glsl.hpp"
#include "cutplane/lines.glsl.hpp"

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

   progLines.link(
      VertexShader(version, {shaders::cutplane::lines}, defs),
      FragmentShader(version, {shaders::cutplane::lines}, defs));

   // adjust the tables
   for (int i = 0, j; i < 256; i++)
   {
      for (j = 0; mcTables.triTable[i][j] != -1; j++) {}
      // store vertex count
      mcTables.triTable[i][15] = j;
   }

   // buffer for marching cubes tables
   glGenBuffers(1, &tableBuffer);
   glBindBuffer(SSBO, tableBuffer);
   glBufferData(SSBO, sizeof(mcTables), &mcTables, GL_STATIC_DRAW);

   // atomic counter buffer
   glGenBuffers(1, &counterBuffer);
   glBindBuffer(SSBO, counterBuffer);
   glBufferData(SSBO, 2*sizeof(int), NULL, GL_STATIC_DRAW);

   // create an empty VAO
   glGenVertexArrays(1, &vao);
}


void CutPlaneMesh::compute(const VolumeCoefs &coefs,
                           const glm::vec4 &clipPlane, int level)
{
   deleteBuffers(false);

   subdivLevel = level;
   int numElems = coefs.numElements();

   // STEP 1: compute vertices of a 3D subdivision of the elements
   // TODO: do this only for elements whose bounding box is cut

   progVoxelize.use();
   glUniform1i(progVoxelize.uniform("level"), level);
   glUniform1f(progVoxelize.uniform("invLevel"), 1.0 / level);

   int localSize[3];
   progVoxelize.localSize(localSize);
   int sizeZ = roundUpMultiple(numElems*(level+1), localSize[2]);

   long vbufSize = 4*sizeof(float)*sqr(level+1)*sizeZ;
   std::cout << "Voxel buffer size: "
             << double(vbufSize) / 1024 / 1024 << " MB." << std::endl;

   glGenBuffers(1, &voxelBuffer);
   glBindBuffer(SSBO, voxelBuffer);
   glBufferData(SSBO, vbufSize, NULL, GL_DYNAMIC_COPY);
   glBindBufferBase(SSBO, 1, voxelBuffer);

   glBindBuffer(SSBO, coefs.buffer());
   glBindBufferBase(SSBO, 0, coefs.buffer());

   lagrangeUniforms(progVoxelize, solution.order(), solution.nodes1d());

   // launch the compute shader
   int groupsZ = divRoundUp((level+1)*numElems, localSize[2]);
   glDispatchCompute(level+1, level+1, groupsZ);
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);


   // STEP 2: use marching cubes to extract the mesh of the cut plane
   // TODO: keep buffers allocated

   progMarch.use();
   glUniform1i(progMarch.uniform("level"), level);
   glUniform4fv(progMarch.uniform("clipPlane"), 1, glm::value_ptr(clipPlane));

   glBindBuffer(SSBO, voxelBuffer);
   glBindBufferBase(SSBO, 0, voxelBuffer);

   glBindBuffer(SSBO, tableBuffer);
   glBindBufferBase(SSBO, 1, tableBuffer);

   // buffer to store generated triangles (triples of vertices)
   glGenBuffers(1, &triangleBuffer);
   glBindBuffer(SSBO, triangleBuffer);
   glBufferData(SSBO, 16*1024*1024/*FIXME*/*sizeof(float), NULL, GL_DYNAMIC_COPY);
   glBindBufferBase(SSBO, 2, triangleBuffer);

   // buffer to store generated lines (pairs of vertices)
   glGenBuffers(1, &lineBuffer);
   glBindBuffer(SSBO, lineBuffer);
   glBufferData(SSBO, 4*1024*1024/*FIXME*/*sizeof(float), NULL, GL_DYNAMIC_COPY);
   glBindBufferBase(SSBO, 3, lineBuffer);

   // reset the atomic counters
   counters[0] = counters[1] = 0;
   glBindBuffer(SSBO, counterBuffer);
   glBufferSubData(SSBO, 0, 2*sizeof(int), counters);
   glBindBufferBase(SSBO, 4, counterBuffer);

   // launch the compute shader and wait for completion
   glDispatchCompute(level, level, level*numElems);
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

   // read the number of vertices generated
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterBuffer);
   glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 2*sizeof(int), counters);

   std::cout << "counters: " << counters[0] << ", " << counters[1] << std::endl;

   glDeleteBuffers(1, &voxelBuffer);
   voxelBuffer = 0;
}


void CutPlaneMesh::draw(const glm::mat4 &mvp, bool lines)
{
   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   glBindBuffer(SSBO, triangleBuffer);
   glBindBufferBase(SSBO, 0, triangleBuffer);

   glBindVertexArray(vao);
   glDrawArrays(GL_TRIANGLES, 0, counters[0]);

   if (lines)
   {
      progLines.use();
      glUniformMatrix4fv(progLines.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

      glBindBuffer(SSBO, lineBuffer);
      glBindBufferBase(SSBO, 0, lineBuffer);

      glBindVertexArray(vao);
      glDrawArrays(GL_LINES, 0, counters[1]);
   }
}


void CutPlaneMesh::deleteBuffers(bool all)
{
   glDeleteBuffers(1, &triangleBuffer);
   glDeleteBuffers(1, &lineBuffer);

   if (all)
   {
      glDeleteBuffers(1, &voxelBuffer);
      glDeleteBuffers(1, &tableBuffer);
      glDeleteBuffers(1, &counterBuffer);
   }
}
