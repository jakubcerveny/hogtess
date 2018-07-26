
#include <fstream>

#include "mfem.hpp"
#include "coefs.hpp"


void SurfaceCoefs::Extract(const mfem::GridFunction &solution,
                           const mfem::GridFunction &curvature)
{
   const mfem::Mesh *mesh = solution.FESpace()->GetMesh();

   MFEM_VERIFY(mesh->Dimension() == 3, "Need a 3D mesh.");
   MFEM_VERIFY(mesh == curvature.FESpace()->GetMesh(),
               "Solution and curvature must share the same mesh!");

   const mfem::FiniteElementSpace *sln_space = solution.FESpace();
   const mfem::FiniteElementSpace *crv_space = curvature.FESpace();

   const mfem::H1_QuadrilateralElement *sln_fe =
      dynamic_cast<const mfem::H1_QuadrilateralElement*>(
         sln_space->FEColl()->FiniteElementForGeometry(mfem::Geometry::SQUARE));

   const mfem::H1_QuadrilateralElement *crv_fe =
      dynamic_cast<const mfem::H1_QuadrilateralElement*>(
         crv_space->FEColl()->FiniteElementForGeometry(mfem::Geometry::SQUARE));

   MFEM_VERIFY(sln_fe != NULL && crv_fe != NULL,
               "Only H1_QuadrilateralElement supported at the moment.");
   MFEM_VERIFY(sln_fe->GetDof() == crv_fe->GetDof(),
               "Curvature currently must have the same space as the solution.");

   ndof = sln_fe->GetDof();
   order = sln_fe->GetOrder();

   const mfem::Array<int> &dof_map = sln_fe->GetDofMap();

   // count boundary faces
   nf = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      if (mesh->GetFace(i)->GetAttribute() > 0) { nf++; }
   }

   mfem::Array<int> dofs, vdofs;
   std::vector<float> face_coefs(4*nf*ndof, 0.f);

   // extract face coefs
   nf = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      const mfem::Element* face = mesh->GetFace(i);
      if (face->GetAttribute() <= 0) { continue; }

      float* coefs = &(face_coefs[4*(nf++)*ndof]);

      sln_space->GetFaceDofs(i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      dofs.Copy(vdofs);
      sln_space->DofsToVDofs(0, vdofs);

      for (int j = 0; j < ndof; j++)
      {
         coefs[4*j + 3] = solution(vdofs[dof_map[j]]);
      }

      crv_space->GetFaceDofs(i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      for (int vd = 0; vd < crv_space->GetVDim(); vd++)
      {
         dofs.Copy(vdofs);
         crv_space->DofsToVDofs(vd, vdofs);

         for (int j = 0; j < ndof; j++)
         {
            coefs[4*j + vd] = curvature(vdofs[dof_map[j]]);
         }
      }
   }

   // clean up if buffer already exists
   if (buffer)
   {
      glDeleteBuffers(1, &buffer);
   }

   // create a shader buffer
   glGenBuffers(1, &buffer);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
   glBufferData(GL_SHADER_STORAGE_BUFFER,
                face_coefs.size() * sizeof(float),
                face_coefs.data(), GL_STATIC_COPY);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

   // debug
   std::ofstream f("coefs.m");
   f << "A = [\n";
   for (int i = 0; i < face_coefs.size()/4; i++)
   {
      f << face_coefs[4*i + 0] << " ";
      f << face_coefs[4*i + 1] << " ";
      f << face_coefs[4*i + 2] << "\n";
   }
   f << "];\n";

}



