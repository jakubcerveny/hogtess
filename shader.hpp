#ifndef SHADER_HPP_INCLUDED
#define SHADER_HPP_INCLUDED

#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <utility>
#include <memory>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif


namespace
{
inline std::string fetchGLError(const std::string &def = std::string())
{
   std::string val;

   while (GLenum err = glGetError())
   {
      if (!val.empty()) {
         val.append(", ");
      }

      switch (err) {
      case GL_INVALID_ENUM: val += "GL_INVALID_ENUM"; break;
      case GL_INVALID_VALUE: val += "GL_INVALID_VALUE"; break;
      case GL_INVALID_OPERATION: val += "GL_INVALID_OPERATION"; break;
      case GL_OUT_OF_MEMORY: val += "GL_OUT_OF_MEMORY"; break;
      default: val += "(unknown)";
      };
   }

   return val.empty() ? def : val;
}
}


struct GLError : public std::runtime_error
{
   GLError(const std::string &msg)
      : std::runtime_error(msg + ": " + fetchGLError("no error"))
   {}

   GLError(const std::string &msg, const std::string &error)
      : std::runtime_error(msg + ": " + error)
   {}
};


inline void checkGLError(const std::string &msg)
{
   std::string e(fetchGLError());
   if (!e.empty()) {
      throw GLError(msg, e);
   }
}


struct ShaderError : public std::runtime_error {
   ShaderError(const std::string &msg) : std::runtime_error(msg) {}
};

struct ProgramError : public std::runtime_error {
   ProgramError(const std::string &msg) : std::runtime_error(msg) {}
};


template<GLenum Kind>
class Shader
{
public:

   Shader() {}

   template<typename T, int size>
   Shader(const T(&data)[size]) {
      load(data, size * sizeof(T));
   }

   Shader(const std::string &src) {
      load(src);
   }

   Shader(const char *src) {
      load(src, strlen(src));
   }

   void load(const std::string &src) {
      load(src.data(), src.length());
   }

   template<typename T, int size>
   void load(const T(&data)[size]) {
      load(data, size * sizeof(T));
   }

   GLuint get() const {
      return shader_ ? *shader_ : 0;
   }

   operator GLuint() const {
      return get();
   }

private:

   void load(const void *data, int size)
   {
      shader_ptr shader(new GLuint(0), &Shader::deleter);

      // having valid pointer we can safely create shader
      *shader = glCreateShader(Kind);
      if (!*shader) {
         throw GLError("cannot create shader");
      }

      const char *defs = "";
      switch (Kind) {
      case GL_VERTEX_SHADER:
         defs = "#define _VERTEX_ 1\n";
         break;
      case GL_FRAGMENT_SHADER:
         defs = "#define _FRAGMENT_ 1\n";
         break;
      }

      const GLchar* src[2] = {
         defs,
         static_cast<const GLchar*>(data),
      };
      const GLint len[2] = {
         -1,
         size
      };

      /*for (int i = 0; i < l; i++)
            std::cout << d[i];*/

      glShaderSource(*shader, GLsizei(2),
                     (const GLchar**) &src, (const GLint*) &len);

      glCompileShader(*shader);

      GLint compiled;
      glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

      if (!compiled)
      {
         GLint il = 0;
         glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &il);
         if (il > 1) {
            std::vector<char> log;
            log.resize(il);

            glGetShaderInfoLog(*shader, il, 0x0, &log[0]);
            throw ShaderError
                  (std::string("cannot compile shader: ") + &log[0]);
         }
         throw ShaderError("cannot compile shader");
      }

      // everything went OK, we can replace shader
      shader_ = shader;
   }

   static void deleter(const GLuint *s)
   {
      if (s && *s) {
         glDeleteShader(*s);
      }
   }

   typedef std::shared_ptr<GLuint> shader_ptr;
   shader_ptr shader_;
};


typedef Shader<GL_VERTEX_SHADER> VertexShader;
typedef Shader<GL_FRAGMENT_SHADER> FragmentShader;


struct Attributes
{
   Attributes() {}

   Attributes(GLuint index, const GLchar *name) {
      attrs.push_back(Attr(index, name));
   }

   Attributes& operator()(GLuint index, const GLchar *name) {
      attrs.push_back(Attr(index, name));
      return *this;
   }

   typedef std::pair<GLuint, const GLchar *> Attr;
   typedef std::vector<Attr> AttrVector;
   AttrVector attrs;
};


class Program
{
public:
   Program() {}

   void link(const VertexShader &vs, const FragmentShader &fs
             , const Attributes &attrs = Attributes())
   {
      // create invalid program
      ProgramId id(new GLuint(0), &Program::deleter);

      // having valid pointer we can safely create program
      *id = glCreateProgram();
      if (!*id) throw GLError("cannot create shader");

      glAttachShader(*id, vs);
      glAttachShader(*id, fs);

      for (const Attributes::Attr &a : attrs.attrs) {
         glBindAttribLocation(*id, a.first, a.second);
      }

      glLinkProgram(*id);

      GLint linked;
      glGetProgramiv(*id, GL_LINK_STATUS, &linked);

      if (!linked)
      {
         GLint il = 0;
         glGetProgramiv(*id, GL_INFO_LOG_LENGTH, &il);
         if (il > 1) {
            std::vector<char> log;
            log.resize(il);

            glGetProgramInfoLog(*id, il, 0x0, &log[0]);
            throw ProgramError
                  (std::string("cannot link program: ") + &log[0]);
         }
         throw ProgramError("cannot link program");
      }

      programId_ = id;
      vs_ = vs;
      fs_ = fs;
   }

   GLuint get() const {
      return programId_ ? *programId_ : 0;
   }

   operator GLuint() const {
      return get();
   }

   void use() const {
      glUseProgram(get());
   }

   void stop() const {
      glUseProgram(0);
   }

   GLint uniform(const char *name) const {
      return glGetUniformLocation(get(), name);
   }

   GLint uniform(const std::string &name) const {
      return glGetUniformLocation(get(), name.c_str());
   }

   GLint attribute(const char *name) const {
      return glGetAttribLocation(get(), name);
   }

   GLint attribute(const std::string &name) const {
      return glGetAttribLocation(get(), name.c_str());
   }

private:

   static void deleter(const GLuint *p) {
      if (p && *p) {
         glDeleteProgram(*p);
      }
   }

   VertexShader vs_;
   FragmentShader fs_;

   typedef std::shared_ptr<GLuint> ProgramId;
   ProgramId programId_;
};


#endif // SHADER_HPP_INCLUDED
