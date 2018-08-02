#include <glm/gtc/type_ptr.hpp>

#include "cutmesh.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "shape/shape.glsl.hpp"
#include "cutplane/compute.glsl.hpp"
#include "cutplane/draw.glsl.hpp"


void CutPlaneMesh::initializeGL(int order)
{
   const int version = 430;

   Definitions defs;
   defs("P", std::to_string(order))
       ("PALETTE_SIZE", std::to_string(RGB_Palette_3_Size));

   ShaderSource::list compute{
      shaders::shape,
      shaders::cutplane::compute
   };

   progCompute.link(
      ComputeShader(version, compute, defs));

   progDraw.link(
      VertexShader(version, {shaders::cutplane::draw}, defs),
      FragmentShader(version, {shaders::cutplane::draw}, defs));

   /*progLines.link(
      VertexShader(version, {shaders::surface::lines}, defs),
      FragmentShader(version, {shaders::surface::lines}, defs));*/

   // create an empty VAO
   glGenVertexArrays(1, &vao);
}


void CutPlaneMesh::compute(const VolumeCoefs &coefs,
                           const glm::vec4 &clipPlane, int level)
{
   deleteBuffers();

   numElems = coefs.numElements();
   subdivLevel = level;

   int elemVerts = cube(level+1);
   long bufSize = 4*sizeof(float)*elemVerts*numElems;

   glGenBuffers(1, &vertexBuffer);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glBufferData(GL_SHADER_STORAGE_BUFFER, bufSize, NULL, GL_STATIC_DRAW);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexBuffer);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, coefs.buffer());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, coefs.buffer());

   progCompute.use();
   glUniform1i(progCompute.uniform("level"), level);
   glUniform1f(progCompute.uniform("invLevel"), 1.0 / level);

   lagrangeUniforms(progCompute, solution.order(), solution.nodes1d());

   // launch the compute shader
   // TODO: group size 32 in Z
   glDispatchCompute(level+1, level+1, (level+1)*numElems);

   // wait until we can use the computed vertices
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}


void CutPlaneMesh::draw(const glm::mat4 &mvp, bool lines)
{
   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexBuffer);

   int nv = cube(subdivLevel + 1)*numElems;

   glBindVertexArray(vao);
   glDrawArrays(GL_POINTS, 0, nv);
}


void CutPlaneMesh::deleteBuffers()
{
   GLuint buf[2] = { vertexBuffer, lineBuffer };
   glDeleteBuffers(2, buf);
}
