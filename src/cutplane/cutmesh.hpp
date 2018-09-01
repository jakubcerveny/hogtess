#ifndef hogtess_cutmesh_hpp_included__
#define hogtess_cutmesh_hpp_included__

#include <glm/fwd.hpp>

#include "input/input.hpp"
#include "shader.hpp"
#include "buffer.hpp"


/**
 */
class CutPlaneMesh
{
public:
   CutPlaneMesh(const Solution &solution, const VolumeCoefs &coefs)
      : solution(solution), coefs(coefs)
      , subdivLevel(0), vao(0)
   {}

   /// Compile shaders.
   void initializeGL(int order);

   /** Subdivide the volume into linear cells and extract an "isosurface"
       that corresponds to the cutting plane. */
   void compute(const glm::vec4 &clipPlane,
                const Buffer &bufPartMat,
                int level);

   /// Draw the computed cut plane.
   void draw(const glm::mat4 &mvp, bool lines);

   /// Deallocate all GPU buffers.
   void free();

protected:
   const Solution &solution;
   const VolumeCoefs &coefs;

   int subdivLevel, counters[2];

   Program progVoxelize, progMarch;
   Program progDraw, progLines;

   Buffer bufElemIndices, bufVertices;
   Buffer bufTables, bufCounters;
   Buffer bufTriangles, bufLines;

   GLuint vao;
};


#endif // hogtess_cutmesh_hpp_included__
