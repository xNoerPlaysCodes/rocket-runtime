namespace rocket_resource {
    const char *shader_polygon_rlsl = R"(
Version: 1.3
GL_MinimumVersion: 3.3
GLES_MinimumVersion: 3.0
Name: Polygon

VertexStart
    layout (location = 0) in vec2 aPos;

    uniform vec4 uColor;
    out vec4 vColor;

    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        vColor = uColor;
    }
VertexEnd

FragmentStart
    in vec4 vColor;
    out vec4 FragColor;
    void main() {
        FragColor = vColor;
    }
FragmentEnd)";
}
