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
      : nodalPoints(nodalPoints)
      , numFaces(0), tessLevel(0)
      , vao(0), vertexBuffer(0)
      , indexBuffer(0), indexBufferLines(0)
   {}

   virtual ~SurfaceMesh() { deleteBuffers(); }

   ///
   void initializeGL(int order);

   /**
    */
   void tesselate(const SurfaceCoefs &coefs, int level);

   /**
    */
   void drawSurface();


protected:
   const double* nodalPoints;
   int numFaces, tessLevel;

   Program progCompute;
   GLuint vao;
   GLuint vertexBuffer;
   GLuint indexBuffer, indexBufferLines;

   void deleteBuffers();
};


#endif // hogtess_surface_hpp_included__
