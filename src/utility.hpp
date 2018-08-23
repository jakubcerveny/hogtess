#ifndef hogtess_utility_hpp_included__
#define hogtess_utility_hpp_included__

#include <string>


template<typename T>
inline T sqr(T x) { return x*x; }

template<typename T>
inline T cube(T x) { return x*x*x; }


template<typename Int>
inline Int divRoundUp(Int x, Int div)
{
   return (x + div-1) / div;
}

template<typename Int>
inline Int roundUpMultiple(Int x, Int mul)
{
   return divRoundUp(x, mul) * mul;
}


void tic();
double toc();


#define OMP(x)


std::string format_str(const char* fmt, ...);


#endif // hogtess_utility_hpp_included__
