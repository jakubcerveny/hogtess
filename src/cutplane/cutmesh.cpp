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

   // adjust and upload the tables
   for (int i = 0, j; i < 256; i++)
   {
      for (j = 0; mcTables.triTable[i][j] != -1; j++) {}
      // store vertex count
      mcTables.triTable[i][15] = j;
   }
   bufTables.upload(&mcTables, sizeof(mcTables));

   // create an empty VAO
   glGenVertexArrays(1, &vao);
}


bool boxCut(const BBox<float> &box, const glm::vec4 clipPlane,
            const glm::mat4 &mat, float eps = 1e-3)
{
   float corners[8][3] =
   {
      { box.min[0], box.min[1], box.min[2] },
      { box.max[0], box.min[1], box.min[2] },
      { box.min[0], box.max[1], box.min[2] },
      { box.max[0], box.max[1], box.min[2] },
      { box.min[0], box.min[1], box.max[2] },
      { box.max[0], box.min[1], box.max[2] },
      { box.min[0], box.max[1], box.max[2] },
      { box.max[0], box.max[1], box.max[2] }
   };

   int pos = 0, neg = 0;
   for (int i = 0; i < 8; i++)
   {
      glm::vec4 corner(corners[i][0], corners[i][1], corners[i][2], 1);

      float d = dot(mat*corner, clipPlane);

      if (d > -eps) { pos++; }
      if (d < eps) { neg++; }

      if (pos && neg) { return true; }
   }

   return false; // all on positive or all on negative side
}


void CutPlaneMesh::compute(const glm::vec4 &clipPlane,
                           const Buffer &bufPartMat,
                           int level)
{
   const long MB = 1024*1024;

   subdivLevel = level;

   // STEP 1: determine which elements need to be processed. It's the ones
   //         whose bounding box intersects the clipping plane

   std::vector<int> elemIndices;
   for (int i = 0; i < coefs.numElements(); i++)
   {
      // get transform for element i
      int rank = coefs.elemRanks().data<int>(i);
      const glm::mat4 &mat = bufPartMat.data<glm::mat4>(rank);

      // check intersection with cutting plane
      if (boxCut(coefs.boundingBox(i), clipPlane, mat))
      {
         elemIndices.push_back(i);
      }
   }
   int numElems = elemIndices.size();

   bufElemIndices.upload(elemIndices);


   // STEP 2: compute the vertices of a 3D subdivision of selected elements

   progVoxelize.use();
   glUniform1i(progVoxelize.uniform("level"), level);
   glUniform1f(progVoxelize.uniform("invLevel"), 1.0 / level);
   glUniform1i(progVoxelize.uniform("numElems"), numElems);

   lagrangeUniforms(progVoxelize, solution.order(), solution.nodes1d());

   int lsize[3];
   progVoxelize.localSize(lsize);
   int sizeX = roundUpMultiple(numElems*(level+1), lsize[0]);

   // prepare a buffer for voxel vertices, try to reuse the buffer if possible
   long vbSize = 4*sizeof(float)*sizeX*sqr(level+1);
   if (bufVertices.size() < vbSize)
   {
      std::cout << "Voxel buffer size: " << double(vbSize)/MB << " MB." << std::endl;
      bufVertices.resize(vbSize);
   }

   coefs.buffer().bind(0);
   bufElemIndices.bind(1);
   bufVertices.bind(2);
   coefs.elemRanks().bind(3);
   bufPartMat.bind(4);

   // launch the compute shader
   int groupsX = divRoundUp((level+1)*numElems, lsize[0]);
   glDispatchCompute(groupsX, level+1, level+1);
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);


   // STEP 3: use marching cubes to extract the mesh of the cut plane

   while (1)
   {
      // start with big enough buffers
      bufTriangles.resize(std::max(bufTriangles.size(), 2*MB));
      bufLines.resize(std::max(bufLines.size(), 1*MB));

      progMarch.use();
      glUniform1i(progMarch.uniform("level"), level);
      glUniform4fv(progMarch.uniform("clipPlane"), 1, glm::value_ptr(clipPlane));

      bufVertices.bind(0);
      bufTables.bind(1);
      bufTriangles.bind(2);
      bufLines.bind(3);

      // reset the atomic counters
      counters[0] = counters[1] = 0;
      bufCounters.upload(counters, 2*sizeof(int));
      bufCounters.bind(4);

      // launch the compute shader and wait for completion
      glDispatchCompute(level, level, level*numElems);
      glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

      // read the number of vertices generated
      bufCounters.download(counters, 2*sizeof(int));

      long tsize = counters[0] * 4*sizeof(float);
      long lsize = counters[1] * 4*sizeof(float);

      if (tsize <= bufTriangles.size() &&
          lsize <= bufLines.size())
      {
         // good, buffers were large enough, we're done
         break;
      }

      // enlarge the buffers and try again
      if (tsize > bufTriangles.size())
      {
         bufTriangles.resize(2*bufTriangles.size());
         std::cout << "Triangle buffer size: "
                   << double(bufTriangles.size())/MB << " MB." << std::endl;
      }
      if (lsize > bufLines.size())
      {
         bufLines.resize(2*bufLines.size());
         std::cout << "Line buffer size: "
                   << double(bufLines.size())/MB << " MB." << std::endl;
      }
   }
}


void CutPlaneMesh::draw(const glm::mat4 &mvp, bool lines)
{
   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   bufTriangles.bind(0);

   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(1, 1); // push triangles behind lines

   glBindVertexArray(vao);
   glDrawArrays(GL_TRIANGLES, 0, counters[0]);

   glDisable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(0, 0);

   if (lines)
   {
      progLines.use();
      glUniformMatrix4fv(progLines.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

      bufLines.bind(0);

      glBindVertexArray(vao);
      glDrawArrays(GL_LINES, 0, counters[1]);
   }
}


void CutPlaneMesh::free()
{
   bufElemIndices.discard();
   bufVertices.discard();
   bufCounters.discard();
   bufTriangles.discard();
   bufLines.discard();
   // NOTE: not discarding bufTables
}
