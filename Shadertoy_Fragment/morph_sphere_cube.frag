// Shadertoy-compatible fragment shader
// Sphere <-> Cube morph loop

float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float mapScene(vec3 p, out float morphT) {
    // 0..1..0 loop over time
    morphT = 0.5 + 0.5 * sin(iTime * 1.2);
    float dSphere = sdSphere(p, 0.65);
    float dBox = sdBox(p, vec3(0.55));
    return mix(dSphere, dBox, morphT);
}

vec3 calcNormal(vec3 p) {
    const float e = 0.001;
    float t;
    vec2 h = vec2(e, 0.0);
    return normalize(vec3(
        mapScene(p + vec3(h.x, h.y, h.y), t) - mapScene(p - vec3(h.x, h.y, h.y), t),
        mapScene(p + vec3(h.y, h.x, h.y), t) - mapScene(p - vec3(h.y, h.x, h.y), t),
        mapScene(p + vec3(h.y, h.y, h.x), t) - mapScene(p - vec3(h.y, h.y, h.x), t)
    ));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    // Mouse-based camera control (iMouse: xy=current, zw=click)
    vec2 mouse = iMouse.xy;
    vec2 click = iMouse.zw;
    vec2 drag = vec2(0.0);
    if (click.x > 0.0 || click.y > 0.0) {
        drag = mouse - click; // pixel delta while dragging
    }
    // map pixel drag -> angles
    float yaw = -drag.x * 0.01; // horizontal
    float pitch = clamp(drag.y * 0.01, -1.4, 1.4); // vertical

    // Camera position on a sphere of radius r around origin, controlled by yaw/pitch
    float r = 2.6;
    vec3 ro = vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch)) * r;

    // Build camera basis and ray direction from uv
    vec3 forward = normalize(-ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    vec3 rd = normalize(forward + uv.x * right + uv.y * up);

    // Light
    vec3 lightDir = normalize(vec3(0.6, 0.7, 0.5));

    // Ray march
    float tRay = 0.0;
    float d;
    float morphT = 0.0;
    bool hit = false;

    for (int i = 0; i < 120; i++) {
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
        vec3 n = calcNormal(p);
        float diff = max(dot(n, lightDir), 0.0);
        float fres = pow(1.0 - max(dot(n, -rd), 0.0), 4.0);
        vec3 base = mix(vec3(0.25, 0.7, 1.0), vec3(1.0, 0.45, 0.25), morphT);
        col = base * (0.2 + 0.8 * diff) + 0.35 * fres;
    }

    fragColor = vec4(col, 1.0);
}
