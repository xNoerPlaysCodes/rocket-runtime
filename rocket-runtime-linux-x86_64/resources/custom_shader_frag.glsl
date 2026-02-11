#version 330 core
out vec4 FragColor;
in vec2 fragPos;
vec3 hsv2rgb(float h, float s, float v);

void main() {
    float r = length(fragPos);
    float hue = atan(fragPos.y, fragPos.x) / (2.0 * 3.14159) + 0.5;

    FragColor = vec4(hsv2rgb(hue, 1., 1.), 1.);
}
