#ifndef hogtess_surface_hpp_included__
#define hogtess_surface_hpp_included__

#include "input.hpp"
#include "shader.hpp"


/**
 *
 */
class SurfaceMesh
{
public:
   SurfaceMesh(const double *nodalPoints)
      : vertexBuffer(0)
      , indexBuffer(0), indexBufferLines(0)
      , nodalPoints(nodalPoints)
   {}

   virtual ~SurfaceMesh() { deleteBuffers(); }

   ///
   void compileShaders(int order);

   /**
    */
   void tesselate(const SurfaceCoefs &coefs, int level);


protected:
   Program progCompute;
   GLuint vertexBuffer;
   GLuint indexBuffer, indexBufferLines;
   const double* nodalPoints;

   void deleteBuffers();
};


#endif // hogtess_surface_hpp_included__
