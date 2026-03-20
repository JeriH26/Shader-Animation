// Black background with a central sky-cloud blob, idly moving.
// Shadertoy style entry: mainImage(out vec4, in vec2).
uniform float iDistance; // host zoom distance, controlled by mouse wheel

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 m = mat2(1.7, 1.1, -1.1, 1.7);
    for (int i = 0; i < 5; ++i) {
        v += a * valueNoise(p);
        p = m * p;
        a *= 0.5;
    }
    return v;
}

float softParticles(vec2 p, float t, float breathe) {
    float acc = 0.0;
    for (int i = 0; i < 24; ++i) {
        float fi = float(i);
        float r1 = hash12(vec2(fi, 11.7));
        float r2 = hash12(vec2(fi, 41.3));
        float r3 = hash12(vec2(fi, 93.9));

        float angle = 6.2831853 * r1 + t * (0.12 + 0.06 * r2);
        vec2 dir = vec2(cos(angle), sin(angle));

        float radialBand = mix(0.08, 0.42, r2);
        vec2 pos = dir * radialBand * mix(0.9, 1.15, breathe);

        float size = mix(0.020, 0.008, r3);
        float d = length(p - pos);
        float core = exp(-d * d / (size * size));

        float twinkle = 0.5 + 0.5 * sin(t * (2.8 + 2.5 * r3) + fi * 1.7);
        twinkle = pow(twinkle, 4.5);

        acc += core * twinkle;
    }

    return acc * 0.030;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 p = (fragCoord.xy - 0.5 * iResolution.xy) / iResolution.y;

    float t = iTime;
    float r = length(p);

    // Use near-black background so edge alpha falloff does not look like a harsh black halo.
    vec3 col = vec3(0.014, 0.018, 0.028);

    // Sky palette used only inside cloud body (not full screen).
    vec3 skyTop = vec3(0.20, 0.42, 0.72);
    vec3 skyMid = vec3(0.42, 0.64, 0.86);
    vec3 skyBottom = vec3(0.68, 0.82, 0.94);
    float h = smoothstep(0.0, 1.0, uv.y);
    vec3 skyColor = mix(mix(skyBottom, skyMid, smoothstep(0.0, 0.60, h)), skyTop, smoothstep(0.45, 1.0, h));

    // Slow idle motion for the cloud mass.
    vec2 center = vec2(0.09 * sin(t * 0.22), 0.06 * sin(t * 0.17 + 1.4));
    vec2 q = p - center;
    float rq = length(q);
    float angQ = atan(q.y, q.x);
    vec2 dirQ = vec2(cos(angQ), sin(angQ));

    // Irregular spherical cloud boundary.
    float boundaryNoiseA = fbm(q * 3.4 + vec2(t * 0.04, -t * 0.03));
    float boundaryNoiseB = fbm(vec2(dirQ.x * 1.4 + dirQ.y * 0.9, rq * 4.2 - t * 0.02));
    float edgeWarp = (boundaryNoiseA - 0.5) * 0.13 + (boundaryNoiseB - 0.5) * 0.07;

    // Map camera distance [1.2, 8.0] -> cloud size scale [1.55, 0.55].
    float zoom01 = clamp((iDistance - 1.2) / (8.0 - 1.2), 0.0, 1.0);
    float sizeScale = mix(1.55, 0.55, zoom01);
    float blobRadius = 0.46 * sizeScale;
    float blobMask = smoothstep(blobRadius + 0.11, blobRadius - 0.11, rq + edgeWarp);

    // Cloud texture inside blob.
    vec2 cloudUv = q * 1.8 + vec2(t * 0.02, -t * 0.01);
    float c1 = fbm(cloudUv * 2.6);
    float c2 = fbm(cloudUv * 5.0 + 7.1);
    float cloudTex = mix(c1, c2, 0.45);
    float cloudMask = smoothstep(0.42, 0.78, cloudTex);

    vec3 cloudCol = vec3(0.82, 0.88, 0.94);
    vec3 blobCol = mix(skyColor, cloudCol, cloudMask * 0.72);

    // Gentle fog edge aligned with warped boundary to avoid seam/halo mismatch.
    float warpedR = rq + edgeWarp;
    float fogEdge = smoothstep(blobRadius + 0.28, blobRadius - 0.06, warpedR);
    blobCol = mix(blobCol, vec3(0.72, 0.79, 0.88), fogEdge * 0.10);

    float sparkle = softParticles(q, t, 0.5);
    blobCol += sparkle * vec3(0.75, 0.82, 0.90) * blobMask * 0.07;

    // Compose with softened edge alpha to avoid double-darkening near boundary.
    float edgeAlpha = smoothstep(0.06, 0.98, blobMask);
    col = mix(col, blobCol, edgeAlpha);

    float vignette = smoothstep(1.25, 0.20, r);
    col *= 0.90 + 0.10 * vignette;

    fragColor = vec4(col, 1.0);
}
