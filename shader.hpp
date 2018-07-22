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


struct ShaderSource
{
   const char* data;
   int length;

   ShaderSource(const char *src)
      : data(src), length(strlen(src)) {}

   ShaderSource(const std::string &src)
      : data(src.data()), length(src.length()) {}

   template<typename T, int size>
   ShaderSource(const T(&data)[size])
      : data((const char*) data), length(size) {}

   typedef std::vector<ShaderSource> list;
};


struct Definitions
{
   Definitions() {}

   template<typename A, typename B>
   Definitions(const A &name, const B &value) {
      defs.push_back(Def(name, value));
   }

   template<typename A, typename B>
   Definitions& operator()(const A &name, const B &value) {
      defs.push_back(Def(name, value));
      return *this;
   }

   typedef std::pair<std::string, std::string> Def;
   typedef std::vector<Def> DefVector;
   DefVector defs;

   std::string toString() const
   {
      std::string result;
      for (const auto &pair : defs) {
         result += std::string("#define ") + pair.first + " " + pair.second + "\n";
      }
      return result;
   }
};


template<GLenum Kind>
class Shader
{
public:
   Shader() {}

   Shader(int version, const ShaderSource::list &sources,
          const Definitions &defs = Definitions())
   {
      load(version, sources, defs);
   }

   GLuint get() const {
      return shader_ ? *shader_ : 0;
   }

   bool valid() const {
      return shader_ != nullptr;
   }

   operator GLuint() const {
      return get();
   }

   void load(int version, const ShaderSource::list sources,
             const Definitions &defs = Definitions())
   {
      shader_ptr shader(new GLuint(0), &Shader::deleter);

      *shader = glCreateShader(Kind);
      if (!*shader) {
         throw GLError(std::string("Cannot create shader ") + glKindName());
      }

      std::vector<const GLchar*> strings(sources.size() + 1);
      std::vector<GLint> lengths(strings.size());

      char buf[200];
      sprintf(buf, "#version %d\n#define %s 1\n", version, glslDefName());

      std::string preamble = std::string(buf) + defs.toString();
      strings[0] = preamble.data();
      lengths[0] = preamble.length();

      for (unsigned i = 0; i < sources.size(); i++) {
         strings[i+1] = sources[i].data;
         lengths[i+1] = sources[i].length;
      }

      glShaderSource(*shader, strings.size(), strings.data(), lengths.data());

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
                  (std::string("Error compiling shader:\n") + &log[0]);
         }
         throw ShaderError("cannot compile shader");
      }

      // everything went OK, we can replace shader
      shader_ = shader;
   }

protected:
   static void deleter(const GLuint *s)
   {
      if (s && *s) {
         glDeleteShader(*s);
      }
   }

   static const char* glKindName()
   {
      switch (Kind) {
         case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
         case GL_TESS_CONTROL_SHADER: return "GL_TESS_CONTROL_SHADER";
         case GL_TESS_EVALUATION_SHADER: return "GL_TESS_EVALUATION_SHADER";
         case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
         case GL_COMPUTE_SHADER: return "GL_COMPUTE_SHADER";
         default: return "???";
      }
   }

   static const char* glslDefName()
   {
      switch (Kind) {
         case GL_VERTEX_SHADER: return "_VERTEX_";
         case GL_TESS_CONTROL_SHADER: return "_TESS_CONTROL_";
         case GL_TESS_EVALUATION_SHADER: return "_TESS_EVAL_";
         case GL_FRAGMENT_SHADER: return "_FRAGMENT_";
         case GL_COMPUTE_SHADER: return "_COMPUTE_";
         default: return "???";
      }
   }

   typedef std::shared_ptr<GLuint> shader_ptr;
   shader_ptr shader_;
};

typedef Shader<GL_VERTEX_SHADER> VertexShader;
typedef Shader<GL_TESS_CONTROL_SHADER> TessControlShader;
typedef Shader<GL_TESS_EVALUATION_SHADER> TessEvalShader;
typedef Shader<GL_FRAGMENT_SHADER> FragmentShader;
typedef Shader<GL_COMPUTE_SHADER> ComputeShader;


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
      link(vs, {}, {}, fs, {}, attrs);
   }

   void link(const VertexShader &vs, const TessControlShader &tcs,
             const TessEvalShader &tes, const FragmentShader &fs
             , const Attributes &attrs = Attributes())
   {
      link(vs, tcs, tes, fs, {}, attrs);
   }

   void link(const ComputeShader &cs
             , const Attributes &attrs = Attributes())
   {
      link({}, {}, {}, {}, cs, attrs);
   }

   void link(const VertexShader &vs, const TessControlShader &tcs,
             const TessEvalShader &tes, const FragmentShader &fs,
             const ComputeShader &cs
             , const Attributes &attrs = Attributes())
   {
      // create invalid program
      ProgramId id(new GLuint(0), &Program::deleter);

      // having valid pointer we can safely create program
      *id = glCreateProgram();
      if (!*id) throw GLError("cannot create shader");

      if (vs.valid()) {
         glAttachShader(*id, vs);
      }
      if (tcs.valid()) {
         glAttachShader(*id, tcs);
      }
      if (tes.valid()) {
         glAttachShader(*id, tes);
      }
      if (fs.valid()) {
         glAttachShader(*id, fs);
      }
      if (cs.valid()) {
         glAttachShader(*id, cs);
      }

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
      tcs_ = tcs;
      tes_ = tes;
      fs_ = fs;
      cs_ = cs;
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

   /// Return local work group size of a compute shader.
   void localSize(GLint lsize[3]) {
      glGetProgramiv(get(), GL_COMPUTE_WORK_GROUP_SIZE, lsize);
   }

private:

   static void deleter(const GLuint *p) {
      if (p && *p) {
         glDeleteProgram(*p);
      }
   }

   VertexShader vs_;
   TessControlShader tcs_;
   TessEvalShader tes_;
   FragmentShader fs_;
   ComputeShader cs_;

   typedef std::shared_ptr<GLuint> ProgramId;
   ProgramId programId_;
};


#endif // SHADER_HPP_INCLUDED
