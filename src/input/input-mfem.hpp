#ifndef hogtess_input_mfem_hpp_included_
#define hogtess_input_mfem_hpp_included_

#include <string>

#include "input.hpp"


namespace mfem // forwards
{
class Mesh;
class GridFunction;
}


class MFEMSolution : public Solution
{
public:
   MFEMSolution(const std::string &meshPath,
                const std::string &solutionPath);

   const mfem::Mesh *mesh() const { return mesh_; }
   const mfem::GridFunction *solution() const { return solution_; }

   // normalization coefficients (0,1,2=domain, 3=solution)
   double normScale(int i) const { return scale_[i]; }
   double normOffset(int i) const { return offset_[i]; }

   virtual ~MFEMSolution();

protected:
   mfem::Mesh *mesh_;
   mfem::GridFunction *solution_;
   double scale_[4], offset_[4];

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
