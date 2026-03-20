// Shadertoy-compatible fragment shader
// Sphere <-> Cube morph loop
uniform vec2 iOrbit; // host-provided yaw/pitch, persistent across drags
uniform float iLightOn;
uniform float iDistance; // host-provided camera distance, controlled by scroll wheel

float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float mapScene(vec3 p, float morphT) {
    float dSphere = sdSphere(p, 0.65);
    float dBox = sdBox(p, vec3(0.55));
    return mix(dSphere, dBox, morphT);
}

vec3 calcNormal(vec3 p, float morphT) {
    const float e = 0.002;
    vec2 h = vec2(e, 0.0);
    return normalize(vec3(
        mapScene(p + vec3(h.x, h.y, h.y), morphT) - mapScene(p - vec3(h.x, h.y, h.y), morphT),
        mapScene(p + vec3(h.y, h.x, h.y), morphT) - mapScene(p - vec3(h.y, h.x, h.y), morphT),
        mapScene(p + vec3(h.y, h.y, h.x), morphT) - mapScene(p - vec3(h.y, h.y, h.x), morphT)
    ));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    // Persistent camera angles from host (no jump when drag restarts).
    float yaw = iOrbit.x;
    float pitch = iOrbit.y;

    // Camera position on a sphere of radius r around origin, controlled by yaw/pitch and scroll wheel
    float r = iDistance;
    vec3 ro = vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch)) * r;

    // Build camera basis and ray direction from uv
    vec3 forward = normalize(-ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    vec3 rd = normalize(forward + uv.x * right + uv.y * up);

    // Light controlled by host-side toggle switch.
    vec3 lightDir = normalize(vec3(0.6, 0.7, 0.5));
    bool lightOn = iLightOn > 0.5;

    // Precompute morphT once
    float morphT = 0.5 + 0.5 * sin(iTime * 1.2);

    // Ray march
    float tRay = 0.0;
    float d;
    bool hit = false;

    for (int i = 0; i < 80; i++) {
        vec3 p = ro + rd * tRay;
        d = mapScene(p, morphT);
        if (d < 0.001) {
            hit = true;
            break;
        }
        tRay += d;
        if (tRay > 8.0) break;
    }

    vec3 col = vec3(0.02, 0.03, 0.05) + 0.2 * vec3(uv.y + 0.4);

    if (hit) {
        vec3 p = ro + rd * tRay;
        vec3 n = calcNormal(p, morphT);
        vec3 base = mix(vec3(0.25, 0.7, 1.0), vec3(1.0, 0.45, 0.25), morphT);
        if (lightOn) {
            float diff = max(dot(n, lightDir), 0.0);
            float fres = pow(1.0 - max(dot(n, -rd), 0.0), 4.0);
            col = base * (0.2 + 0.8 * diff) + 0.35 * fres;
        } else {
            // No direct light when not clicking.
            col = base * 0.06;
        }
    }

    // Draw light toggle UI at right side of screen.
    vec2 swSize = vec2(110.0, 44.0);
    vec2 swMin = vec2(iResolution.x - swSize.x - 24.0, 0.5 * iResolution.y - 0.5 * swSize.y);
    vec2 swMax = swMin + swSize;
    float inSw = step(swMin.x, fragCoord.x) * step(swMin.y, fragCoord.y) * step(fragCoord.x, swMax.x) * step(fragCoord.y, swMax.y);
    if (inSw > 0.5) {
        vec3 bgOff = vec3(0.18, 0.18, 0.2);
        vec3 bgOn = vec3(0.15, 0.45, 0.18);
        col = mix(bgOff, bgOn, lightOn ? 1.0 : 0.0);

        vec2 knobCenter = vec2(lightOn ? (swMax.x - 22.0) : (swMin.x + 22.0), 0.5 * (swMin.y + swMax.y));
        float knob = 1.0 - smoothstep(14.5, 16.5, length(fragCoord - knobCenter));
        col = mix(col, vec3(0.95, 0.95, 0.95), knob);
    }

    fragColor = vec4(col, 1.0);
}
