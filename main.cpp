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
    if (argc < 2) {
        std::cout << "Usage: hogtess <mesh> <solution>\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> files;
    std::set<int> nodes, elements;

/*    for (int i = 1; i < argc; i++) {
       std::string arg(argv[i]);
       if (arg == "--node") {
          nodes.insert(atol(argv[++i]));
       }
       else if (arg == "--element") {
          elements.insert(atol(argv[++i]));
       }
       else {
         files.push_back(arg);
       }
    }*/

    // run the browser!
    QApplication app(argc, argv);

    QSize size(1200, 1000);
    RenderWidget* gl = new RenderWidget(files, nodes, elements);
    MainWindow wnd(gl);
    gl->setParent(&wnd);
    wnd.resize(size);
    wnd.show();

    return app.exec();
}
