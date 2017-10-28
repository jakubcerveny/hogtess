const float PI = 3.141592654;

uniform mat4 MVP;

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

    vec4 diagonal = MVP*gl_in[2].gl_Position - MVP*gl_in[0].gl_Position;
    float d = sqrt(dot(diagonal.xy, diagonal.xy)) * 1000 /*FIXME*/;

    float subdiv = max(d / 70, 1);

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

    position.x += 0.07 * sin(2*PI*v);
    position.y += 0.07 * sin(2*PI*u);


    gl_Position = MVP * position;
}


#elif _FRAGMENT_

out vec4 fragColor;

void main()
{
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}

#endif
