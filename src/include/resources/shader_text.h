namespace rocket_resource {
    const char *shader_text_rlsl = R"(
=LangProperty NoPropertyOverride true
=Set Name "Text"
=Set Version 1.4
=EnterNamespace API
    =Add SupportedAPIs GL
    =Add SupportedAPIs GLES
=ExitNamespace
=EnterNamespace API
    =EnterNamespace GLES
        =Set MinimumVersion 3.0
    =ExitNamespace
    =EnterNamespace GL
        =Set MinimumVersion 3.3
    =ExitNamespace
=ExitNamespace
=Begin VertexShader
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTex;
    out vec2 TexCoord;
    uniform mat4 u_sgfx_model;
    void main() {
        vec2 ndc = aPos.xy * 2.0 - 1.0;
        gl_Position = vec4(ndc, 0.0, 1.0);
        // gl_Position = u_sgfx_model * vec4(aPos.xy, 0.0, 1.0);
        TexCoord = aTex;
    }
=End
=Begin FragmentShader
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform vec3 u_color;
    uniform sampler2D u_texture;
    void main() {
        float alpha = texture(u_texture, TexCoord).r;
        // FragColor = vec4(u_color, alpha);
        // gamma correct to linear
        alpha = pow(alpha, 2.2);
        // sharpen edges
        alpha = pow(alpha, 0.5);
        // back to sRGB
        alpha = pow(alpha, 1.0 / 2.2);
        vec3 rgb = u_color * alpha;
        FragColor = vec4(rgb, alpha);
    }
=End)";
}

