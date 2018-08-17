#include "mfem.hpp"

#include <fstream>

#include "input-mfem.hpp"

using namespace mfem;


MFEMSolution::MFEMSolution(const std::string &meshPath,
                           const std::string &solutionPath)
   : mesh_(nullptr)
   , solution_(nullptr)
{
   std::cout << "Loading mesh: " << meshPath << std::endl;
   mesh_ = new Mesh(meshPath.c_str());

   int geom = Geometry::CUBE;
   MFEM_VERIFY(mesh_->Dimension() == 3 || mesh_->GetElementBaseGeometry() == geom,
               "Only 3D hexes supported so far, sorry.");

   std::cout << "Loading solution: " << solutionPath << std::endl;
   std::ifstream is(solutionPath.c_str());
   solution_ = new GridFunction(mesh_, is);
   is.close();

   const FiniteElement* slnFE =
      solution_->FESpace()->FEColl()->FiniteElementForGeometry(geom);

   order_ = slnFE->GetOrder();
   std::cout << "Polynomial order: " << order_ << std::endl;

   if (mesh_->GetNodes() == NULL)
   {
      mesh_->SetCurvature(order_);
   }
   // TODO: project NURBS or elevate order of Nodes here, if needed

   const FiniteElement* meshFE =
      mesh_->GetNodes()->FESpace()->FEColl()->FiniteElementForGeometry(geom);

   MFEM_VERIFY(slnFE->GetDof() == meshFE->GetDof(),
               "Only isoparametric elements supported at the moment.");

   // calculate the approximate min/max of the solution and the domain
   for (int i = 0; i < 4; i++)
   {
      min_[i] = max_[i] = 0;
   }
   for (int i = 0; i < mesh_->GetNodes()->FESpace()->GetVDim(); i++)
   {
      getMinMax(mesh_->GetNodes(), i, min_[i], max_[i]);
   }
   getMinMax(solution_, 0, min_[3], max_[3]);

   // calculate normalization coefficients
   offset_[3] = -min_[3];
   scale_[3] = 1.0 / (max_[3] - min_[3]);

   double max_size = 0.;
   for (int i = 0; i < 3; i++)
   {
      offset_[i] = -0.5*(min_[i] + max_[i]);
      double size = max_[i] - min_[i];
      max_size = std::max(size, max_size);
   }
   scale_[0] = scale_[1] = scale_[2] = 1.0 / max_size;

   // nodal points
   nodes1d_ = mfem::poly1d.ClosedPoints(order_, mfem::Quadrature1D::GaussLobatto);
}


void MFEMSolution::getMinMax(GridFunction *gf, int vd,
                             double &min, double &max)
{
   min = std::numeric_limits<double>::max();
   max = std::numeric_limits<double>::lowest();

   const auto *fes = gf->FESpace();
   for (int dof = 0; dof < fes->GetNDofs(); dof++)
   {
      double x = (*gf)(fes->DofToVDof(dof, vd));
      min = std::min(x, min);
      max = std::max(x, max);
   }
}


MFEMSolution::~MFEMSolution()
{
   delete mesh_;
   delete solution_;
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


void MFEMSurfaceCoefs::extract(const Solution &solution)
{
   const auto *msln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(msln, "Not an MFEM solution!");

   const Mesh *mesh = msln->mesh();

   const GridFunction *gf = msln->solution();
   const GridFunction *nodes = msln->mesh()->GetNodes();

   const auto *sln_space = gf->FESpace();
   const auto *nodes_space = nodes->FESpace();

   const auto *sln_fe = dynamic_cast<const H1_QuadrilateralElement*>(
      sln_space->FEColl()->TraceFiniteElementForGeometry(Geometry::SQUARE));
   const auto *nodes_fe = dynamic_cast<const H1_QuadrilateralElement*>(
      nodes_space->FEColl()->TraceFiniteElementForGeometry(Geometry::SQUARE));

   MFEM_VERIFY(sln_fe != NULL && nodes_fe != NULL,
               "Only H1_QuadrilateralElement supported at the moment.");
   MFEM_VERIFY(sln_fe->GetDof() == nodes_fe->GetDof(),
               "Curvature currently must have the same space as the solution.");

   int ndof = sln_fe->GetDof();

   const Array<int> &dof_map = sln_fe->GetDofMap();

   // count boundary faces
   nf_ = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      if (mesh->GetFace(i)->GetAttribute() > 0) { nf_++; }
   }

   Array<int> dofs, vdofs;
   std::vector<float> face_coefs(4*nf_*ndof, 0.f);

   // extract face coefs
   nf_ = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      if (mesh->FaceIsInterior(i)) { continue; }

      float* coefs = &(face_coefs[4*(nf_++)*ndof]);

      GetFaceDofs(sln_space, i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      dofs.Copy(vdofs);
      sln_space->DofsToVDofs(0, vdofs);

      for (int j = 0; j < ndof; j++)
      {
         double c = (*gf)(vdofs[dof_map[j]]);
         coefs[4*j + 3] = (c + msln->normOffset(3))*msln->normScale(3);
      }

      GetFaceDofs(nodes_space, i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      for (int vd = 0; vd < nodes_space->GetVDim(); vd++)
      {
         dofs.Copy(vdofs);
         nodes_space->DofsToVDofs(vd, vdofs);

         double min = msln->min(vd);
         double scale = 1.0/(msln->max(vd) - min);

         for (int j = 0; j < ndof; j++)
         {
            double c = (*nodes)(vdofs[dof_map[j]]);
            coefs[4*j + vd] = (c + msln->normOffset(vd))*msln->normScale(vd);
         }
      }
   }

   // upload to a shader buffer
   buffer_.upload(face_coefs.data(), face_coefs.size()*sizeof(float));
}


void MFEMVolumeCoefs::extract(const Solution &solution)
{
   const auto *msln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(msln, "Not an MFEM solution!");

   const Mesh *mesh = msln->mesh();

   const GridFunction *gf = msln->solution();
   const GridFunction *nodes = msln->mesh()->GetNodes();

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

   ne_ = mesh->GetNE();

   int ndof = sln_fe->GetDof();
   std::cout << "elem dofs: " << ndof << std::endl;

   boxes_.clear();
   boxes_.resize(ne_);

   Array<int> dofs, vdofs;
   const Array<int> &dof_map = sln_fe->GetDofMap();

   std::vector<float> elem_coefs(4*ndof*ne_, 0.f);

   // extract element coefficients
   for (int i = 0; i < ne_; i++)
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

         double min = msln->min(vd);
         double scale = 1.0/(msln->max(vd) - min);

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

