// Debug shader to visualize iMouse
// Expects uniforms: iResolution, iTime, iMouse

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec3 col = vec3(0.0);

    // show normalized mouse position as background tint
    vec2 mousePos = iMouse.xy / iResolution.xy;
    col += vec3(mousePos.x, 1.0 - mousePos.y, 0.2) * 0.2;

    // if click stored in zw, draw a small red circle at click
    vec2 click = iMouse.zw; // pixel coords
    if (click.x > 0.0 || click.y > 0.0) {
        float d = length(fragCoord - click);
        float r = 12.0; // pixel radius
        float m = smoothstep(r, r - 2.0, d);
        col = mix(col, vec3(1.0,0.0,0.0), 1.0 - m);
    }

    fragColor = vec4(col, 1.0);
}
