#line 2

// TODO: group size 32 in Z
layout(local_size_x = 1,
       local_size_y = 1,
       local_size_z = 1) in;

layout(std430, binding = 0) buffer coefBuffer
{
    vec4 coefs[];
};

layout(std430, binding = 1) buffer vertexBuffer
{
    vec4 vertices[];
};

uniform int level;
uniform float invLevel;

void main()
{
   const int ndof = (P+1)*(P+1);
   const int ntess = (level+1)*(level+1);

   uint faceIdx = gl_GlobalInvocationID.z;
   uint tessX = gl_GlobalInvocationID.x;
   uint tessY = gl_GlobalInvocationID.y;

   float u = tessX * invLevel;
   float v = tessY * invLevel;

   float ushape[P+1], vshape[P+1];
   lagrangeShape(u, ushape);
   lagrangeShape(v, vshape);

   vec4 value = vec4(0.0);
   for (int i = 0; i <= P; i++)
   for (int j = 0; j <= P; j++)
   {
       vec4 coef = coefs[faceIdx*ndof + (P+1)*i + j];
       value += ushape[i]*vshape[j]*coef;
   }

   vertices[faceIdx*ntess + tessY*(level+1) + tessX] = value;
}
