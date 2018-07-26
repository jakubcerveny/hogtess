#ifndef hogtess_coefs_hpp_included_
#define hogtess_coefs_hpp_included_

#include "GL/gl.h"

//namespace mfem { class GridFunction; }


/** Holds (an abstract) finite element solution, i.e., a high-order FE function
 *  on a curved mesh.
 */
class Solution
{
public:
   virtual ~Solution() {}
};


/** Extracts and stores the 2D coefficients of the surface of a 3D FEM solution
 *  on a curved mesh. The solution is normalized, converted from double to
 *  single precision and uploaded to a GPU buffer. The faces are treated as
 *  dicontinous, i.e., interface DOFs are duplicated.
 *
 *  The buffer data has this format: vec4[NFaces][NDofs]
 */
class SurfaceCoefs
{
public:
   SurfaceCoefs() : nf(), order(), buffer() {}

   virtual void Extract(const Solution* solution) = 0;

   int NFaces() const { return nf; }
   int Order() const { return order; }

   /// Return the ID of the SSBO.
   GLuint Buffer() const { return buffer; }

   virtual ~SurfaceCoefs();

protected:
   int nf, order;
   GLuint buffer;
};


/** Stores coefficients of a 3D FEM solution on a curved mesh. The solution is
 *  normalized, converted from double to single precision and uploaded to a GPU
 *  buffer. The elements are treated as discontinous, i.e., interface DOFs are
 *  duplicated.
 */
class VolumeCoefs
{
public:
   VolumeCoefs();

   virtual void Extract(const Solution* solution) = 0;

   int NElements() const { return ne; }
   int Order() const { return order; }

   /// Return the ID of the SSBO.
   GLuint Buffer() const { return buffer; }

   virtual ~VolumeCoefs();

protected:
   int ne, order;
   GLuint buffer;
};


#endif // hogtess_coefs_hpp_included_
