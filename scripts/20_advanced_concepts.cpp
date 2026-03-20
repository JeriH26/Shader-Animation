// ================================================================
// 20_advanced_concepts.cpp  -- Advanced Graphics Concepts
// ================================================================
// This file covers GPU-side concepts that can't be fully demonstrated
// without a rendering context, but explains the math and logic.
//
// Topics:
//   A. Forward vs Deferred Rendering
//   B. Temporal Anti-Aliasing (TAA)
//   C. Screen Space Reflections (SSR)
//   D. Ambient Occlusion (SSAO)
//   E. Tone Mapping & HDR
//   F. Render Graph / Frame Graph
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

static const double PI = 3.14159265358979;

// ================================================================
// A. FORWARD vs DEFERRED RENDERING
// ================================================================
// Forward Rendering:
//   For each object: for each light: compute lighting and write to framebuffer
//   Complexity: O(objects * lights) -- bad for many lights
//   Pro: simple, supports transparency, MSAA easy
//   Con: overdraw wastes lighting work on hidden pixels
//
// Deferred Rendering (G-Buffer pass + Lighting pass):
//   Pass 1 (Geometry): render all opaque geometry into G-Buffer
//     G-Buffer stores per-pixel: position, normal, albedo, roughness/metallic, etc.
//   Pass 2 (Lighting): for each light, only shade the visible pixels in screen space
//   Complexity: O(pixels * lights) -- much better for many lights
//   Pro: separates geometry and lighting, cheap point lights, easy to add lights
//   Con: no MSAA (depth is already resolved), transparency is hard, large memory bandwidth
//
// G-Buffer layout example (4 render targets):
//   RT0: albedo.rgb  | roughness   (RGBA8)
//   RT1: normal.xyz  | metallic    (RGBA16F)
//   RT2: emissive.rgb| AO          (RGBA8)
//   RT3: depth                     (D24S8)

static void forwardVsDeferred() {
    std::cout << "A. Forward vs Deferred Rendering\n";
    std::cout << "   Forward: shade every fragment that passes depth test\n";
    std::cout << "   Deferred: 2-pass — write G-buffer, then light screen-space data\n\n";

    int lights = 1000, triangles = 100000, pixels = 1920*1080;
    std::cout << "   Scene: " << lights << " lights, " << triangles << " triangles\n";
    std::cout << "   Forward (worst case):  " << (long long)triangles * lights
              << " lighting evaluations (many are discarded pixels!)\n";
    std::cout << "   Deferred (lighting pass): " << (long long)pixels * lights
              << " lighting evaluations\n";
    std::cout << "   BUT: deferred lights can be tiled (Tiled Deferred) -> much better:\n";
    std::cout << "   Each tile (~16x16 px) only evaluates lights that overlap it.\n\n";

    std::cout << "   G-Buffer contents (typical 4 render targets):\n";
    std::cout << "     RT0 RGBA8:  albedo.rgb + roughness\n";
    std::cout << "     RT1 RGBA16F: world normal.xyz + metallic\n";
    std::cout << "     RT2 RGBA8:  emissive + AO\n";
    std::cout << "     D32:        depth (reconstruct world pos via inverse MVP)\n\n";
}

// ================================================================
// B. TEMPORAL ANTI-ALIASING (TAA)
// ================================================================
// TAA accumulates color samples across multiple frames to reduce aliasing.
//
// Each frame: jitter the projection matrix by a sub-pixel offset
//   (e.g., Halton sequence, 8-16 positions)
// The jitter causes the scene to be rendered from slightly different viewpoints.
// Accumulate in a "history buffer": blend current frame with previous frames.
//
// Basic algorithm:
//   1. Jitter projection matrix (sub-pixel offset)
//   2. Render current frame (jittered)
//   3. Reproject history: where was this pixel in the previous frame?
//      (using motion vectors from velocity buffer)
//   4. Clamp history sample to current neighborhood color box
//      (prevents ghosting when history is invalid)
//   5. Blend: output = lerp(history, current, alpha)  where alpha ≈ 0.1
//
// Motion vector: stores per-pixel (dx, dy) in screen space
//   = current_ndc - reprojected_previous_ndc
//   Computed in the vertex shader: output = curr_clip - prev_clip
//
// Ghosting: history from a pixel that has moved/changed -> looks smeared
//   Fix: velocity rejection, neighborhood clamping, variance clipping
//
// Flicker (temporal instability): thin features or specular highlights
//   Fix: sharpen after accumulation, use higher alpha

