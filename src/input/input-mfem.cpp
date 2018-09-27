#include "mfem.hpp"

#include <fstream>

#include "input-mfem.hpp"
#include "utility.hpp"

using namespace mfem;


MFEMSolution::MFEMSolution(const std::vector<std::string> &meshPaths,
                           const std::vector<std::string> &solutionPaths)
{
   MFEM_VERIFY(meshPaths.size() == solutionPaths.size(), "");

   numRanks_ = meshPaths.size();
   meshes_.resize(numRanks_);
   solutions_.resize(numRanks_);

   order_ = -1;

   // load mesh & solution for each rank
   OMP(parallel for schedule(dynamic))
   for (int rank = 0; rank < numRanks_; rank++)
   {
      std::cout << "Loading " << meshPaths[rank] << std::endl;
      mfem::Mesh *mesh = new Mesh(meshPaths[rank].c_str());

      int geom = Geometry::CUBE;
      MFEM_VERIFY(mesh->Dimension() == 3 &&
                  mesh->GetElementBaseGeometry() == geom,
                  "Only 3D hexes supported so far, sorry.");

      std::cout << "Loading " << solutionPaths[rank] << std::endl;
      std::ifstream is(solutionPaths[rank].c_str());
      mfem::GridFunction *sln = new GridFunction(mesh, is);
      is.close();

      const FiniteElement* slnFE =
         sln->FESpace()->FEColl()->FiniteElementForGeometry(geom);

      int order = slnFE->GetOrder();

      OMP(critical)
      {
         if (order_ < 0) {
            order_ = order;
         }
         else {
            MFEM_VERIFY(order == order_,
                        "All solutions must have the same polynomial order.");
         }
      }

      // ensure the mesh has Nodes
      if (mesh->GetNodes() == NULL)
      {
         mesh->SetCurvature(order);
      }

      const FiniteElement* meshFE =
         mesh->GetNodes()->FESpace()->FEColl()->FiniteElementForGeometry(geom);

      // elevate degree of Nodes, if needed
      if (meshFE->GetOrder() < order)
      {
         mesh->SetCurvature(order);
         meshFE = mesh->GetNodes()->FESpace()->FEColl()
               ->FiniteElementForGeometry(geom);
      }

      MFEM_VERIFY(slnFE->GetDof() == meshFE->GetDof(),
                  "Only isoparametric elements are supported at the moment.");

      meshes_[rank].reset(mesh);
      solutions_[rank].reset(sln);
   }

   std::cout << "Polynomial order: " << order_ << std::endl;

   // calculate min/max, normalization and centers
   getMinMaxNorm();
   getCenters();

   // nodal points
   nodes1d_ = mfem::poly1d.ClosedPoints
         (order_, mfem::Quadrature1D::GaussLobatto);
}


static void updateMinMax(const GridFunction *gf, int vd,
                         double &min, double &max)
{
   const auto *fes = gf->FESpace();
   for (int dof = 0; dof < fes->GetNDofs(); dof++)
   {
      double x = (*gf)(fes->DofToVDof(dof, vd));
      min = std::min(x, min);
      max = std::max(x, max);
   }
}

void MFEMSolution::getMinMaxNorm()
{
   // calculate the approximate min/max of the solution and the domain
   for (int i = 0; i < 4; i++)
   {
      min_[i] = std::numeric_limits<double>::max();
      max_[i] = std::numeric_limits<double>::lowest();
   }
   for (unsigned rank = 0; rank < numRanks_; rank++)
   {
      const auto *nodes = meshes_[rank]->GetNodes();
      for (int i = 0; i < nodes->FESpace()->GetVDim(); i++)
      {
         updateMinMax(nodes, i, min_[i], max_[i]);
      }
      updateMinMax(solutions_[rank].get(), 0, min_[3], max_[3]);
   }
   for (int i = 0; i < 4; i++)
   {
      if (min_[i] > max_[i]) { min_[i] = max_[i] = 0; }
   }

   // calculate normalization coefficients
   double max_size = 0.;
   for (int i = 0; i < 3; i++)
   {
      offset_[i] = -0.5*(min_[i] + max_[i]);
      double size = max_[i] - min_[i];
      max_size = std::max(size, max_size);
   }
   scale_[0] = scale_[1] = scale_[2] = 1.0 / max_size;

   offset_[3] = -min_[3];
   scale_[3] = 1.0 / (max_[3] - min_[3]);
}


