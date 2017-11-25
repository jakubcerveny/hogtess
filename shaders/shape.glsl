
uniform float lagrangeNodes[P+1];
uniform float lagrangeWeights[P+1];


void lagrangeShape(float y, out float result[P+1])
{
    if (P == 0)
    {
        result[0] = 1.0;
    }
    else if (P <= /*3*/ 10) // O(p^2) evaluation for small p
    {
        for (int i = 0; i <= P; i++)
        {
            float l = lagrangeWeights[i];
            for (int j = 0; j < i; j++)
            {
                l *= (y - lagrangeNodes[j]);
            }
            for (int j = i+1; j <= P; j++)
            {
                l *= (y - lagrangeNodes[j]);
            }
            result[i] = l;
        }
    }
    else // O(p) evaluation
    {
        float l = 1.0, lk = 1.0;
        for (int i = 0; i <= P; i++)
        {
            l *= (y - lagrangeNodes[i]);
            // TODO lk
        }
        for (int i = 0; i <= P; i++)
        {
            result[i] = l * lagrangeWeights[i] / (y - lagrangeNodes[i]);
            // TODO result[k] = lk * w[i];
        }
    }
}


uniform sampler2DRect coefSampler;

vec4 lagrangeQuadSolution(int elemID, float u, float v)
{
    float ushape[P+1], vshape[P+1];
    lagrangeShape(u, ushape);
    lagrangeShape(v, vshape);

    vec4 position = vec4(0.0);
    for (int i = 0; i <= P; i++)
    for (int j = 0; j <= P; j++)
    {
        vec4 coef = texture(coefSampler, ivec2(i*(P+1) + j, elemID));
        position += ushape[i]*vshape[j] * coef;
    }
    return position;
}
