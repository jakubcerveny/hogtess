#include <vector>
#include <chrono>
#include <memory>

#include <cstring>
#include <cstdlib>
#include <cstdarg>

#include "utility.hpp"

namespace chrono = std::chrono;

static std::vector<chrono::time_point<chrono::steady_clock>> timeStack;

void tic()
{
   timeStack.push_back(chrono::steady_clock::now());
}

double toc()
{
   auto end = std::chrono::steady_clock::now();

   if (timeStack.empty()) {
      return -1;
   }
   auto start = timeStack.back();
   timeStack.pop_back();

   std::chrono::duration<double> diff = end-start;

   return diff.count();
}


std::string format_str(const char* fmt, ...)
{
   // reserve two times as much as the length of the fmt
   int n = 2*std::strlen(fmt);
   std::unique_ptr<char> formatted;

   va_list ap;
   while (1)
   {
      formatted.reset(new char[n]);

      va_start(ap, fmt);
      int final_n = vsnprintf(formatted.get(), n, fmt, ap);
      va_end(ap);

      if (final_n < 0 || final_n >= n) {
         n += std::abs(final_n - n + 1);
      }
      else {
         break;
      }
   }
   return std::string(formatted.get());
}