void MFEMSolution::getCenters()
{
   centers_.clear();
   centers_.resize(3*numRanks_, 0.0);

   for (unsigned rank = 0; rank < numRanks_; rank++)
   {
      const auto *nodes = meshes_[rank]->GetNodes();
      const auto *fes = nodes->FESpace();

      for (int vd = 0; vd < fes->GetVDim(); vd++)
      {
         for (int dof = 0; dof < fes->GetNDofs(); dof++)
         {
            double x = (*nodes)(fes->DofToVDof(dof, vd));
            centers_[3*rank + vd] += x*scale_[vd] + offset_[vd];
         }
      }
      for (int vd = 0; vd < 3; vd++)
      {
         centers_[3*rank + vd] /= fes->GetNDofs();
      }
   }
}


MFEMSolution::~MFEMSolution()
{
   // needs to be defined here where Mesh and GridFunction are complete types
}


void MFEMSurfaceCoefs::extract(const Solution &solution)
{
   int numRanks = solution.numRanks();

   const auto *msln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(msln, "Not an MFEM solution!");

   // check the finite element types
   const H1_QuadrilateralElement* fe;
   for (int rank = 0; rank < numRanks; rank++)
   {
      const auto *slnSpace = msln->solution(rank)->FESpace();
      const auto *nodesSpace = msln->mesh(rank)->GetNodes()->FESpace();

      const auto *slnFE = dynamic_cast<const H1_QuadrilateralElement*>(
         slnSpace->FEColl()->TraceFiniteElementForGeometry(Geometry::SQUARE));
      const auto *nodesFE = dynamic_cast<const H1_QuadrilateralElement*>(
         nodesSpace->FEColl()->TraceFiniteElementForGeometry(Geometry::SQUARE));

      MFEM_VERIFY(slnFE != NULL && nodesFE != NULL,
                  "Only H1_QuadrilateralElement supported at the moment.");
      MFEM_VERIFY(slnFE->GetDof() == nodesFE->GetDof(),
                  "Curvature currently must have the same space as the solution.");
      fe = slnFE;
   }
   int ndof = fe->GetDof();

   // count boundary faces
   std::vector<int> faceOffset(numRanks+1, 0);
   for (int rank = 0; rank < numRanks; rank++)
   {
      const Mesh *mesh = msln->mesh(rank);
      faceOffset[rank+1] = faceOffset[rank] + mesh->GetNBE();
   }
   nf_ = faceOffset[numRanks];

   // CPU instances of the buffers
   std::vector<float> faceCoefs(4*nf_*ndof, 0.f);
   std::vector<int> ranks(nf_, 0);

   // extract coefficients
   OMP(parallel for schedule(dynamic))
   for (int rank = 0; rank < numRanks; rank++)
   {
      const Mesh *mesh = msln->mesh(rank);

      const GridFunction *gf = msln->solution(rank);
      const GridFunction *nodes = mesh->GetNodes();

      const auto *slnSpace = gf->FESpace();
      const auto *nodesSpace = nodes->FESpace();

      Array<int> dofs, vdofs;
      const Array<int> &dofMap = fe->GetDofMap();

      // extract face coefs
      for (int i = 0, nf = 0; i < mesh->GetNBE(); i++)
      {
         int fi = faceOffset[rank] + i;
         ranks[fi] = rank;

         float* coefs = &(faceCoefs[4*fi*ndof]);

         slnSpace->GetBdrElementDofs(i, dofs);
         MFEM_ASSERT(dofs.Size() == ndof, "");

         dofs.Copy(vdofs);
         slnSpace->DofsToVDofs(0, vdofs);

         for (int j = 0; j < ndof; j++)
         {
            double c = (*gf)(vdofs[dofMap[j]]);
            coefs[4*j + 3] = (c + msln->normOffset(3))*msln->normScale(3);
         }

         nodesSpace->GetBdrElementDofs(i, dofs);
         MFEM_ASSERT(dofs.Size() == ndof, "");

         for (int vd = 0; vd < nodesSpace->GetVDim(); vd++)
         {
            dofs.Copy(vdofs);
            nodesSpace->DofsToVDofs(vd, vdofs);

            double min = msln->min(vd);
            double scale = 1.0/(msln->max(vd) - min);

            for (int j = 0; j < ndof; j++)
            {
               double c = (*nodes)(vdofs[dofMap[j]]);
               coefs[4*j + vd] = (c + msln->normOffset(vd))*msln->normScale(vd);
            }
         }
      }
   }

   // upload to shader buffers
   buffer_.upload(faceCoefs);
   ranks_.upload(ranks);
}


