// ================================================================
// 11_shadow_map.cpp  -- Shadow Map Principle & Artifacts
// ================================================================
// Shadow mapping is a 2-pass technique:
//
// PASS 1 (light pass):
//   Render scene from the light's point of view.
//   For each fragment, record depth (distance from light) into a texture.
//   -> This is the "shadow map" (depth map).
//
// PASS 2 (camera pass):
//   For each fragment in camera view:
//   1. Transform the fragment's world position to light clip space:
//      pos_light = LightViewProj * world_pos
//   2. Perspective divide + remap to [0,1]:
//      uv_light  = pos_light.xy / pos_light.w * 0.5 + 0.5
//      z_light   = pos_light.z  / pos_light.w * 0.5 + 0.5
//   3. Sample shadow map at uv_light: shadow_depth = texture(shadowMap, uv_light)
//   4. If z_light > shadow_depth + bias -> IN SHADOW (blocked by something closer)
//
// ARTIFACTS:
//   Shadow Acne: self-shadowing due to depth precision.
//     Fix: add a bias to z_light before comparison.
//   Peter Panning: bias too large -> shadow detaches from object.
//     Fix: tune bias, or use geometry offset (push back-faces).
//   Aliasing: shadow edge is blocky due to shadow map resolution.
//     Fix: PCF (Percentage Closer Filtering) - average several samples.
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
    Vec3 operator-(const Vec3& r) const { return {x-r.x, y-r.y, z-r.z}; }
    Vec3 operator*(double s)      const { return {x*s, y*s, z*s}; }
};
static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static double len(const Vec3& a) { return std::sqrt(dot(a,a)); }
static Vec3 normalize(const Vec3& a) { double l=len(a); return {a.x/l,a.y/l,a.z/l}; }

// ---- Shadow test (CPU simulation, no GPU texture) ----
//
// We simulate a 1D scene:
//   Occluder is a block at worldZ in [2, 3].
//   Receiver surface is a plane at worldZ = 5.
//   Light is at worldZ = 0, directional.
//
// "Shadow map value" is the minimum Z where something exists from light's view.
// Here we approximate: occluder blocks depth [2, 3], receiver is at depth 5.

static const double OCCLUDER_NEAR = 2.0;
static const double OCCLUDER_FAR  = 3.0;
static const double RECEIVER_Z    = 5.0;

// Simulate: is a point at world_z in shadow?
// shadow_map_depth = depth of the closest thing between light and receiver
bool inShadow(double world_z, double shadow_map_depth, double bias) {
    // world_z is the depth of the fragment from the light
    return (world_z - bias) > shadow_map_depth;
}

static void printTheory() {
    std::cout << "================ Shadow Map Technique ================\n";
    std::cout << "PASS 1 - Light Pass:\n";
    std::cout << "  Render scene from light viewpoint.\n";
    std::cout << "  Store depth of closest surface in shadow map texture.\n\n";
    std::cout << "PASS 2 - Camera Pass (in fragment shader):\n";
    std::cout << "  1. Project fragment to light space: pos_L = LightVP * worldPos\n";
    std::cout << "  2. Normalize:  uv = pos_L.xy/pos_L.w * 0.5 + 0.5\n";
    std::cout << "                  z  = pos_L.z/pos_L.w  * 0.5 + 0.5\n";
    std::cout << "  3. Sample shadow map: shadow_z = texture(shadowMap, uv).r\n";
    std::cout << "  4. In shadow if:  z > shadow_z + bias\n\n";
    std::cout << "ARTIFACTS:\n";
    std::cout << "  Shadow Acne  : self-shadow due to precision -> add bias\n";
    std::cout << "  Peter Panning: bias too large -> shadow floats -> tune bias\n";
    std::cout << "  Aliasing     : blocky edges -> use PCF (average N nearby samples)\n";
    std::cout << "  Hard edge     : use PCSS (Percentage Closer Soft Shadows)\n";
    std::cout << "======================================================\n\n";
}

