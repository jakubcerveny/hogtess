#ifndef hogtess_surface_hpp_included__
#define hogtess_surface_hpp_included__

#include <glm/glm.hpp>

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
   SurfaceMesh(const double *nodalPoints)
      : nodalPoints(nodalPoints)
      , numFaces(0), tessLevel(0)
      , vao(0), vertexBuffer(0)
      , indexBuffer(0), lineBuffer(0)
   {}

   /// Compile the shaders.
   void initializeGL(int order);

   /// Tesselate the surface. Can only be called once.
   void tesselate(const SurfaceCoefs &coefs, int level);

   /// Draw the tesselated faces. Can be called many times.
   void draw(const glm::mat4 &mvp, bool lines);

   virtual ~SurfaceMesh() { deleteBuffers(); }

protected:
   const double* nodalPoints;
   int numFaces, tessLevel;

   Program progCompute, progDraw, progLines;
   GLuint vao, vertexBuffer;
   GLuint indexBuffer, lineBuffer;

   void makeQuadFaceIndexBuffers(int level);
   void deleteBuffers();
};


#endif // hogtess_surface_hpp_included__
