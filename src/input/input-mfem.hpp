#ifndef hogtess_input_mfem_hpp_included_
#define hogtess_input_mfem_hpp_included_

#include "mfem.hpp"

#include <string>

#include "input.hpp"


class MFEMSolution : public Solution
{
public:
   MFEMSolution(const std::string &meshPath,
                const std::string &solutionPath);

   const mfem::Mesh *mesh() const { return mesh_; }
   const mfem::GridFunction *solution() const { return solution_; }

   virtual ~MFEMSolution();

protected:
   mfem::Mesh *mesh_;
   mfem::GridFunction *solution_;

   void getMinMax(mfem::GridFunction *gf, int vd, double &min, double &max);
};


class MFEMSurfaceCoefs : public SurfaceCoefs
{
public:
   MFEMSurfaceCoefs() : SurfaceCoefs() {}

   virtual void extract(const Solution &solution);
};


class MFEMVolumeCoefs : public VolumeCoefs
{
public:
   MFEMVolumeCoefs() : VolumeCoefs() {}

   virtual void extract(const Solution &solution);
};


#endif // hogtess_input_mfem_hpp_included_