static void runTests() {
    // Scene: occluder at depth [2,3], receiver at depth 5.
    // Shadow map records depth 2.0 (front face of occluder).
    double shadow_map_depth = OCCLUDER_NEAR;   // 2.0

    std::cout << "Scene:\n";
    std::cout << "  Light at z=0 (directional, shooting +Z)\n";
    std::cout << "  Occluder at z=2 to z=3\n";
    std::cout << "  Receiver plane at z=5\n";
    std::cout << "  Shadow map depth = " << shadow_map_depth << "\n\n";

    // Test points on the receiver surface at z=5
    std::cout << std::fixed << std::setprecision(4);
    struct TC { const char* label; double world_z; bool expectedShadow; };
    TC tests[] = {
        {"Receiver below occluder  ", RECEIVER_Z,  true},   // blocked by occluder
        {"Surface of occluder      ", OCCLUDER_NEAR, false}, // AT the shadow map depth
        {"Just behind shadow map   ", OCCLUDER_NEAR+0.001, true},  // epsilon past
        {"Light-side of occluder   ", OCCLUDER_NEAR-0.1, false},   // in front of occluder
    };

    double bias = 0.005;
    std::cout << "Shadow test with bias=" << bias << ":\n";
    std::cout << "  " << std::setw(30) << std::left << "Case"
              << " world_z   shadow_z  diff     result\n";
    std::cout << "  " << std::string(65, '-') << "\n";

    for (auto& t : tests) {
        bool shadow = inShadow(t.world_z, shadow_map_depth, bias);
        std::cout << "  " << std::setw(30) << std::left << t.label
                  << " " << std::setw(8) << t.world_z
                  << " " << std::setw(8) << shadow_map_depth
                  << " " << std::setw(8) << (t.world_z - shadow_map_depth)
                  << " -> " << (shadow ? "SHADOW" : "LIT   ")
                  << (shadow==t.expectedShadow ? "  EXPECTED" : "  UNEXPECTED")
                  << "\n";
    }
    std::cout << "\n";

    // Show shadow acne phenomenon (without bias)
    std::cout << "Shadow Acne demo (bias=0, surface point at SAME depth as shadow map):\n";
    double surface_z = shadow_map_depth + 0.003;  // tiny floating point error
    bool acne = inShadow(surface_z, shadow_map_depth, 0.0);  // no bias
    bool fixed = inShadow(surface_z, shadow_map_depth, 0.005);  // with bias
    std::cout << "  surface_z=" << surface_z << "  shadow_map=" << shadow_map_depth << "\n";
    std::cout << "  Without bias: " << (acne ? "INCORRECTLY IN SHADOW (acne!)" : "OK") << "\n";
    std::cout << "  With bias=0.005: " << (fixed ? "INCORRECTLY IN SHADOW" : "OK") << "\n\n";
}

static void printPCF() {
    std::cout << "PCF (Percentage Closer Filtering) concept:\n";
    std::cout << "  Instead of 1 sample, take NxN samples around uv:\n";
    std::cout << "  shadow = 0.0\n";
    std::cout << "  for each offset (dx, dy) in kernel:\n";
    std::cout << "      s = (z > texture(shadowMap, uv + offset).r + bias) ? 1.0 : 0.0\n";
    std::cout << "      shadow += s\n";
    std::cout << "  shadow /= (N*N)\n";
    std::cout << "  -> shadow in [0,1] = soft penumbra effect\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Bias formula: max(0.05*(1-dot(N,L)), 0.005) for slope-scale bias\n";
    std::cout << "  [ ] Point light shadows: 6 shadow maps (cube map) for all directions\n";
    std::cout << "  [ ] Cascaded Shadow Maps (CSM): separate shadow maps per distance range\n";
    std::cout << "  [ ] VSM (Variance Shadow Maps): store depth + depth^2 for soft shadows\n";
}

int main() {
    printTheory();
    runTests();
    printPCF();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why does shadow acne happen and why does constant bias sometimes fail?
A key points:
- Limited depth precision causes self-shadow comparisons to fail on the same surface.
- Constant bias may under-bias steep slopes and over-bias flat areas.
- Slope-scale bias and normal-offset bias reduce acne while limiting peter-panning.
*/
