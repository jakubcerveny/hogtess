#ifndef hogtess_buffer_hpp_included_
#define hogtess_buffer_hpp_included_

//#include <iostream>
#include <vector>

#include <GL/gl.h>


/** Represents a GL_SHADER_STORAGE_BUFFER (SSBO) allocated on the GPU.
 *  Deletes itself on destruction.
 */
class Buffer
{
public:
   Buffer(GLenum usage = GL_STREAM_COPY)
      : id_(0), size_(0), usage_(usage)
   {}

   ~Buffer() { discard(); }

   void bind(GLuint location) const
   {
      genBind();
      glBindBufferBase(target, location, id_);
   }

   long size() const { return size_; }

   void resize(long size)
   {
      if (size != size_)
      {
         genBind();
         //std::cout << "Allocating buffer, size " << size << std::endl;
         glBufferData(target, size, NULL, usage_);
         // TODO: error checking
         size_ = size;
      }
   }

   void upload(const void* data, long size, long offset = 0)
   {
      genBind();
      if (offset + size > size_)
      {
         resize(offset + size);
      }
      glBufferSubData(target, offset, size, data);
   }

   template<typename T>
   void upload(const std::vector<T> &data, long offset = 0)
   {
      upload(data.data(), data.size()*sizeof(T), offset);
   }

   void download(void* data, long size, long offset = 0) const
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
         size_ = 0;
      }
   }

protected:
   enum { target = GL_SHADER_STORAGE_BUFFER };

   mutable GLuint id_;
   GLenum usage_;
   GLsizeiptr size_;

   void genBind() const
   {
      if (!id_) {
         glGenBuffers(1, &id_);
      }
      glBindBuffer(target, id_);
   }
};


#endif // hogtess_buffer_hpp_included_
