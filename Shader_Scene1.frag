#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    //Need to work here after demo with Sachin and Sohel
    // overlayParams:
    // x = fade (0..1)
    // y = screen width  (px)
    // z = screen height (px)
    // w = +size fraction of min(screenW,screenH)  (Overlay on which Rashi texture shown will be rectangle shape)
	
    vec4 overlayParams;
} ubo;

layout(set = 0, binding = 1) uniform samplerCube texCube;   // background
layout(set = 0, binding = 2) uniform sampler2D   texOverlay; // overlay image

layout(location = 0) in  vec3 vDir;    // from vertex shader (skybox dir)
layout(location = 0) out vec4 outColor;

const float kEdgeFeatherFrac = 0.08;

void main()
{
    // Background sample from cubemap
    vec3 dir = normalize(vDir);
    vec3 bg  = texture(texCube, dir).rgb;

    float fade = clamp(ubo.overlayParams.x, 0.0, 1.0);
    if (fade <= 0.0) {
        outColor = vec4(bg, 1.0);
        return;
    }

    // Screen in pixels (Vulkan: origin top-left aahe)
    vec2 screen = vec2(max(1.0, ubo.overlayParams.y), max(1.0, ubo.overlayParams.z));
    vec2 fragPx = gl_FragCoord.xy;

    // Desired overlay size  and overlay is centered on screen
    float frac    = max(0.001, abs(ubo.overlayParams.w));
    float targetM = frac * min(screen.x, screen.y);

    ivec2 texSizeI = textureSize(texOverlay, 0);
    vec2  texSize  = vec2(max(1, texSizeI.x), max(1, texSizeI.y));

    // Scale kela aahe image la
    float sTarget = targetM / min(texSize.x, texSize.y);
    float sMax    = min(screen.x / texSize.x, screen.y / texSize.y);
    float s       = min(sTarget, sMax);

    vec2 sizePx   = texSize * s;
    vec2 center   = 0.5 * screen;
    vec2 rectMin  = center - 0.5 * sizePx; // top-left corner of overlay rect
    vec2 uv       = (fragPx - rectMin) / sizePx;

    // Overlay rectangle ulta asel tar
    // uv.y = 1.0 - uv.y;

    //Texture fakt overlay cha rectangle madhe yenar
    if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0))))
	{
        vec4 overlay = texture(texOverlay, uv);

        // Distance in pixels to the nearest rectangle edge
        float dxPx = min(uv.x, 1.0 - uv.x) * sizePx.x;
        float dyPx = min(uv.y, 1.0 - uv.y) * sizePx.y;
        float dPx  = min(dxPx, dyPx);

        float featherPx = max(1.0, kEdgeFeatherFrac * min(sizePx.x, sizePx.y));

        float edgeMask = smoothstep(0.0, featherPx, dPx);

        float a = overlay.a * fade * edgeMask;

        outColor = vec4(mix(bg, overlay.rgb, a), 1.0);
    } else {
        outColor = vec4(bg, 1.0);
    }
}
