#ifndef hogtess_cutmesh_hpp_included__
#define hogtess_cutmesh_hpp_included__

#include <glm/fwd.hpp>

#include "input/input.hpp"
#include "shader.hpp"


/**
 */
class CutPlaneMesh
{
public:
   CutPlaneMesh(const Solution &solution)
      : solution(solution)
      , subdivLevel(0), numVertices(0)
      , vao(0), triangleBuffer(0), lineBuffer(0)
   {}

   /// Compile shaders.
   void initializeGL(int order);

   /** Subdivide the volume into cells and extract an "isosurface" that
       corresponds to the physical cutting plane. */
   void compute(const VolumeCoefs &coefs, const glm::vec4 &clipPlane,
                int level);

   /// Draw the calculated cut plane.
   void draw(const glm::mat4 &mvp, bool lines);

   virtual ~CutPlaneMesh() { deleteBuffers(); }

protected:
   const Solution &solution;
   int subdivLevel, numVertices;

   Program progVoxelize, progMarch;
   Program progDraw, progLines;

   GLuint vao, triangleBuffer, lineBuffer;

   void deleteBuffers();
};


#endif // hogtess_cutmesh_hpp_included__
