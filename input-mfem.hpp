#ifndef hogtess_input_mfem_hpp_included_
#define hogtess_input_mfem_hpp_included_

#include "mfem.hpp"

#include <string>

#include "coefs.hpp"


class MFEMSolution : public Solution
{
public:
   MFEMSolution(const std::string &meshPath,
                const std::string &solutionPath);

   const mfem::Mesh *Mesh() const { return mesh; }
   const mfem::GridFunction *Solution() const { return solution; }

   virtual ~MFEMSolution();

protected:
   mfem::Mesh *mesh;
   mfem::GridFunction *solution;
};


class MFEMSurfaceCoefs : public SurfaceCoefs
{
public:
   MFEMSurfaceCoefs() : SurfaceCoefs() {}

   virtual void Extract(const Solution &solution);

   virtual ~MFEMSurfaceCoefs();
};


#endif // hogtess_input_mfem_hpp_included_
