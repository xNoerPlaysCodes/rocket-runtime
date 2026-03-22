namespace rocket_resource {
    const char *shader_textured_rectangle_rlsl = R"(
Version: 1.3
GL_MinimumVersion: 3.3
GLES_MinimumVersion: 3.0
Name: Textured Rectangle

VertexStart
    layout(location = 0) in vec2 aPos;
    out vec2 v_uv;
    uniform mat4 u_transform;
    uniform float u_flip_y;
    void main() {
        float uv_y = mix(aPos.y, 1.0 - aPos.y, u_flip_y);
        v_uv = vec2(aPos.x, uv_y);
        gl_Position = u_transform * vec4(aPos, 0.0, 1.0);
    }
VertexEnd
FragmentStart
    in vec2 v_uv;
    out vec4 FragColor;
    uniform sampler2D u_texture;
    uniform float u_radius;
    uniform vec2 u_size;
    void main() {
        vec2 local_px = v_uv * u_size;
        float radius_px = u_radius * 0.5 * min(u_size.x, u_size.y);
        vec2 cornerDist = min(local_px, u_size - local_px);
        float dist = length(cornerDist - vec2(radius_px));
        float edge_thickness = 1.0;
        float alpha = 1.0;
        float corner_mask = step(cornerDist.x, radius_px) * step(cornerDist.y, radius_px);
        alpha = mix(alpha, 1.0 - smoothstep(radius_px - edge_thickness, radius_px, dist), corner_mask);
        vec4 texColor = texture(u_texture, v_uv);
        FragColor = vec4(texColor.rgb, texColor.a * alpha);
    }
FragmentEnd)";
}
