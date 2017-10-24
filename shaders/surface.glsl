#if _VERTEX_

attribute vec3 aPosition;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(aPosition, 1);

    //gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
    //normal = gl_NormalMatrix * gl_Normal;
}

#elif _FRAGMENT_

void main()
{
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

#endif
