#line 2

// TODO: sizes?
layout(local_size_x = 1,
       local_size_y = 1,
       local_size_z = 1) in;

layout(std430, binding = 0) buffer voxelBuffer
{
   vec4 vertices[];
};

layout(std430, binding = 1) buffer tablesBuffer
{
   int edgeTable[256];
   int triTable[256][16];
};

layout(std430, binding = 2) buffer triangleBuffer
{
   vec4 outVertices[];
};

layout(std430, binding = 3) buffer counterBuffer
{
   uint totalVertices;
};

uniform vec4 clipPlane;
uniform int level;

const uvec3 cornerXYZ[8] =
{
   uvec3(0, 0, 0),
   uvec3(1, 0, 0),
   uvec3(1, 1, 0),
   uvec3(0, 1, 0),
   uvec3(0, 0, 1),
   uvec3(1, 0, 1),
   uvec3(1, 1, 1),
   uvec3(0, 1, 1)
};

vec4 interpolate(vec4 p1, vec4 p2, float val1, float val2)
{
   return mix(p1, p2, val1/(val1 - val2));
}

float planeDistance(vec4 pt)
{
   return dot(clipPlane, vec4(pt.xyz, 1));
}


void main()
{
   vec4 corner[8];
   float value[8];
   vec4 vertex[12];
   bool bnd[8], bflag[12];

   int level1 = level+1;
   uint elemVert = level1*level1*level1;
   uint elemIdx = gl_GlobalInvocationID.z / level;
   uvec3 xyz = uvec3(gl_GlobalInvocationID.xy, gl_GlobalInvocationID.z % level);

   uvec3 lo = uvec3(0, 0, 0);
   uvec3 hi = uvec3(level, level, level);

   uint cubeIndex = 0;
   for (uint i = 0, bit = 1; i < 8; i++, bit *= 2)
   {
      uvec3 v = (xyz + cornerXYZ[i]);
      corner[i] = vertices[elemIdx*elemVert + level1*(level1*v.z + v.y) + v.x];
      value[i] = planeDistance(corner[i]);
      bnd[i] = any(equal(v, lo) || equal(v, hi));

      if (value[i] < 0) {
         cubeIndex |= bit;
      }
   }

   int edgeMask = edgeTable[cubeIndex];
   if (edgeMask == 0) {
      // voxel completely in/out of the surface
      return;
   }

   if ((edgeMask & 1) != 0) {
      vertex[0] = interpolate(corner[0], corner[1], value[0], value[1]);
      bflag[0] = bnd[0] || bnd[1];
   }
   if ((edgeMask & 2) != 0) {
      vertex[1] = interpolate(corner[1], corner[2], value[1], value[2]);
      bflag[1] = bnd[1] || bnd[2];
   }
   if ((edgeMask & 4) != 0) {
      vertex[2] = interpolate(corner[2], corner[3], value[2], value[3]);
      bflag[2] = bnd[2] || bnd[3];
   }
   if ((edgeMask & 8) != 0) {
      vertex[3] = interpolate(corner[3], corner[0], value[3], value[0]);
      bflag[3] = bnd[3] || bnd[0];
   }
   if ((edgeMask & 16) != 0) {
      vertex[4] = interpolate(corner[4], corner[5], value[4], value[5]);
      bflag[4] = bnd[4] || bnd[5];
   }
   if ((edgeMask & 32) != 0) {
      vertex[5] = interpolate(corner[5], corner[6], value[5], value[6]);
      bflag[5] = bnd[5] || bnd[6];
   }
   if ((edgeMask & 64) != 0) {
      vertex[6] = interpolate(corner[6], corner[7], value[6], value[7]);
      bflag[6] = bnd[6] || bnd[7];
   }
   if ((edgeMask & 128) != 0) {
      vertex[7] = interpolate(corner[7], corner[4], value[7], value[4]);
      bflag[7] = bnd[7] || bnd[4];
   }
   if ((edgeMask & 256) != 0) {
      vertex[8] = interpolate(corner[0], corner[4], value[0], value[4]);
      bflag[8] = bnd[0] || bnd[4];
   }
   if ((edgeMask & 512) != 0) {
      vertex[9] = interpolate(corner[1], corner[5], value[1], value[5]);
      bflag[9] = bnd[1] || bnd[5];
   }
   if ((edgeMask & 1024) != 0) {
      vertex[10] = interpolate(corner[2], corner[6], value[2], value[6]);
      bflag[10] = bnd[2] || bnd[6];
   }
   if ((edgeMask & 2048) != 0) {
      vertex[11] = interpolate(corner[3], corner[7], value[3], value[7]);
      bflag[11] = bnd[3] || bnd[7];
   }

   uint nv = triTable[cubeIndex][15];
   uint pos = atomicAdd(totalVertices, nv);

   for (int i = 0; i < 12; i++) {
      if (bflag[i]) { vertex[i].w = 0; }
   }

   for (uint i = 0; i < nv; i++) {
      outVertices[pos++] = vertex[triTable[cubeIndex][i]];
   }
}
