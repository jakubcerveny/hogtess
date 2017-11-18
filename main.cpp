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


int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: hogtess <mesh> <solution>\n";
        return EXIT_FAILURE;
    }

    std::cout << "Loading mesh: " << argv[1] << std::endl;
    mfem::Mesh mesh(argv[1]);

    std::cout << "Loading solution: " << argv[2] << std::endl;
    std::ifstream is(argv[2]);
    mfem::GridFunction gridfn(&mesh, is);
    is.close();

    QApplication app(argc, argv);

    QSize size(1200, 1000);
    RenderWidget* gl = new RenderWidget();
    MainWindow wnd(gl);
    gl->setParent(&wnd);
    wnd.resize(size);
    wnd.show();

    return app.exec();
}
