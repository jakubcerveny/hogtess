#line 2
#if _VERTEX_

out float solution;

uniform mat4 mvp;

layout(std430, binding = 0) buffer bufTriangles
{
   vec4 vertices[];
};

void main()
{
   vec4 vert = vertices[gl_VertexID];
   vec4 pos = vec4(vert.xyz, 1);
   gl_Position = mvp * pos;
   solution = vert.w;
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
