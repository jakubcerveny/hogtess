#line 2

// TODO: sizes?
layout(local_size_x = 1,
       local_size_y = 1,
       local_size_z = 1) in;

layout(std430, binding = 0) buffer bufVertices
{
   vec4 vertices[];
};

layout(std430, binding = 1) buffer bufTables
{
   int edgeTable[256];
   int triTable[256][16];
};

layout(std430, binding = 2) buffer bufTriangles
{
   vec4 outVertices[];
};

layout(std430, binding = 3) buffer bufLines
{
   vec4 outLines[];
};

layout(std430, binding = 4) buffer bufCounters
{
   uint totalVertices, totalLines;
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
   float dist[8];
   vec4 vertex[12];

   int level1 = level+1;
   uint elemVerts = level1*level1*level1;
   uint elemIdx = gl_GlobalInvocationID.z / level;
   uvec3 xyz = uvec3(gl_GlobalInvocationID.xy,
                     gl_GlobalInvocationID.z % level);

   uint cubeIndex = 0;
   for (uint i = 0, bit = 1; i < 8; i++, bit *= 2)
   {
      uvec3 v = (xyz + cornerXYZ[i]);
      corner[i] = vertices[elemIdx*elemVerts + level1*(level1*v.z + v.y) + v.x];
      dist[i] = planeDistance(corner[i]);

      if (dist[i] < 0) {
         cubeIndex |= bit;
      }
   }

   // voxel completely in/out of the surface?
   int edgeMask = edgeTable[cubeIndex];
   if (edgeMask == 0) {
      return;
   }

   // for each vertex, check if it touches the 6 boundary planes of the element
   uint vmask[8], emask[12];
   uvec3 lo = uvec3(0, 0, 0);
   uvec3 hi = uvec3(level, level, level);

   for (uint i = 0; i < 8; i++)
   {
      uvec3 v = (xyz + cornerXYZ[i]);
      vmask[i] = uint(dot(uvec3(equal(v, lo)), uvec3(1, 2, 4))) +
                 uint(dot(uvec3(equal(v, hi)), uvec3(8, 16, 32)));
   }

   // interpolate isosurface vertices along cube edges
   if ((edgeMask & 1) != 0) {
      vertex[0] = interpolate(corner[0], corner[1], dist[0], dist[1]);
      emask[0] = vmask[0] & vmask[1];
   }
   if ((edgeMask & 2) != 0) {
      vertex[1] = interpolate(corner[1], corner[2], dist[1], dist[2]);
      emask[1] = vmask[1] & vmask[2];
   }
   if ((edgeMask & 4) != 0) {
      vertex[2] = interpolate(corner[2], corner[3], dist[2], dist[3]);
      emask[2] = vmask[2] & vmask[3];
   }
   if ((edgeMask & 8) != 0) {
      vertex[3] = interpolate(corner[3], corner[0], dist[3], dist[0]);
      emask[3] = vmask[3] & vmask[0];
   }
   if ((edgeMask & 16) != 0) {
      vertex[4] = interpolate(corner[4], corner[5], dist[4], dist[5]);
      emask[4] = vmask[4] & vmask[5];
   }
   if ((edgeMask & 32) != 0) {
      vertex[5] = interpolate(corner[5], corner[6], dist[5], dist[6]);
      emask[5] = vmask[5] & vmask[6];
   }
   if ((edgeMask & 64) != 0) {
      vertex[6] = interpolate(corner[6], corner[7], dist[6], dist[7]);
      emask[6] = vmask[6] & vmask[7];
   }
   if ((edgeMask & 128) != 0) {
      vertex[7] = interpolate(corner[7], corner[4], dist[7], dist[4]);
      emask[7] = vmask[7] & vmask[4];
   }
   if ((edgeMask & 256) != 0) {
      vertex[8] = interpolate(corner[0], corner[4], dist[0], dist[4]);
      emask[8] = vmask[0] & vmask[4];
   }
   if ((edgeMask & 512) != 0) {
      vertex[9] = interpolate(corner[1], corner[5], dist[1], dist[5]);
      emask[9] = vmask[1] & vmask[5];
   }
   if ((edgeMask & 1024) != 0) {
      vertex[10] = interpolate(corner[2], corner[6], dist[2], dist[6]);
      emask[10] = vmask[2] & vmask[6];
   }
   if ((edgeMask & 2048) != 0) {
      vertex[11] = interpolate(corner[3], corner[7], dist[3], dist[7]);
      emask[11] = vmask[3] & vmask[7];
   }

   uint nv = triTable[cubeIndex][15];
   uint pos = atomicAdd(totalVertices, nv);

   uint nl = 0;
   vec4 tmpLines[12];

   for (uint i = 0; i < nv; )
   {
      int a = triTable[cubeIndex][i++];
      int b = triTable[cubeIndex][i++];
      int c = triTable[cubeIndex][i++];

      outVertices[pos++] = vertex[a];
      outVertices[pos++] = vertex[b];
      outVertices[pos++] = vertex[c];

      if ((emask[a] & emask[b]) != 0) {
         tmpLines[nl++] = vertex[a];
         tmpLines[nl++] = vertex[b];
      }
      if ((emask[b] & emask[c]) != 0) {
         tmpLines[nl++] = vertex[b];
         tmpLines[nl++] = vertex[c];
      }
      if ((emask[c] & emask[a]) != 0) {
         tmpLines[nl++] = vertex[c];
         tmpLines[nl++] = vertex[a];
      }
   }

   if (nl != 0)
   {
      pos = atomicAdd(totalLines, nl);

      for (uint i = 0; i < nl; i++) {
         outLines[pos++] = tmpLines[i];
      }
   }
}
