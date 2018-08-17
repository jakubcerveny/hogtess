#include <vector>
#include <chrono>

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

