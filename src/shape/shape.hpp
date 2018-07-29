#ifndef hogtess_shape_hpp_included_
#define hogtess_shape_hpp_included_


class Program;

// set the uniforms required by shape.glsl
void lagrangeUniforms(const Program &prog, int p, const double *nodalPoints);


#endif // hogtess_shape_hpp_included_
