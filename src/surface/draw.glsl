#line 2
#if _VERTEX_

uniform mat4 mvp;
uniform int nFaceVert;

layout(std430, binding = 0) buffer vertexBuffer
{
   vec4 vertices[];
};

out float solution;

void main()
{
   vec4 position = vertices[gl_VertexID + gl_InstanceID*nFaceVert];
   gl_Position = mvp * vec4(position.xyz, 1);
   solution = position.w;
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
