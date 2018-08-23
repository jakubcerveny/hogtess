#ifndef hogtess_input_mfem_hpp_included_
#define hogtess_input_mfem_hpp_included_

#include <vector>
#include <string>
#include <memory>

#include "input.hpp"


namespace mfem
{
// forwards
class Mesh;
class GridFunction;
}


class MFEMSolution : public Solution
{
public:
   MFEMSolution(const std::vector<std::string> &meshPaths,
                const std::vector<std::string> &solutionPaths);

   const int numRanks() const { return meshes_.size(); }

   const mfem::Mesh *mesh(int rank) const
   {
      return meshes_[rank].get();
   }
   const mfem::GridFunction *solution(int rank) const
   {
      return solutions_[rank].get();
   }

   // normalization coefficients (0,1,2=domain, 3=solution)
   double normScale(int i) const { return scale_[i]; }
   double normOffset(int i) const { return offset_[i]; }

   virtual ~MFEMSolution();

protected:
   std::vector<std::unique_ptr<mfem::Mesh>> meshes_;
   std::vector<std::unique_ptr<mfem::GridFunction>> solutions_;

   double scale_[4], offset_[4];

   void getMinMaxNorm();
   void updateMinMax(const mfem::GridFunction *gf,
                     int vd, double &min, double &max);
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
