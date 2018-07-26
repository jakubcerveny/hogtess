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
   SurfaceCoefs();

   void Extract(const mfem::GridFunction &solution,
                const mfem::GridFunction &curvature);

   int NFaces() const { return nf; }
   int Order() const { return order; }

   void BindBuffer(int location) const {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, buffer);
   }

protected:
   int nf, ndof, order;
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

   void Extract(const mfem::GridFunction &solution,
                const mfem::GridFunction &curvature);

   int NElements() const { return ne; }
   int Order() const { return order; }

   void BindBuffer(int location) const;

protected:
   int ne, ndof, order;
   GLuint buffer;
};


#endif // hogtess_coefs_hpp_included_