void MFEMVolumeCoefs::extract(const Solution &solution)
{
   int numRanks = solution.numRanks();

   const auto *msln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(msln, "Not an MFEM solution!");

   // check the finite element types
   const H1_HexahedronElement* fe;
   for (int rank = 0; rank < numRanks; rank++)
   {
      const auto *slnSpace = msln->solution(rank)->FESpace();
      const auto *nodesSpace = msln->mesh(rank)->GetNodes()->FESpace();

      const auto *slnFE = dynamic_cast<const H1_HexahedronElement*>(
         slnSpace->FEColl()->FiniteElementForGeometry(Geometry::CUBE));
      const auto *nodesFE = dynamic_cast<const H1_HexahedronElement*>(
         nodesSpace->FEColl()->FiniteElementForGeometry(Geometry::CUBE));

      MFEM_VERIFY(slnFE != NULL && nodesFE != NULL,
                  "Only H1_HexahedronElement supported at the moment.");
      MFEM_VERIFY(slnFE->GetDof() == nodesFE->GetDof(),
                  "Curvature currently must have the same space as the solution.");
      fe = slnFE;
   }
   int ndof = fe->GetDof();

   // count elements
   std::vector<int> elemOffset(numRanks+1, 0);
   for (int rank = 0; rank < numRanks; rank++)
   {
      const Mesh *mesh = msln->mesh(rank);
      elemOffset[rank+1] = elemOffset[rank] + mesh->GetNE();
   }
   ne_ = elemOffset[numRanks];

   // CPU instances of the buffers
   std::vector<float> elemCoefs(4*ne_*ndof, 0.f);
   std::vector<int> ranks(ne_, 0);

   boxes_.clear();
   boxes_.resize(ne_);

   // extract coefficients
   OMP(parallel for schedule(dynamic))
   for (int rank = 0; rank < numRanks; rank++)
   {
      const Mesh *mesh = msln->mesh(rank);

      const GridFunction *gf = msln->solution(rank);
      const GridFunction *nodes = msln->mesh(rank)->GetNodes();

      const auto *slnSpace = gf->FESpace();
      const auto *nodesSpace = nodes->FESpace();

      Array<int> dofs, vdofs;
      const Array<int> &dofMap = fe->GetDofMap();

      // extract element coefficients
      for (int i = 0; i < mesh->GetNE(); i++)
      {
         int ei = elemOffset[rank] + i;
         ranks[ei] = rank;

         float* coefs = &(elemCoefs[4*ei*ndof]);

         slnSpace->GetElementDofs(i, dofs);
         MFEM_ASSERT(dofs.Size() == ndof, "");

         dofs.Copy(vdofs);
         slnSpace->DofsToVDofs(0, vdofs);

         for (int j = 0; j < ndof; j++)
         {
            double c = (*gf)(vdofs[dofMap[j]]);
            coefs[4*j + 3] = (c + msln->normOffset(3))*msln->normScale(3);
         }

         nodesSpace->GetElementDofs(i, dofs);
         MFEM_ASSERT(dofs.Size() == ndof, "");

         for (int vd = 0; vd < nodesSpace->GetVDim(); vd++)
         {
            dofs.Copy(vdofs);
            nodesSpace->DofsToVDofs(vd, vdofs);

            for (int j = 0; j < ndof; j++)
            {
               double c = (*nodes)(vdofs[dofMap[j]]);
               c = (c + msln->normOffset(vd))*msln->normScale(vd);
               coefs[4*j + vd] = c;
               boxes_[ei].update(c, vd);
            }
         }
      }
   }

   // upload to shader buffers
   buffer_.upload(elemCoefs);
   ranks_.upload(ranks);
   ranks_.copy(ranks);
}

