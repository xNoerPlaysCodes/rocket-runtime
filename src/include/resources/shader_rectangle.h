namespace rocket_resource {
    const char *shader_rectangle_rlsl = R"(
=LangProperty NoPropertyOverride true 
=Set Name "Rectangle"
=Set Version 1.4

=EnterNamespace API 
    =Add SupportedAPIs GL 
    =Add SupportedAPIs GLES
//  =Add SupportedAPIs VK
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

// =Begin VertexShader
//     layout(location = 0) in vec3 a_pos;
//     layout(location = 1) in vec2 a_uv;
//
//     uniform mat4 u_sgfx_model;
//
//     out vec2 v_uv;
//
//     void main() {
//         v_uv = a_uv;
//         gl_Position = u_sgfx_model * vec4(a_pos, 1.0);
//     }
// =End
//
// =Begin FragmentShader
//     in vec2 v_uv;
//
//     uniform vec4 u_color;
//
//     out vec4 FragColor;
//
//     void main() {
//         // FragColor = vec4(v_uv, 0.0, 1.0); // gradient
//         FragColor = u_color;
//     }
// =End

=Begin VertexShader
    layout(location = 0) in vec3 a_pos; // 0→1 quad coords
    layout(location = 1) in vec2 a_uv;

    uniform mat4 u_sgfx_model;
    out vec2 v_local;

    void main() {
        v_local = a_pos.xy; // normalized quad coordinates
        gl_Position = u_sgfx_model * vec4(a_pos, 1.0);
    }
=End

=Begin FragmentShader
    in vec2 v_local;
    out vec4 FragColor;

    uniform vec4 u_color;   // RGBA 0–1
    uniform float u_radius; // fraction 0..1 of min size
    uniform vec2 u_size;    // rect size in pixels

    void main() {
        vec2 local_px = v_local * u_size;

        // Convert fraction to pixel radius
        float radius_px = u_radius * 0.5 * min(u_size.x, u_size.y);

        // Distance from nearest edge
        vec2 cornerDist = min(local_px, u_size - local_px);

        // Distance from corner arc
        float dist = length(cornerDist - vec2(radius_px));

        // Anti-aliased alpha edge
        float edge_thickness = 1.0; // in pixels
        float alpha = 1.0;

        float corner_mask = step(cornerDist.x, radius_px) * step(cornerDist.y, radius_px);
        alpha = mix(alpha, 1.0 - smoothstep(radius_px - edge_thickness, radius_px, dist), corner_mask);

        FragColor = vec4(u_color.rgb, u_color.a * alpha);
    }
=End)";
}