static void taaExplainer() {
    std::cout << "B. Temporal Anti-Aliasing (TAA)\n";
    std::cout << "   Core idea: jitter + accumulate across frames\n\n";
    std::cout << "   Halton(2,3) sub-pixel jitter sequence (8 positions):\n";

    // Halton sequence base 2 and 3
    auto halton = [](int i, int base) -> double {
        double r = 0, f = 1;
        while (i > 0) { f /= base; r += f * (i % base); i /= base; }
        return r;
    };

    std::cout << "   Frame | jitter_x | jitter_y\n";
    std::cout << "   ------|----------|----------\n";
    for (int i = 1; i <= 8; ++i) {
        double jx = halton(i, 2) - 0.5;
        double jy = halton(i, 3) - 0.5;
        std::cout << std::fixed << std::setprecision(4)
                  << "   " << std::setw(5) << i
                  << " | " << std::setw(8) << jx
                  << " | " << std::setw(8) << jy << "\n";
    }
    std::cout << "\n   These offsets cover the pixel area evenly over 8 frames.\n";
    std::cout << "\n   Algorithm (per frame):\n";
    std::cout << "   1. proj[2][0] += jitter_x * 2 / screenW  (jitter proj matrix)\n";
    std::cout << "   2. Render scene with jittered proj\n";
    std::cout << "   3. Sample history at reprojected UV (UV + motionVector)\n";
    std::cout << "   4. Clamp history to neighborhood min/max (anti-ghosting)\n";
    std::cout << "   5. output = lerp(history, current, 0.1)\n\n";
}

// ================================================================
// C. SCREEN SPACE REFLECTIONS (SSR)
// ================================================================
// Raymarches in screen space to find reflection hits.
//
// Algorithm per fragment:
//   1. Get world position P from depth buffer
//   2. Get world normal N from G-buffer
//   3. Compute reflection ray R = reflect(-V, N)
//   4. March R in screen space (project into NDC at each step)
//   5. Compare marched depth with depth buffer at that screen position
//   6. If depth difference < threshold: HIT, sample color at that UV
//   7. Otherwise: miss (fall back to reflection probe / skybox)
//
// Key challenges:
//   - Off-screen reflections: can't see what's outside the viewport
//   - Self-intersection: ray hits the surface it started from
//     (fix: step past the surface normal before marching)
//   - Thin-surface problem: flat ray nearly parallel to surface
//   - Performance: many ray steps -> use hierarchical Z-buffer

static void ssrExplainer() {
    std::cout << "C. Screen Space Reflections (SSR)\n";
    std::cout << "   Rays are marched in screen (NDC) space, not world space.\n\n";
    std::cout << "   Per-fragment algorithm:\n";
    std::cout << "   1. world_pos = reconstructFromDepth(uv, depth)\n";
    std::cout << "   2. N = normalize(gBuffer_normal)\n";
    std::cout << "   3. V = normalize(cameraPos - world_pos)\n";
    std::cout << "   4. R = reflect(-V, N)   // reflection direction\n";
    std::cout << "   5. March R in screen space:\n";
    std::cout << "      for i in 0..MAX_STEPS:\n";
    std::cout << "        march_pos = world_pos + R * stepSize * i\n";
    std::cout << "        screen_uv = projectToScreen(march_pos)\n";
    std::cout << "        sampled_depth = texture(depthBuffer, screen_uv)\n";
    std::cout << "        if march_pos.z > sampled_depth + eps: HIT! break\n";
    std::cout << "   6. On hit: reflection_color = texture(colorBuffer, screen_uv)\n";
    std::cout << "\n   Limitations: no reflections for off-screen objects\n";
    std::cout << "   Fade SSR at screen edges, blend with reflection probe\n\n";
}

// ================================================================
// D. SCREEN SPACE AMBIENT OCCLUSION (SSAO)
// ================================================================
// Approximates ambient occlusion by sampling the depth buffer.
//
// Algorithm per fragment:
//   1. Get world position P and normal N from G-buffer
//   2. Sample K random points in a hemisphere around N
//   3. For each sample S: project to screen space
//   4. Compare S.z with depth buffer at projected UV
//   5. If depth buffer < S.z: that sample is occluded
//   6. AO = 1 - (occluded samples / total samples)
//
// Noise reduction: use a small noise texture to randomize sample
// kernel, then blur the SSAO result in screen space.
// Range check: only count occlusion from nearby geometry.

static void ssaoExplainer() {
    std::cout << "D. SSAO (Screen Space Ambient Occlusion)\n";
    std::cout << "   Estimate how much ambient light reaches each surface point.\n\n";
    std::cout << "   Per-fragment algorithm:\n";
    std::cout << "   1. world_pos = from depth, world_normal = from G-buffer\n";
    std::cout << "   2. Build TBN (tangent space) from normal + random tangent\n";
    std::cout << "   3. For K random hemisphere samples s_i (in tangent space):\n";
    std::cout << "      world_sample = world_pos + TBN * s_i * radius\n";
    std::cout << "      screen_sample = project(world_sample)\n";
    std::cout << "      sample_depth  = texture(depthBuf, screen_sample.xy)\n";
    std::cout << "      if world_sample.z > sample_depth + bias: ++occluded\n";
    std::cout << "   4. AO = 1.0 - (occluded / K)\n";
    std::cout << "   5. Blur AO buffer (reduce noise from random samples)\n\n";
    std::cout << "   Typical: K=64 samples, radius=0.5, bias=0.025\n\n";
}

