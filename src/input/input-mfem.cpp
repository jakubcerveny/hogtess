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
   MFEM_VERIFY(mesh_->GetNodes() != NULL,
               "Mesh needs to be curved (Nodes != NULL).");

   std::cout << "Loading solution: " << solutionPath << std::endl;
   std::ifstream is(solutionPath.c_str());
   solution_ = new GridFunction(mesh_, is);
   is.close();

   // TODO: project NURBS or elevate order of Nodes here, if needed

   const FiniteElement* meshFE =
         mesh_->GetNodes()->FESpace()->FEColl()->FiniteElementForGeometry(geom);
   const FiniteElement* slnFE =
         solution_->FESpace()->FEColl()->FiniteElementForGeometry(geom);

   MFEM_VERIFY(slnFE->GetDof() == meshFE->GetDof(),
               "Only isoparametric elements supported at the moment.");

   order_ = slnFE->GetOrder();
   std::cout << "Polynomial order: " << order_ << std::endl;

   // calculate min/max of the solution and the curvature
   // TODO: this is the lazy way to do that, the real extremes may not be in the nodes
   for (int i = 0; i < 4; i++)
   {
      min_[i] = max_[i] = 0;
   }
   for (int i = 0; i < mesh_->GetNodes()->FESpace()->GetVDim(); i++)
   {
      getMinMax(mesh_->GetNodes(), i, min_[i], max_[i]);
   }
   getMinMax(solution_, 0, min_[3], max_[3]);
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
   const auto *mfem_sln = dynamic_cast<const MFEMSolution*>(&solution);
   MFEM_VERIFY(mfem_sln, "Not an MFEM solution!");

   const Mesh *mesh = mfem_sln->mesh();

   const GridFunction *sln = mfem_sln->solution();
   const GridFunction *nodes = mfem_sln->mesh()->GetNodes();

   const auto *sln_space = sln->FESpace();
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
   order_ = sln_fe->GetOrder();

   const Array<int> &dof_map = sln_fe->GetDofMap();

   // count boundary faces
   nf_ = 0;
   for (int i = 0; i < mesh->GetNFaces(); i++)
   {
      if (mesh->GetFace(i)->GetAttribute() > 0) { nf_++; }
   }

   // calculate normalization coefficients
   double sln_min = mfem_sln->min(3);
   double sln_scale = 1.0/(mfem_sln->max(3) - sln_min);

   double center[3], max_size = 0.;
   for (int i = 0; i < 3; i++)
   {
      center[i] = (mfem_sln->min(i) + mfem_sln->max(i)) / 2;
      double size = mfem_sln->max(i) - mfem_sln->min(i);
      max_size = std::max(size, max_size);
   }
   double mesh_scale = 1.0 / max_size;

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
         coefs[4*j + 3] = ((*sln)(vdofs[dof_map[j]]) - sln_min)*sln_scale;
      }

      GetFaceDofs(nodes_space, i, dofs);
      MFEM_ASSERT(dofs.Size() == ndof, "");

      for (int vd = 0; vd < nodes_space->GetVDim(); vd++)
      {
         dofs.Copy(vdofs);
         nodes_space->DofsToVDofs(vd, vdofs);

         double min = mfem_sln->min(vd);
         double scale = 1.0/(mfem_sln->max(vd) - min);

         for (int j = 0; j < ndof; j++)
         {
            coefs[4*j + vd] =
               ((*nodes)(vdofs[dof_map[j]]) - center[vd])*mesh_scale;
         }
      }
   }

   // clean up if buffer already exists
   glDeleteBuffers(1, &buffer_);

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


