#include "shader.hpp"
#include "shape.hpp"


void lagrangeUniforms(const Program &prog, int p, const double *nodalPoints)
{
   int p1 = p+1;

   double weights[p1];
   float fweights[p1];
   float fnodes[p1];

   for (int i = 0; i <= p; i++)
   {
      weights[i] = 1.0;
   }
   for (int i = 0; i <= p; i++)
   {
      for (int j = 0; j < i; j++)
      {
         double xij = nodalPoints[i] - nodalPoints[j];
         weights[i] *=  xij;
         weights[j] *= -xij;
      }
   }
   for (int i = 0; i <= p; i++)
   {
      fnodes[i] = nodalPoints[i];
      fweights[i] = 1.0 / weights[i];
   }

   glUniform1fv(prog.uniform("lagrangeNodes"), p1, fnodes);
   glUniform1fv(prog.uniform("lagrangeWeights"), p1, fweights);
}

