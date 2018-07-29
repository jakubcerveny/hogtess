#line 2
#if _VERTEX_

uniform mat4 mvp;

in vec4 position;
out float solution;

void main()
{
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
