#include <fstream>

#include <QApplication>

#include "main.hpp"
#include "render.hpp"

#include "input-mfem.hpp"


MainWindow::MainWindow(QWidget* gl)
{
    setCentralWidget(gl);
    setWindowTitle("hogtess");
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: hogtess <mesh> <solution>\n";
        return EXIT_FAILURE;
    }

    MFEMSolution solution(argv[1], argv[2]);

    MFEMSurfaceCoefs surfaceCoefs;
    surfaceCoefs.Extract(solution);

    /*const double* nodes =
       mfem::poly1d.ClosedPoints(order, mfem::Quadrature1D::GaussLobatto);*/

    QApplication app(argc, argv);

    /*QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    glf.setSamples(8);

    RenderWidget* gl =
         new RenderWidget(glf, mesh.GetNE(), order, nodes,
                          2, meshCoefs, 1, slnCoefs);

    MainWindow wnd(gl);
    gl->setParent(&wnd);

    QSize size(1200, 1000);
    wnd.resize(size);
    wnd.show();*/

    return app.exec();
}
