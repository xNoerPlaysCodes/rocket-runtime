namespace rocket_resource {
    const char *shader_text_rlsl = R"(
Version: 1.1
GL_MinimumVersion: 2.0
Name: Text

VertexStart
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTex;
    out vec2 TexCoord;
    void main() {
        gl_Position = vec4(aPos.xy, 0.0, 1.0);
        TexCoord = aTex;
    }
VertexEnd

FragmentStart
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform vec3 u_color;
    uniform sampler2D u_texture;
    void main() {
        float alpha = texture(u_texture, TexCoord).r;
        // gamma correct to linear
        alpha = pow(alpha, 2.2);
        // sharpen edges
        alpha = pow(alpha, 0.5);
        // back to sRGB
        alpha = pow(alpha, 1.0 / 2.2);
        vec3 rgb = u_color * alpha;
        FragColor = vec4(rgb, alpha);
    }
FragmentEnd)";
}
