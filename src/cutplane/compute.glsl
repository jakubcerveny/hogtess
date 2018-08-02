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
   const int ndof = (P+1)*(P+1)*(P+1);
   const int ntess = (level+1)*(level+1)*(level+1);

   uint tessX = gl_GlobalInvocationID.x;
   uint tessY = gl_GlobalInvocationID.y;
   uint tessZ = gl_GlobalInvocationID.z % (level+1);
   uint elemIdx = gl_GlobalInvocationID.z / (level+1);
   uint base = elemIdx * ndof;

   float u = tessX * invLevel;
   float v = tessY * invLevel;
   float w = tessZ * invLevel;

   float ushape[P+1], vshape[P+1], wshape[P+1];
   lagrangeShape(u, ushape);
   lagrangeShape(v, vshape);
   lagrangeShape(w, wshape);

   vec4 value = vec4(0.0);
   for (int i = 0; i <= P; i++)
   for (int j = 0; j <= P; j++)
   for (int k = 0; k <= P; k++)
   {
       vec4 coef = coefs[base + (P+1)*((P+1)*i + j) + k];
       value += coef*ushape[i]*vshape[j]*wshape[k];
   }

   vertices[elemIdx*ntess + (level+1)*((level+1)*tessZ + tessY) + tessX] = value;
}
