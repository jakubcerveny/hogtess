#line 2
const float PI = 3.141592654;

uniform mat4 MVP;
uniform vec2 screenSize;

#if _VERTEX_

in vec3 inPosition;

void main()
{
    gl_Position = vec4(inPosition, 1);
}


#elif _TESS_CONTROL_

layout(vertices = 4) out;

void main()
{
    #define ID gl_InvocationID
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;

    vec4 diag = MVP*gl_in[2].gl_Position - MVP*gl_in[0].gl_Position;
    diag *= vec4(screenSize * 0.5, 1, 1);
    float d = sqrt(dot(diag.xy, diag.xy));

    float subdiv = max(d / 50, 1);

    if (ID == 0) {
        gl_TessLevelInner[0] = subdiv;
        gl_TessLevelInner[1] = subdiv;
        gl_TessLevelOuter[0] = subdiv;
        gl_TessLevelOuter[1] = subdiv;
        gl_TessLevelOuter[2] = subdiv;
        gl_TessLevelOuter[3] = subdiv;
    }
}


#elif _TESS_EVAL_

layout(quads) in;

void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 a = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, u);
    vec4 b = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u);
    vec4 position = mix(a, b, v);

    float ushape[P+1], vshape[P+1];
    lagrange(u, ushape);
    lagrange(v, vshape);

    position.x += 0.1 * vshape[1];
    position.y += -0.1 * ushape[3];

    gl_Position = MVP * position;
}


#elif _FRAGMENT_

out vec4 fragColor;

uniform sampler2DRect sampler;

void main()
{
    //fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    fragColor = texture(sampler, ivec2(1, 0));
}

#endif
