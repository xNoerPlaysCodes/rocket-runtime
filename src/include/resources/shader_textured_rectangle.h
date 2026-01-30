namespace rocket_resource {
    const char *shader_textured_rectangle_rlsl = R"(
Version: 1.1
GL_MinimumVersion: 2.0
Name: Rectangle

VertexStart
    #version 330 core
    layout(location = 0) in vec2 aPos;
    out vec2 v_uv;
    uniform mat4 u_transform;

    void main() {
        v_uv = aPos; // 0→1 quad coords
        gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
    }
VertexEnd

FragmentStart
    #version 330 core
    in vec2 v_uv;
    out vec4 FragColor;

    uniform sampler2D u_texture;
    uniform float u_radius; // fraction 0..1 of min size
    uniform vec2 u_size;    // rect size in pixels

    void main() {
        // Convert UV to local pixel space
        vec2 local_px = v_uv * u_size;

        // Convert fraction to pixel radius
        float radius_px = u_radius * 0.5 * min(u_size.x, u_size.y);

        // Distance from nearest edge
        vec2 cornerDist = min(local_px, u_size - local_px);

        // Distance from corner arc
        float dist = length(cornerDist - vec2(radius_px));

        // Anti-alias edge
        float edge_thickness = 1.0;
        float alpha = 1.0;

        float corner_mask = step(cornerDist.x, radius_px) * step(cornerDist.y, radius_px);
        alpha = mix(alpha, 1.0 - smoothstep(radius_px - edge_thickness, radius_px, dist), corner_mask);

        vec4 texColor = texture(u_texture, v_uv);
        FragColor = vec4(texColor.rgb, texColor.a * alpha);
    }
FragmentEnd
)";
}
