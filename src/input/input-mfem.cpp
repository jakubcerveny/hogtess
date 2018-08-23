#include "mfem.hpp"

#include <fstream>

#include "input-mfem.hpp"
#include "utility.hpp"

using namespace mfem;


MFEMSolution::MFEMSolution(const std::vector<std::string> &meshPaths,
                           const std::vector<std::string> &solutionPaths)
{
   MFEM_VERIFY(meshPaths.size() == solutionPaths.size(), "");

   int nranks = meshPaths.size();
   meshes_.resize(nranks);
   solutions_.resize(nranks);

   order_ = -1;

   // load mesh & solution for each rank
   OMP(parallel for schedule(dynamic))
   for (int rank = 0; rank < nranks; rank++)
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

   // calculate min/max and normalization
   getMinMaxNorm();

   // nodal points
   nodes1d_ = mfem::poly1d.ClosedPoints
         (order_, mfem::Quadrature1D::GaussLobatto);
}


void MFEMSolution::updateMinMaxDof(const GridFunction *gf, int vd,
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
   for (unsigned rank = 0; rank < meshes_.size(); rank++)
   {
      const auto *nodes = meshes_[rank]->GetNodes();
      for (int i = 0; i < nodes->FESpace()->GetVDim(); i++)
      {
         updateMinMaxDof(nodes, i, min_[i], max_[i]);
      }
      updateMinMaxDof(solutions_[rank].get(), 0, min_[3], max_[3]);
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


static void GetFaceDofs(const FiniteElementSpace *space,
                        int face, Array<int> &dofs)
{
   if (space->GetNFDofs()) // H1 space: face DOFs exist
   {
      space->GetFaceDofs(face, dofs);
   }
   else // L2 space
   {
      Mesh* mesh = space->GetMesh();
      int e1, e2, inf1, inf2;
      mesh->GetFaceElements(face, &e1, &e2);
      mesh->GetFaceInfos(face, &inf1, &inf2);

      MFEM_ABORT("TODO");
   }
}


MFEMSolution::~MFEMSolution()
{
   // needs to be defined here where Mesh and GridFunction are complete types
}


void MFEMSurfaceCoefs::extract(const Solution &solution)
{
   const auto *msln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(msln, "Not an MFEM solution!");

   int numRanks = msln->numRanks();

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

   // count boundary faces
   std::vector<int> faceOffset(numRanks+1, 0);
   for (int rank = 0; rank < numRanks; rank++)
   {
      int nf = 0;
      const Mesh *mesh = msln->mesh(rank);
      for (int i = 0; i < mesh->GetNFaces(); i++)
      {
         if (!mesh->FaceIsInterior(i)) { nf++; }
      }
      faceOffset[rank+1] = faceOffset[rank] + nf;
   }

   int numFaces = faceOffset[numRanks];
   rank_.resize(numFaces);

   // CPU instance of the coefficient buffer
   int ndof = fe->GetDof();
   std::vector<float> faceCoefs(4*numFaces*ndof, 0.f);

   // extract coefficients
   OMP(parallel for schedule(dynamic))
   for (int rank = 0; rank < numRanks; rank++)
   {
      const Mesh *mesh = msln->mesh(rank);

      const auto *slnSpace = msln->solution(rank)->FESpace();
      const auto *nodesSpace = mesh->GetNodes()->FESpace();

      const GridFunction *gf = msln->solution(rank);
      const GridFunction *nodes = mesh->GetNodes();

      Array<int> dofs, vdofs;
      const Array<int> &dofMap = fe->GetDofMap();

      // extract face coefs
      for (int i = 0, nf = 0; i < mesh->GetNFaces(); i++)
      {
         if (mesh->FaceIsInterior(i)) { continue; }

         int fi = faceOffset[rank] + nf++;
         rank_[fi] = rank;

         float* coefs = &(faceCoefs[4*fi*ndof]);

         GetFaceDofs(slnSpace, i, dofs);
         MFEM_ASSERT(dofs.Size() == ndof, "");

         dofs.Copy(vdofs);
         slnSpace->DofsToVDofs(0, vdofs);

         for (int j = 0; j < ndof; j++)
         {
            double c = (*gf)(vdofs[dofMap[j]]);
            coefs[4*j + 3] = (c + msln->normOffset(3))*msln->normScale(3);
         }

         GetFaceDofs(nodesSpace, i, dofs);
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

   // upload to a shader buffer
   buffer_.upload(faceCoefs.data(), faceCoefs.size()*sizeof(float));
}


void MFEMVolumeCoefs::extract(const Solution &solution)
{
   const auto *msln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(msln, "Not an MFEM solution!");

   const Mesh *mesh = msln->mesh(0);

   const GridFunction *gf = msln->solution(0);
   const GridFunction *nodes = msln->mesh(0)->GetNodes();

   const auto *sln_space = gf->FESpace();
   const auto *nodes_space = nodes->FESpace();

   const auto *sln_fe = dynamic_cast<const H1_HexahedronElement*>(
      sln_space->FEColl()->FiniteElementForGeometry(Geometry::CUBE));
   const auto *nodes_fe = dynamic_cast<const H1_HexahedronElement*>(
      nodes_space->FEColl()->FiniteElementForGeometry(Geometry::CUBE));

   MFEM_VERIFY(sln_fe != NULL && nodes_fe != NULL,
               "Only H1_HexahedronElement supported at the moment.");
   MFEM_VERIFY(sln_fe->GetDof() == nodes_fe->GetDof(),
               "Curvature currently must have the same space as the solution.");

   int ne = mesh->GetNE();
   int ndof = sln_fe->GetDof();

   rank_.clear();
   rank_.resize(ne);

   boxes_.clear();
   boxes_.resize(ne);

   Array<int> dofs, vdofs;
   const Array<int> &dof_map = sln_fe->GetDofMap();

   std::vector<float> elem_coefs(4*ndof*ne, 0.f);

   // extract element coefficients
   for (int i = 0; i < ne; i++)
   {
      float* coefs = &(elem_coefs[4*ndof*i]);

      sln_space->GetElementDofs(i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      dofs.Copy(vdofs);
      sln_space->DofsToVDofs(0, vdofs);

      for (int j = 0; j < ndof; j++)
      {
         double c = (*gf)(vdofs[dof_map[j]]);
         coefs[4*j + 3] = (c + msln->normOffset(3))*msln->normScale(3);
      }

      nodes_space->GetElementDofs(i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      for (int vd = 0; vd < nodes_space->GetVDim(); vd++)
      {
         dofs.Copy(vdofs);
         nodes_space->DofsToVDofs(vd, vdofs);

         for (int j = 0; j < ndof; j++)
         {
            double c = (*nodes)(vdofs[dof_map[j]]);
            c = (c + msln->normOffset(vd))*msln->normScale(vd);
            coefs[4*j + vd] = c;
            boxes_[i].update(c, vd);
         }
      }
   }

   // upload to a shader buffer
   buffer_.upload(elem_coefs.data(), elem_coefs.size()*sizeof(float));
}

