namespace rocket_resource {
    const char *shader_rectangle_rlsl = R"(
VertexBegin
    #version 330 core
    layout(location = 0) in vec2 aPos; // 0→1 quad coords
    uniform mat4 u_transform;
    out vec2 v_local;

    void main() {
        v_local = aPos; // normalized quad coordinates
        gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
    }
VertexEnd
    )";
}
