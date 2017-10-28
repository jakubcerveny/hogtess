
#if _VERTEX_

in vec3 inPosition;

//uniform mat4 MVP;

void main()
{
    gl_Position = vec4(inPosition, 1);
}


#elif _TESS_CONTROL_

layout(vertices = 4) out;

const float inner = 16.0;
const float outer = 16.0;

void main()
{
    #define ID gl_InvocationID
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    if (ID == 0) {
        gl_TessLevelInner[0] = inner;
        gl_TessLevelInner[1] = inner;
        gl_TessLevelOuter[0] = outer;
        gl_TessLevelOuter[1] = outer;
        gl_TessLevelOuter[2] = outer;
        gl_TessLevelOuter[3] = outer;
    }
}


#elif _TESS_EVAL_

layout(quads) in;

uniform mat4 MVP;

void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 a = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, u);
    vec4 b = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u);
    vec4 position = mix(a, b, v);

    gl_Position = MVP * position;
}


#elif _FRAGMENT_

out vec4 fragColor;

void main()
{
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

#endif
