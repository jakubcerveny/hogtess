#line 2
#if _VERTEX_

out float solution;
out float gl_ClipDistance[1];

uniform mat4 mvp;
uniform int nFaceVert;
uniform vec4 clipPlane;

layout(std430, binding = 0) buffer bufVertices
{
   vec4 vertices[];
};

layout(std430, binding = 1) buffer bufIndices
{
   int indices[];
};

void main()
{
   vec4 vert = vertices[indices[gl_VertexID] + gl_InstanceID*nFaceVert];
   vec4 pos = vec4(vert.xyz, 1);
   gl_Position = mvp * pos;
   solution = vert.w;
   gl_ClipDistance[0] = -dot(pos, clipPlane);
}

#elif _FRAGMENT_

in float solution;
out vec4 fragColor;

uniform vec3 palette[PALETTE_SIZE];

void main()
{
    int i = clamp(int(solution * PALETTE_SIZE), 0, PALETTE_SIZE-1);
    fragColor = vec4(palette[i], 1);
}

#endif