// ================================================================
// E. TONE MAPPING & HDR
// ================================================================
// HDR rendering pipeline:
//   1. Render scene in floating point buffers (16F or 32F)
//   2. Apply tone mapping to convert HDR -> LDR [0,1]
//   3. Apply gamma correction: display_value = linear_value ^ (1/2.2)
//
// Reinhard tone map: L_display = L_hdr / (1 + L_hdr)
// ACES filmic: industry standard, film-like contrast and color
// Exposure: scale L_hdr by exposure before tone mapping

static void tonemapExplainer() {
    std::cout << "E. Tone Mapping & HDR\n";
    std::cout << "   HDR linear values -> LDR display values -> gamma correction\n\n";

    auto reinhard = [](double L) { return L / (1.0 + L); };
    // Simplified ACES approximation (Krzysztof Narkowicz)
    auto aces = [](double x) -> double {
        const double a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;
        return std::max(0.0, std::min(1.0, (x*(a*x+b))/(x*(c*x+d)+e)));
    };
    auto gamma = [](double L) { return std::pow(L, 1.0/2.2); };

    std::cout << "   HDR value | Reinhard | ACES     | After gamma (ACES)\n";
    std::cout << "   --------- |----------|----------|-------------------\n";
    double hdrValues[] = {0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0};
    for (double v : hdrValues) {
        double r = reinhard(v);
        double a = aces(v);
        std::cout << std::fixed << std::setprecision(4)
                  << "   " << std::setw(9) << v
                  << " | " << std::setw(8) << r
                  << " | " << std::setw(8) << a
                  << " | " << std::setw(8) << gamma(a) << "\n";
    }
    std::cout << "\n   Gamma correction: display = linear^(1/2.2)\n";
    std::cout << "   Why? Monitors apply gamma 2.2, so we pre-correct.\n\n";
}

// ================================================================
// F. RENDER GRAPH / FRAME GRAPH
// ================================================================
// A directed acyclic graph (DAG) representing all render passes in a frame.
// Nodes = render passes, edges = resource dependencies.
// Used in: Frostbite, Unreal, Vulkan (via Render Pass objects).
//
// Benefits:
//   - Automatic resource lifetime management (no manual synchronization)
//   - GPU memory aliasing: reuse memory between non-overlapping passes
//   - GPU barrier insertion: automatically insert pipeline barriers
//   - Parallelism: schedule GPU work with minimal barriers

static void renderGraphExplainer() {
    std::cout << "F. Render Graph (Frame Graph)\n";
    std::cout << "   DAG of render passes with resource dependencies.\n\n";
    std::cout << "   Example frame (Deferred + Post):\n";
    std::cout << "   [Shadow Pass]     -> shadow_map\n";
    std::cout << "   [Geometry Pass]   -> GBuffer (albedo, normal, depth)\n";
    std::cout << "   [SSAO Pass]       <- GBuffer.normal, GBuffer.depth\n";
    std::cout << "                     -> AO buffer\n";
    std::cout << "   [Lighting Pass]   <- GBuffer.*, AO, shadow_map\n";
    std::cout << "                     -> HDR color buffer\n";
    std::cout << "   [TAA Pass]        <- HDR, history, motion_vectors\n";
    std::cout << "                     -> antialiased HDR\n";
    std::cout << "   [Tone Map Pass]   <- antialiased HDR -> LDR (final)\n\n";
    std::cout << "   Compile step: sort by dependency, insert GPU barriers\n";
    std::cout << "   Alias memory: shadow_map memory can be reused by post-process\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Deferred: why can't you do MSAA? (depth buffer already resolved per G-buffer)\n";
    std::cout << "  [ ] TAA: what is 'reprojection'? Move current pixel back to last frame's UV\n";
    std::cout << "  [ ] SSR: why do reflections disappear at screen edges? (off-screen data missing)\n";
    std::cout << "  [ ] SSAO radius in world space vs screen space: world is more consistent\n";
    std::cout << "  [ ] Tone map: why ACES over Reinhard? Filmic look, better highlights\n";
    std::cout << "  [ ] PBR pipeline order: geometry -> lighting (with BRDF) -> post-process\n";
}

int main() {
    std::cout << "================ Advanced Graphics Concepts (Conceptual) ================\n\n";
    forwardVsDeferred();
    taaExplainer();
    ssrExplainer();
    ssaoExplainer();
    tonemapExplainer();
    renderGraphExplainer();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: If you can only pick one anti-aliasing method for a modern real-time renderer, what do you pick and why?
A key points:
- TAA (or TSR-like temporal methods) gives strong quality/performance for subpixel stability.
- It requires good motion vectors, history rejection, and post-sharpen to control ghosting.
- In practice combine with upscaling strategy and content-specific tuning.
*/
