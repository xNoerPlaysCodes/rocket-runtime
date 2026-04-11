namespace rocket_resource {
    const char *shader_atlas_textured_rectangle_rlsl = R"(
=LangProperty NoPropertyOverride true
=Set Name "Texture Atlas"
=Set Version 1.4
=EnterNamespace API
    =Add SupportedAPIs GL
    =Add SupportedAPIs GLES
=ExitNamespace
=EnterNamespace API
    =EnterNamespace VK
        =Set MinimumVersion 1.1
    =ExitNamespace
    =EnterNamespace GLES
        =Set MinimumVersion 3.0
    =ExitNamespace
    =EnterNamespace GL
        =Set MinimumVersion 3.3
    =ExitNamespace
=ExitNamespace
=Begin VertexShader
    layout(location = 0) in vec2 aPos;
    out vec2 v_uv;
    uniform mat4 u_transform;
    void main() {
        v_uv = aPos; // 0→1 quad coords
        gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
    }
=End
=Begin FragmentShader
    in vec2 v_uv;
    out vec4 FragColor;
    uniform sampler2D u_texture;
    uniform float u_radius; 
    uniform vec2 u_size;    
    uniform vec2 u_texPos;   // atlas top-left UV
    uniform vec2 u_texSize;  // atlas size in UV
    void main() {
        vec2 local_px = v_uv * u_size;
        float radius_px = u_radius * 0.5 * min(u_size.x, u_size.y);
        vec2 cornerDist = min(local_px, u_size - local_px);
        float dist = length(cornerDist - vec2(radius_px));
        float edge_thickness = 1.0;
        float alpha = 1.0;
        float corner_mask = step(cornerDist.x, radius_px) * step(cornerDist.y, radius_px);
        alpha = mix(alpha, 1.0 - smoothstep(radius_px - edge_thickness, radius_px, dist), corner_mask);
        // Apply atlas offset/scale
        vec2 uv = u_texPos + vec2(v_uv.x, v_uv.y) * u_texSize;
        vec4 texColor = texture(u_texture, uv);
        FragColor = vec4(texColor.rgb, texColor.a * alpha);
    }
=End)";
}
