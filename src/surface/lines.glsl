#line 2
#if _VERTEX_

uniform mat4 mvp;
uniform int nFaceVert;

layout(std430, binding = 0) buffer vertexBuffer
{
   vec4 vertices[];
};

layout(std430, binding = 1) buffer indexBuffer
{
   int indices[];
};

void main()
{
   vec4 position = vertices[indices[gl_VertexID] + gl_InstanceID*nFaceVert];
   gl_Position = mvp * vec4(position.xyz, 1);
}

#elif _FRAGMENT_

out vec4 fragColor;

void main()
{
    fragColor = vec4(0.4, 0.4, 0.4, 1);
}

#endif
