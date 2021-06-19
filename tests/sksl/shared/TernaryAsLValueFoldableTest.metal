#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
struct Uniforms {
    float unknownInput;
};
struct Inputs {
};
struct Outputs {
    float4 sk_FragColor [[color(0)]];
};

fragment Outputs fragmentMain(Inputs _in [[stage_in]], constant Uniforms& _uniforms [[buffer(0)]], bool _frontFacing [[front_facing]], float4 _fragCoord [[position]]) {
    Outputs _out;
    (void)_out;
    float r;
    float g;
    r = 1.0 - _uniforms.unknownInput;
    g = _uniforms.unknownInput;
    _out.sk_FragColor = float4(r, g, 0.0, 1.0);
    return _out;
}
