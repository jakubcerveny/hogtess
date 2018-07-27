#include <fstream>

#include "input-mfem.hpp"


MFEMSolution::MFEMSolution(const std::string &meshPath,
                           const std::string &solutionPath)
   : mesh_(nullptr)
   , solution_(nullptr)
{
   std::cout << "Loading mesh: " << meshPath << std::endl;
   mesh_ = new mfem::Mesh(meshPath.c_str());

   int geom = mfem::Geometry::CUBE;
   MFEM_VERIFY(mesh_->Dimension() == 3 || mesh_->GetElementBaseGeometry() == geom,
               "Only 3D hexes supported so far, sorry.");
   MFEM_VERIFY(mesh_->GetNodes() != NULL,
               "Mesh needs to be curved (Nodes != NULL).");

   std::cout << "Loading solution: " << solutionPath << std::endl;
   std::ifstream is(solutionPath.c_str());
   solution_ = new mfem::GridFunction(mesh_, is);
   is.close();

   // TODO: project NURBS or elevate order of Nodes here, if needed

   const mfem::FiniteElement* meshFE =
         mesh_->GetNodes()->FESpace()->FEColl()->FiniteElementForGeometry(geom);
   const mfem::FiniteElement* slnFE =
         solution_->FESpace()->FEColl()->FiniteElementForGeometry(geom);

   MFEM_VERIFY(slnFE->GetDof() == meshFE->GetDof(),
               "Only isoparametric elements supported at the moment.");

   order_ = slnFE->GetOrder();
   std::cout << "Polynomial order: " << order_ << std::endl;
}


MFEMSolution::~MFEMSolution()
{
   delete mesh_;
   delete solution_;
}


void MFEMSurfaceCoefs::extract(const Solution &solution)
{
   const auto *mfem_sln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(mfem_sln, "Not an MFEM solution!");

   const mfem::Mesh *mesh = mfem_sln->mesh();

   const mfem::GridFunction *sln = mfem_sln->solution();
   const mfem::GridFunction *nodes = mfem_sln->mesh()->GetNodes();

   const auto *sln_space = sln->FESpace();
   const auto *nodes_space = nodes->FESpace();

   const auto *sln_fe = dynamic_cast<const mfem::H1_QuadrilateralElement*>(
      sln_space->FEColl()->FiniteElementForGeometry(mfem::Geometry::SQUARE));
   const auto *nodes_fe = dynamic_cast<const mfem::H1_QuadrilateralElement*>(
      nodes_space->FEColl()->FiniteElementForGeometry(mfem::Geometry::SQUARE));

   MFEM_VERIFY(sln_fe != NULL && nodes_fe != NULL,
               "Only H1_QuadrilateralElement supported at the moment.");
   MFEM_VERIFY(sln_fe->GetDof() == nodes_fe->GetDof(),
               "Curvature currently must have the same space as the solution.");

   int ndof = sln_fe->GetDof();
   order_ = sln_fe->GetOrder();

   const mfem::Array<int> &dof_map = sln_fe->GetDofMap();

   // count boundary faces
   nf_ = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      if (mesh->GetFace(i)->GetAttribute() > 0) { nf_++; }
   }

   mfem::Array<int> dofs, vdofs;
   std::vector<float> face_coefs(4*nf_*ndof, 0.f);

   // extract face coefs
   nf_ = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      const mfem::Element* face = mesh->GetFace(i);
      if (face->GetAttribute() <= 0) { continue; }

      float* coefs = &(face_coefs[4*(nf_++)*ndof]);

      sln_space->GetFaceDofs(i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      dofs.Copy(vdofs);
      sln_space->DofsToVDofs(0, vdofs);

      for (int j = 0; j < ndof; j++)
      {
         coefs[4*j + 3] = (*sln)(vdofs[dof_map[j]]);
      }

      nodes_space->GetFaceDofs(i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      for (int vd = 0; vd < nodes_space->GetVDim(); vd++)
      {
         dofs.Copy(vdofs);
         nodes_space->DofsToVDofs(vd, vdofs);

         for (int j = 0; j < ndof; j++)
         {
            coefs[4*j + vd] = (*nodes)(vdofs[dof_map[j]]);
         }
      }
   }

   // clean up if buffer already exists
   if (buffer_)
   {
      glDeleteBuffers(1, &buffer_);
   }

   // create a shader buffer
   glGenBuffers(1, &buffer_);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
   glBufferData(GL_SHADER_STORAGE_BUFFER,
                face_coefs.size() * sizeof(float),
                face_coefs.data(), GL_STATIC_COPY);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


void MFEMVolumeCoefs::extract(const Solution &solution)
{
   // TODO
}


