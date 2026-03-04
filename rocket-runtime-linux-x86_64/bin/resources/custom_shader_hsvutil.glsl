vec3 hsv2rgb(float h, float s, float v) {
    vec3 k = vec3(5.0, 3.0, 1.0);
    vec3 p = abs(fract(h + k/6.0) * 6.0 - 3.0);
    return v * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), s);
}
