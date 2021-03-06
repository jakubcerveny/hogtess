#line 2
#if _VERTEX_

uniform mat4 mvp;
uniform int nFaceVert;
uniform vec4 clipPlane;

layout(std430, binding = 0) buffer bufVertices
{
   vec4 vertices[];
};

layout(std430, binding = 1) buffer bufLineIndices
{
   int indices[];
};

layout(std430, binding = 2) buffer bufRanks
{
   int faceRank[];
};

layout(std430, binding = 3) buffer bufPartMat
{
   mat4 matrices[];
};

void main()
{
   vec4 vert = vertices[indices[gl_VertexID] + gl_InstanceID*nFaceVert];
   vec4 pos = matrices[faceRank[gl_InstanceID]] * vec4(vert.xyz, 1);
   gl_Position = mvp * pos;
   gl_ClipDistance[0] = -dot(pos, clipPlane);

}

#elif _FRAGMENT_

out vec4 fragColor;

void main()
{
    fragColor = vec4(0.4, 0.4, 0.4, 1);
}

#endif
