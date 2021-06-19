
out vec4 sk_FragColor;
uniform vec4 a;
uniform vec4 b;
uniform uvec2 c;
uniform uvec2 d;
uniform ivec3 e;
uniform ivec3 f;
void main() {
    bvec4 expectTTFF = bvec4(true, true, false, false);
    bvec4 expectFFTT = bvec4(false, false, true, true);
    sk_FragColor.x = float(lessThanEqual(a, b).x ? 1 : 0);
    sk_FragColor.y = float(lessThanEqual(c, d).y ? 1 : 0);
    sk_FragColor.z = float(lessThanEqual(e, f).z ? 1 : 0);
    sk_FragColor.w = float(any(expectTTFF) || any(expectFFTT) ? 1 : 0);
}
