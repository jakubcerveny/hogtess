#include <glm/gtc/type_ptr.hpp>

#include "cutmesh.hpp"
#include "utility.hpp"
#include "shape/shape.hpp"
#include "palette.hpp"

#include "cutplane/draw.glsl.hpp"


void CutPlaneMesh::initializeGL(int order)
{
   const int version = 430;

   Definitions defs;
   defs("P", std::to_string(order))
       ("PALETTE_SIZE", std::to_string(RGB_Palette_3_Size));

   /*ShaderSource::list isoSurface{
      shaders::shape,
      shaders::surface::tesselate
   };*/

   /*progCompute.link(
      ComputeShader(version, computeSurface, defs));*/

   progDraw.link(
      VertexShader(version, {shaders::cutplane::draw}, defs),
      FragmentShader(version, {shaders::cutplane::draw}, defs));

   /*progLines.link(
      VertexShader(version, {shaders::surface::lines}, defs),
      FragmentShader(version, {shaders::surface::lines}, defs));*/

   // create an empty VAO
   glGenVertexArrays(1, &vao);
}


void CutPlaneMesh::calculate(const VolumeCoefs &coefs,
                             const glm::vec4 &clipPlane, int level)
{
   debug = &coefs;
}


void CutPlaneMesh::draw(const glm::mat4 &mvp, bool lines)
{
   progDraw.use();
   glUniformMatrix4fv(progDraw.uniform("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
   glUniform3fv(progDraw.uniform("palette"), RGB_Palette_3_Size,
                (const float*) RGB_Palette_3);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, debug->buffer());
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, debug->buffer());

   glBindVertexArray(vao);
   glDrawArrays(GL_POINTS, 0, 27*debug->numElements());
}


void CutPlaneMesh::deleteBuffers()
{
   GLuint buf[2] = { triangleBuffer, lineBuffer };
   glDeleteBuffers(2, buf);
}
