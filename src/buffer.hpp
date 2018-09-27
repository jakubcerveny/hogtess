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
      : id_(0), size_(0), usage_(usage), cpuCopy_(nullptr)
   {}

   ~Buffer() { discard(); }

   /// Bind buffer to the given location.
   void bind(GLuint location) const
   {
      genBind();
      glBindBufferBase(target, location, id_);
   }

   /// Return current buffer size.
   long size() const { return size_; }

   /// Allocate the buffer on the GPU.
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

   /// Upload data to the GPU.
   void upload(const void* data, long size)
   {
      genBind();
      if (size > size_) {
         resize(size);
      }
      glBufferSubData(target, 0, size, data);
      discardCopy();
   }

   /// Upload helper for an std::vector.
   template<typename T>
   void upload(const std::vector<T> &data)
   {
      upload(data.data(), data.size()*sizeof(T));
   }

   /// Download data from the GPU.
   void download(void* data, long size) const
   {
      genBind();
      glGetBufferSubData(target, 0, size, data);
   }

   /// Store a CPU copy of the data.
   void copy(const void* data, long size)
   {
       discardCopy();
       cpuCopy_ = new char[size];
       std::memcpy(cpuCopy_, data, size);
   }

   /// Copy helper for an std::vector.
   template<typename T>
   void copy(const std::vector<T> &data)
   {
      copy(data.data(), data.size()*sizeof(T));
   }

   /// Access CPU copy of the data. Return nullptr if CPU copy not present.
   const void* data() const { return cpuCopy_; }

   /// Access CPU copy as "T array[index]".
   template<typename T>
   const T& data(int index) const { return ((const T*) cpuCopy_)[index]; }

   /// Free buffer and CPU copy.
   void discard()
   {
      if (id_)
      {
         glDeleteBuffers(1, &id_);
         id_ = 0;
         size_ = 0;
      }
      discardCopy();
   }

protected:
   enum { target = GL_SHADER_STORAGE_BUFFER };

   mutable GLuint id_;
   GLenum usage_;
   GLsizeiptr size_;
   char* cpuCopy_;

   void genBind() const
   {
      if (!id_) {
         glGenBuffers(1, &id_);
      }
      glBindBuffer(target, id_);
   }

   void discardCopy()
   {
      delete [] cpuCopy_;
      cpuCopy_ = nullptr;
   }
};


#endif // hogtess_buffer_hpp_included_
