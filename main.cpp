#include <fstream>

#include <QApplication>

#include "main.hpp"
#include "render.hpp"

#include "mfem.hpp"


MainWindow::MainWindow(QWidget* gl)
{
    setCentralWidget(gl);
    setWindowTitle("hogtess");
}

double* extractPerElementCoefs(const mfem::GridFunction &gf, int vd)
{
   const mfem::FiniteElementSpace *space = gf.FESpace();

   const mfem::H1_QuadrilateralElement *fe =
         dynamic_cast<const mfem::H1_QuadrilateralElement*>(
            space->FEColl()->FiniteElementForGeometry(mfem::Geometry::SQUARE));

   if (!fe) {
      MFEM_ABORT("Only H1_QuadrilateralElement supported.");
   }

   int nd = fe->GetDof();
   int ne = space->GetNE();

   double* coefs = new double[ne * nd];

   const mfem::Array<int> &dof_map = fe->GetDofMap();

   mfem::Array<int> dofs;
   for (int i = 0; i < ne; i++)
   {
      space->GetElementDofs(i, dofs);
      space->DofsToVDofs(vd, dofs);

      MFEM_VERIFY(dofs.Size() == nd, "");
      for (int j = 0; j < nd; j++)
      {
         coefs[i*nd + j] = gf(dofs[dof_map[j]]);
      }
   }

   return coefs;
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: hogtess <mesh> <solution>\n";
        return EXIT_FAILURE;
    }

    std::cout << "Loading mesh: " << argv[1] << std::endl;
    mfem::Mesh mesh(argv[1]);

    int geom = mfem::Geometry::SQUARE;
    if (mesh.Dimension() != 2 || mesh.GetElementBaseGeometry() != geom) {
       std::cout << "Only 2D quadrilaterals supported so far, sorry.\n";
       return EXIT_FAILURE;
    }
    if (!mesh.GetNodes()) {
       std::cout << "Mesh needs to be curved (Nodes != NULL).\n";
       return EXIT_FAILURE;
    }

    std::cout << "Loading solution: " << argv[2] << std::endl;
    std::ifstream is(argv[2]);
    mfem::GridFunction gridfn(&mesh, is);
    is.close();

    // TODO: project NURBS or elevate order of Nodes here, if needed

    const mfem::FiniteElement* mesh_fe =
          mesh.GetNodes()->FESpace()->FEColl()->FiniteElementForGeometry(geom);
    const mfem::FiniteElement* sln_fe =
          gridfn.FESpace()->FEColl()->FiniteElementForGeometry(geom);

    if (sln_fe->GetDof() != mesh_fe->GetDof()) {
       std::cout << "Only isoparametric elements supported at the moment.\n";
       return EXIT_FAILURE;
    }

    double* mesh_coefs[3] = {
       extractPerElementCoefs(*mesh.GetNodes(), 0),
       extractPerElementCoefs(*mesh.GetNodes(), 1),
       NULL
    };

    double* sln_coefs[3] = {
       extractPerElementCoefs(gridfn, 0),
       NULL,
       NULL
    };

    QApplication app(argc, argv);

    RenderWidget* gl =
         new RenderWidget(mesh.GetNE(), sln_fe->GetOrder(),
                          2, mesh_coefs, 1, sln_coefs);

    MainWindow wnd(gl);
    gl->setParent(&wnd);

    QSize size(1200, 1000);
    wnd.resize(size);
    wnd.show();

    return app.exec();
}
