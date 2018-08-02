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
      , numElems(0), subdivLevel(0)
      , vao(0), vertexBuffer(0), lineBuffer(0)
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
   int numElems, subdivLevel;

   Program progCompute, progDraw, progLines;
   GLuint vao, vertexBuffer, lineBuffer;

   void deleteBuffers();
};


#endif // hogtess_cutmesh_hpp_included__
