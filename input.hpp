#ifndef hogtess_input_hpp_included_
#define hogtess_input_hpp_included_

#include "GL/gl.h"


/** Holds an abstract high order finite element solution on a curved mesh.
 */
class Solution
{
public:
   virtual int order() const = 0;

   virtual ~Solution() {}
};


/** Extracts and stores the 2D coefficients of the surface of a 3D FEM solution.
 *  The solution is normalized, converted from double to single precision and
 *  uploaded to a GPU buffer. The faces are treated as discontinous, i.e.,
 *  interface DOFs are duplicated.
 *
 *  The buffer has this format in GLSL: vec4[numFaces][numFaceDofs]
 *  The 'xyz' part is the curvature and 'w' is the solution.
 */
class SurfaceCoefs
{
public:
   SurfaceCoefs() : nf_(), order_(), buffer_() {}

   virtual void extract(const Solution &solution) = 0;

   int numFaces() const { return nf_; }
   int order() const { return order_; }

   /// Return the ID of the SSBO.
   GLuint buffer() const { return buffer_; }

   virtual ~SurfaceCoefs()
   {
      if (buffer_) { glDeleteBuffers(1, &buffer_); }
   }

protected:
   int nf_, order_;
   GLuint buffer_;
};


/** Stores the coefficients of a 3D FEM solution. The solution is normalized,
 *  converted from double to single precision and uploaded to a GPU buffer.
 *  The elements are treated as discontinous, i.e., interface DOFs are
 *  duplicated.
 *
 *  The buffer has this format in GLSL: vec4[numElements][numElemDofs]
 *  The 'xyz' part is the curvature and 'w' is the solution.
 */
class VolumeCoefs
{
public:
   VolumeCoefs() : ne_(), order_(), buffer_(0) {}

   virtual void extract(const Solution &solution) = 0;

   int numElements() const { return ne_; }
   int order() const { return order_; }

   /// Return the ID of the SSBO.
   GLuint buffer() const { return buffer_; }

   virtual ~VolumeCoefs()
   {
      if (buffer_) { glDeleteBuffers(1, &buffer_); }
   }

protected:
   int ne_, order_;
   GLuint buffer_;
};


#endif // hogtess_input_hpp_included_
