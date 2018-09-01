#line 2

layout(local_size_x = 32,
       local_size_y = 1,
       local_size_z = 1) in;

layout(std430, binding = 0) buffer bufCoefs
{
   vec4 coefs[];
};

layout(std430, binding = 1) buffer bufElemIndices
{
   uint elemIndices[];
};

layout(std430, binding = 2) buffer bufVertices
{
   vec4 vertices[];
};

layout(std430, binding = 3) buffer bufRanks
{
   int elemRank[];
};

layout(std430, binding = 4) buffer bufPartMat
{
   mat4 matrices[];
};


uniform int level;
uniform float invLevel;
uniform int numElems;

void main()
{
   const int ndof = (P+1)*(P+1)*(P+1);
   const int elemVert = (level+1)*(level+1)*(level+1);

   uint tessX = gl_GlobalInvocationID.x % (level+1);
   uint tessY = gl_GlobalInvocationID.y;
   uint tessZ = gl_GlobalInvocationID.z;
   uint elemIdx = gl_GlobalInvocationID.x / (level+1);

   if (elemIdx >= numElems) { return; }

   uint coefBase = elemIndices[elemIdx] * ndof;

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
       vec4 coef = coefs[coefBase + (P+1)*((P+1)*i + j) + k];
       value += coef*ushape[i]*vshape[j]*wshape[k];
   }

   vec4 pos = matrices[elemRank[elemIndices[elemIdx]]] * vec4(value.xyz, 1);
   value.xyz = pos.xyz;

   vertices[elemIdx*elemVert + (level+1)*((level+1)*tessZ + tessY) + tessX] = value;
}
