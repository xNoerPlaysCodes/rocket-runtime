namespace rocket_resource {
    const char *shader_polygon_rlsl = R"(
=LangProperty NoPropertyOverride true
=Set Name "Polygon"
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
    layout (location = 0) in vec2 aPos;
    uniform vec4 uColor;
    out vec4 vColor;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        vColor = uColor;
    }
=End
=Begin FragmentShader
    in vec4 vColor;
    out vec4 FragColor;
    void main() {
        FragColor = vColor;
    }
=End)";
}

