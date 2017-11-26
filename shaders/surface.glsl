#line 2

uniform mat4 MVP;

#if _VERTEX_

out int vElementID;

void main()
{
    vElementID = gl_VertexID >> 2;
}


#elif _TESS_CONTROL_

layout(vertices = 4) out;

in int vElementID[];
out int tcElementID[];

uniform vec2 screenSize;

float subdiv(float pixels)
{
    return max(pixels / 5, P);
}

void main()
{
    if (gl_InvocationID == 0)
    {
        int elemID = vElementID[gl_InvocationID];

        vec4 vert[4];
        vert[0] = nodalValue(elemID, 0, 0);
        vert[1] = nodalValue(elemID, P, 0);
        vert[2] = nodalValue(elemID, 0, P);
        vert[3] = nodalValue(elemID, P, P);

        vec2 screen[4];
        for (int i = 0; i < 4; i++) {
            vec4 v = vec4(vert[i].xyz, 1);
            vec4 t = MVP * v;
            vec2 ndc = t.xy / t.w;
            screen[i] = ndc * screenSize * 0.5;
        }

        // TODO: check if outside the view frustum

        // see www.khronos.org/opengl/wiki/Tessellation for the numbering
        gl_TessLevelOuter[0] = subdiv(distance(screen[2], screen[0]));
        gl_TessLevelOuter[1] = subdiv(distance(screen[0], screen[1]));
        gl_TessLevelOuter[2] = subdiv(distance(screen[1], screen[3]));
        gl_TessLevelOuter[3] = subdiv(distance(screen[3], screen[2]));

        gl_TessLevelInner[0] = (gl_TessLevelOuter[1] + gl_TessLevelOuter[3])*0.5;
        gl_TessLevelInner[1] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[2])*0.5;

        tcElementID[gl_InvocationID] = elemID;
    }
}


#elif _TESS_EVAL_

layout(quads) in;

in int tcElementID[];
out float solution;

void main()
{
    int elemID = tcElementID[0];

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 sln = lagrangeQuadSolution(elemID, u, v);
    vec4 position = vec4(sln.xyz, 1);

    gl_Position = MVP * position;
    solution = sln.w;
}


#elif _FRAGMENT_

in float solution;
out vec4 fragColor;

uniform vec3 palette[PALETTE_SIZE];

void main()
{
    int i = clamp(int(solution * PALETTE_SIZE), 0, PALETTE_SIZE-1);
    fragColor = vec4(palette[i], 1.0);
}

#endif
