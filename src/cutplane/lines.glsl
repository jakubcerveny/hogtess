#line 2
#if _VERTEX_

uniform mat4 mvp;

layout(std430, binding = 0) buffer bufLines
{
   vec4 vertices[];
};

void main()
{
   vec4 vert = vertices[gl_VertexID];
   vec4 pos = vec4(vert.xyz, 1);
   gl_Position = mvp * pos;
}

#elif _FRAGMENT_

out vec4 fragColor;

void main()
{
    fragColor = vec4(0.4, 0.4, 0.4, 1);
}

#endif
