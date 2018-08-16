#ifndef hogtess_buffer_hpp_included_
#define hogtess_buffer_hpp_included_

#include <GL/gl.h>


/** Represents a GL_SHADER_STORAGE_BUFFER (SSBO) allocated on the GPU.
 *  Deletes itself on destruction.
 */
class Buffer
{
public:
   Buffer(GLenum usage = GL_DYNAMIC_DRAW)
      : id_(0), size_(0), usage_(usage)
   {}

   ~Buffer() { discard(); }

   void bind(GLuint location)
   {
      genBind();
      glBindBufferBase(target, location, id_);
   }

   long size() const { return size_; }

   void resize(long size)
   {
      genBind();
      glBufferData(target, size, NULL, usage_);
      size_ = size;
   }

   void upload(const void* data, long size, long offset = 0)
   {
      genBind();
      if (offset + size > size_) {
         resize(offset + size);
      }
      glBufferSubData(target, offset, size, data);
   }

   void download(void* data, long size, long offset = 0)
   {
      genBind();
      glGetBufferSubData(target, offset, size, data);
   }

   void discard()
   {
      if (id_)
      {
         glDeleteBuffers(1, &id_);
         id_ = 0;
      }
   }

protected:
   enum { target = GL_SHADER_STORAGE_BUFFER };

   GLuint id_;
   GLenum usage_;
   GLsizeiptr size_;

   void genBind()
   {
      if (!id_) {
         glGenBuffers(1, &id_);
      }
      glBindBuffer(target, id_);
   }
};


#endif // hogtess_buffer_hpp_included_
