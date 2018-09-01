#ifndef hogtess_surface_hpp_included__
#define hogtess_surface_hpp_included__

#include <glm/fwd.hpp>

#include "input/input.hpp"
#include "shader.hpp"


/** Encapsulates the ability to tesselate faces using a compute shader and
 *  subsequently to draw their meshes with an instanced draw command.
 *
 *  The vertex buffer holds sqr(tessLevel+1) vertices for each face.
 *  The index buffer exists for one face instance only and is used repeatedly.
 */
class SurfaceMesh
{
public:
   SurfaceMesh(const Solution &solution,
               const SurfaceCoefs &coefs)
      : solution(solution), coefs(coefs)
      , numFaces(0), tessLevel(0), vao(0)
   {}

   /// Compile shaders.
   void initializeGL(int order);

   /** Tesselate the surface. The specified subdivision level should be a
       multiple of 2. This function may only be called when 'level' changes. */
   void tesselate(int level);

   /// Draw the tesselated faces. Can be called many times.
   void draw(const glm::mat4 &mvp, const glm::vec4 &clipPlane,
             const Buffer &bufPartMat, bool lines);

protected:
   const Solution &solution;
   const SurfaceCoefs &coefs;

   int numFaces, tessLevel;

   Program progCompute, progDraw, progLines;

   Buffer bufVertices;
   Buffer bufIndices, bufLineIndices;

   GLuint vao;

   void makeQuadFaceIndexBuffers(int level);
};


#endif // hogtess_surface_hpp_included__
