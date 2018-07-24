#ifndef hogtess_coefs_hpp_included_
#define hogtess_coefs_hpp_included_

#include "GL/gl.h"

namespace mfem { class GridFunction; }


/** Extracts and stores the 2D coefficients of the surface of a 3D FEM solution
 *  on a curved mesh. The solution is normalized, converted from double to
 *  single precision and uploaded to a GPU buffer. The faces are treated as
 *  dicontinous, i.e., interface DOFs are duplicated.
 */
class SurfaceCoefs
{
public:
   SurfaceCoefs(const mfem::GridFunction &solution,
                const mfem::GridFunction &curvature);

   int order() const;

   void bindBuffer(int location) const;

protected:
   GLint buffer;
};


/** Stores coefficients of a 3D FEM solution on a curved mesh. The solution is
 *  normalized, converted from double to single precision and uploaded to a GPU
 *  buffer. The elements are treated as discontinous, i.e., interface DOFs are
 *  duplicated.
 */
class VolumeCoefs
{
public:
   VolumeCoefs(const mfem::GridFunction &solution,
               const mfem::GridFunction &curvature);

   int order() const;

   void bindBuffer(int location) const;

protected:
   GLint buffer;
};


#endif // hogtess_coefs_hpp_included_
