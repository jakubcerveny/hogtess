#ifndef hogtess_surface_hpp_included__
#define hogtess_surface_hpp_included__

#include <glm/glm.hpp>

#include "input.hpp"
#include "shader.hpp"


/**
 *
 */
class SurfaceMesh
{
public:
   SurfaceMesh(const double *nodalPoints)
      : nodalPoints(nodalPoints)
      , numFaces(0), tessLevel(0)
      , vao(0), vertexBuffer(0)
      //, indexBuffer(0), indexBufferLines(0)
   {}

   ///
   void initializeGL(int order);

   /**
    */
   void tesselate(const SurfaceCoefs &coefs, int level);

   /**
    */
   void draw(const glm::mat4 &MVP, bool lines);

   virtual ~SurfaceMesh() {}

protected:
   const double* nodalPoints;
   int numFaces, tessLevel;

   Program progCompute, progDraw;
   GLuint vao, vertexBuffer;

   //GLuint indexBuffer, indexBufferLines;

   //void deleteBuffers();
};


#endif // hogtess_surface_hpp_included__
