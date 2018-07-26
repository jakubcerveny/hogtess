#include <fstream>

#include <QApplication>

#include "main.hpp"
#include "render.hpp"
#include "coefs.hpp"

#include "mfem.hpp"


MainWindow::MainWindow(QWidget* gl)
{
    setCentralWidget(gl);
    setWindowTitle("hogtess");
}


/*double* extractPerElementCoefs(const mfem::GridFunction &gf, int vd)
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
}*/

void testCoefs(const SurfaceCoefs &coefs)
{
   std::ofstream f("test.m");
   f << "A = [\n";

   int p1 = coefs.Order() + 1;
   int ndof = p1*p1;

   int size = coefs.NFaces() * ndof * 4;
   float* buf = new float[size];

   coefs.BindBuffer();
   glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, buf);

   for (int i = 0; i < ndof * coefs.NFaces(); i++)
   {
      f << buf[4*i + 0] << " "
        << buf[4*i + 1] << " "
        << buf[4*i + 2] << "\n";
   }

   delete [] buf;
   f << "];\n";
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: hogtess <mesh> <solution>\n";
        return EXIT_FAILURE;
    }

    std::cout << "Loading mesh: " << argv[1] << std::endl;
    mfem::Mesh mesh(argv[1]);

    int geom = mfem::Geometry::SQUARE;
    /*if (mesh.Dimension() != 2 || mesh.GetElementBaseGeometry() != geom) {
       std::cout << "Only 2D quadrilaterals supported so far, sorry.\n";
       return EXIT_FAILURE;
    }*/
    if (!mesh.GetNodes()) {
       std::cout << "Mesh needs to be curved (Nodes != NULL).\n";
       return EXIT_FAILURE;
    }

    std::cout << "Loading solution: " << argv[2] << std::endl;
    std::ifstream is(argv[2]);
    mfem::GridFunction solution(&mesh, is);
    is.close();

    // TODO: project NURBS or elevate order of Nodes here, if needed

    /*const mfem::FiniteElement* meshFE =
          mesh.GetNodes()->FESpace()->FEColl()->FiniteElementForGeometry(geom);*/
    const mfem::FiniteElement* slnFE =
          solution.FESpace()->FEColl()->FiniteElementForGeometry(geom);

    /*if (slnFE->GetDof() != meshFE->GetDof()) {
       std::cout << "Only isoparametric elements supported at the moment.\n";
       return EXIT_FAILURE;
    }*/

    int order = slnFE->GetOrder();
    std::cout << "Polynomial order: " << order << std::endl;

    SurfaceCoefs surfaceCoefs(solution, *mesh.GetNodes());

    testCoefs(surfaceCoefs);

    const double* nodes =
       mfem::poly1d.ClosedPoints(order, mfem::Quadrature1D::GaussLobatto);

    /*double* meshCoefs[3] = {
       extractPerElementCoefs(*mesh.GetNodes(), 0),
       extractPerElementCoefs(*mesh.GetNodes(), 1),
       NULL
    };

    double* slnCoefs[3] = {
       extractPerElementCoefs(gridfn, 0),
       NULL,
       NULL
    };*/

/*    QApplication app(argc, argv);

    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    glf.setSamples(8);

    RenderWidget* gl =
         new RenderWidget(glf, mesh.GetNE(), order, nodes,
                          2, meshCoefs, 1, slnCoefs);

    MainWindow wnd(gl);
    gl->setParent(&wnd);

    QSize size(1200, 1000);
    wnd.resize(size);
    wnd.show();

    return app.exec();*/
    return 0;
}
