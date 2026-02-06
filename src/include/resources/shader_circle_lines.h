namespace rocket_resource {
    const char *shader_circle_lines_rlsl = R"(
Version: 1.1
GL_MinimumVersion: 2.0
Name: Ring

VertexStart
    #version 330 core
    layout(location = 0) in vec2 aPos;
    uniform mat4 u_transform;
    out vec2 v_local;

    void main() {
        v_local = aPos; // normalized quad coordinates
        gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
    }
VertexEnd

FragmentStart
    #version 330 core
    in vec2 v_local;
    out vec4 FragColor;

    uniform vec4 u_color;
    uniform float u_radius;    // normalized [0..1] (relative to min size)
    uniform float u_thickness; // thickness in pixels
    uniform vec2 u_size;       // rect size in pixels

    void main() {
        // Convert quad coords [0..1] → pixels
        vec2 local_px = v_local * u_size;

        // Center of the rect
        vec2 center = u_size * 0.5;

        // Distance from center
        float dist = length(local_px - center);

        // Outer radius in pixels
        float outer_r = u_radius * 0.5 * min(u_size.x, u_size.y);
        float inner_r = outer_r - u_thickness;

        // Anti-alias width (1 pixel is usually fine)
        float aa = 1.0;

        // Alpha for outer edge
        float outer_alpha = smoothstep(outer_r + aa, outer_r - aa, dist);
        // Alpha for inner edge (invert)
        float inner_alpha = smoothstep(inner_r - aa, inner_r + aa, dist);

        float alpha = outer_alpha * inner_alpha;

        if (alpha <= 0.0)
            discard;

        FragColor = vec4(u_color.rgb, u_color.a * alpha);
    }
FragmentEnd)";
}
