#include <iostream>

#include <QApplication>

#include "main.hpp"
#include "render.hpp"


MainWindow::MainWindow(QWidget* gl)
{
    setCentralWidget(gl);
    setWindowTitle("hogtess");
}


int main(int argc, char *argv[])
{
    /*if (argc < 2) {
        std::cout << "Usage: hogtess <mesh> <solution>\n";
        return EXIT_FAILURE;
    }*/

    QApplication app(argc, argv);

    QSize size(1200, 1000);
    RenderWidget* gl = new RenderWidget();
    MainWindow wnd(gl);
    gl->setParent(&wnd);
    wnd.resize(size);
    wnd.show();

    return app.exec();
}
