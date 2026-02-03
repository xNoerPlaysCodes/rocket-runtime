namespace rocket_resource {
    const char *shader_fxaa_rlsl = R"(
Version: 1.1
GL_MinimumVersion: 2.0
Name: FXAA

VertexStart
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 vTexCoord;

    void main() {
        vTexCoord = aTexCoord;
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
VertexEnd

FragmentStart
    #version 330 core
    in vec2 vTexCoord;
    out vec4 FragColor;

    uniform sampler2D uScene;
    uniform vec2 uResolution; // screen width/height

    // FXAA settings
    #define FXAA_REDUCE_MIN   (1.0/128.0)
    #define FXAA_REDUCE_MUL   (1.0/8.0)  // was 1/8
    #define FXAA_SPAN_MAX     8.0       // was 8.0

    void main() {
        vec3 rgbNW = texture(uScene, vTexCoord + vec2(-1.0, -1.0) / uResolution).rgb;
        vec3 rgbNE = texture(uScene, vTexCoord + vec2( 1.0, -1.0) / uResolution).rgb;
        vec3 rgbSW = texture(uScene, vTexCoord + vec2(-1.0,  1.0) / uResolution).rgb;
        vec3 rgbSE = texture(uScene, vTexCoord + vec2( 1.0,  1.0) / uResolution).rgb;
        vec3 rgbM  = texture(uScene, vTexCoord).rgb;

        vec3 luma = vec3(0.299, 0.587, 0.114);

        float lumaNW = dot(rgbNW, luma);
        float lumaNE = dot(rgbNE, luma);
        float lumaSW = dot(rgbSW, luma);
        float lumaSE = dot(rgbSE, luma);
        float lumaM  = dot(rgbM,  luma);

        float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
        float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

        vec2 dir;
        dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
        dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

        float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
        float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
        dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) / uResolution;

        vec3 rgbA = 0.5 * (
            texture(uScene, vTexCoord + dir * (1.0/3.0 - 0.5)).rgb +
            texture(uScene, vTexCoord + dir * (2.0/3.0 - 0.5)).rgb
        );
        vec3 rgbB = rgbA * 0.5 + 0.25 * (
            texture(uScene, vTexCoord + dir * -0.5).rgb +
            texture(uScene, vTexCoord + dir * 0.5).rgb
        );

        float lumaB = dot(rgbB, luma);
        FragColor = vec4((lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB, 1.0);
    }
FragmentEnd)";
}
