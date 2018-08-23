#include <fstream>

#include <QApplication>

#include "main.hpp"
#include "render.hpp"
#include "utility.hpp"

#include "input/input-mfem.hpp"

#include "3rdparty/argagg.hpp"


MainWindow::MainWindow(QWidget* gl)
{
   setCentralWidget(gl);
   setWindowTitle("hogtess");
}


int main(int argc, char *argv[])
{
   argagg::parser argparser
   {{
      { "help", {"-h", "--help"},
         "Shows this help message.", 0},
      { "mesh", {"-m", "--mesh"},
         "Mesh file to visualize.", 1},
      { "gf", {"-g", "--grid-function"},
         "Solution (GridFunction) file to visualize.", 1},
      { "np", {"-n", "--num-proc"},
         "Load mesh/solution from multiple processors.", 1}
   }};

   argagg::parser_results args;
   try
   {
      args = argparser.parse(argc, argv);
   }
   catch (const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
   }

   if (args["help"])
   {
      std::cerr << "Usage: hogtess [options]" << std::endl << argparser;
      return EXIT_SUCCESS;
   }
   if (!args["mesh"])
   {
      std::cerr << "--mesh (-m) argument is required." << std::endl;
      return EXIT_FAILURE;
   }
   if (!args["gf"])
   {
      std::cerr << "--grid-function (-g) argument is required." << std::endl;
      return EXIT_FAILURE;
   }

   std::string argMesh = args["mesh"].as<std::string>("");
   std::string argGF = args["gf"].as<std::string>("");

   std::vector<std::string> meshPaths, gfPaths;

   if (args["np"])
   {
      int numProc = args["np"].as<int>(1);
      for (int n = 0; n < numProc; n++)
      {
         meshPaths.push_back(format_str("%s.%06d", argMesh.c_str(), n));
         gfPaths.push_back(format_str("%s.%06d", argGF.c_str(), n));
      }
   }
   else
   {
      meshPaths = {argMesh};
      gfPaths = {argGF};
   }

   MFEMSolution solution(meshPaths, gfPaths);
   MFEMSurfaceCoefs surfaceCoefs;
   MFEMVolumeCoefs volumeCoefs;

   QApplication app(argc, argv);

   QGLFormat glf = QGLFormat::defaultFormat();
   glf.setSampleBuffers(true);
   glf.setSamples(8);

   RenderWidget* gl =
      new RenderWidget(glf, solution, surfaceCoefs, volumeCoefs);

   MainWindow wnd(gl);
   gl->setParent(&wnd);

   QSize size(1200, 1000);
   wnd.resize(size);
   wnd.show();

   return app.exec();
}
