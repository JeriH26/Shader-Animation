// ================================================================
// 17_alpha_blending.cpp  -- Alpha Blending
// ================================================================
// Alpha (α) represents opacity: α=1 fully opaque, α=0 fully transparent.
//
// STANDARD blend equation (Porter-Duff "over"):
//   result.rgb = src.rgb * src.a + dst.rgb * (1 - src.a)
//   result.a   = src.a + dst.a * (1 - src.a)
//
// In OpenGL: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
//
// PREMULTIPLIED ALPHA:
//   src.rgb is pre-multiplied: src_pre.rgb = src.rgb * src.a
//   Blend equation becomes simpler:
//   result.rgb = src_pre.rgb + dst.rgb * (1 - src.a)
//   In OpenGL: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
//
// WHY PREMULTIPLIED is better:
//   1. Correct filtering/mipping: interpolating premultiplied values
//      gives correct results. Standard alpha causes "dark fringe" at
//      transparent edges after bilinear filtering.
//   2. Compositing chains work correctly (associativity)
//   3. Most professional tools (Photoshop, Nuke) use premultiplied
//
// ORDER MATTERS for transparent objects!
//   Must render back-to-front (painter's algorithm) for correct blending.
//   Alternatives: OIT (Order-Independent Transparency), depth peeling.
//
// Additive blending (for particles/glow/fire):
//   result.rgb = src.rgb * src.a + dst.rgb
//   In OpenGL: glBlendFunc(GL_SRC_ALPHA, GL_ONE)
//   Note: order doesn't matter! (commutative)
// ================================================================

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

struct Color { double r=0, g=0, b=0, a=1; };

static void printColor(const char* name, Color c) {
    std::cout << std::fixed << std::setprecision(3)
              << "  " << name
              << " rgb=(" << c.r << "," << c.g << "," << c.b << ")"
              << " a=" << c.a << "\n";
}

// Standard blend: GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
static Color blendStandard(Color src, Color dst) {
    double inv = 1.0 - src.a;
    return {
        src.r * src.a + dst.r * inv,
        src.g * src.a + dst.g * inv,
        src.b * src.a + dst.b * inv,
        src.a + dst.a * inv
    };
}

// Premultiplied blend: GL_ONE, GL_ONE_MINUS_SRC_ALPHA
// (src.rgb is ALREADY multiplied by src.a)
static Color blendPremultiplied(Color src_pre, Color dst) {
    // src_pre.rgb = original_rgb * src.a
    double inv = 1.0 - src_pre.a;
    return {
        src_pre.r + dst.r * inv,
        src_pre.g + dst.g * inv,
        src_pre.b + dst.b * inv,
        src_pre.a + dst.a * inv
    };
}

// Convert standard color to premultiplied
static Color toPremultiplied(Color c) {
    return {c.r * c.a, c.g * c.a, c.b * c.a, c.a};
}

// Additive blend: GL_SRC_ALPHA, GL_ONE
static Color blendAdditive(Color src, Color dst) {
    return {
        std::min(1.0, src.r * src.a + dst.r),
        std::min(1.0, src.g * src.a + dst.g),
        std::min(1.0, src.b * src.a + dst.b),
        dst.a
    };
}

static void printTheory() {
    std::cout << "================ Alpha Blending ================\n";
    std::cout << "Standard (over): result = src*src.a + dst*(1-src.a)\n";
    std::cout << "  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)\n\n";
    std::cout << "Premultiplied: src.rgb already *= src.a beforehand\n";
    std::cout << "  result = src.rgb + dst*(1-src.a)\n";
    std::cout << "  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)\n\n";
    std::cout << "Additive: result = src*src.a + dst  (ignores dst alpha)\n";
    std::cout << "  glBlendFunc(GL_SRC_ALPHA, GL_ONE)  -- for fire, glow\n\n";
    std::cout << "Order matters for alpha (not for additive)!\n";
    std::cout << "  Draw back-to-front (painter's algorithm)\n";
    std::cout << "=================================================\n\n";
}

static void blendingDemo() {
    std::cout << "-- Blending demo: semi-transparent red over opaque blue --\n";
    Color background = {0, 0, 1, 1};       // blue, fully opaque
    Color overlay    = {1, 0, 0, 0.5};     // red, 50% transparent

    Color result1 = blendStandard(overlay, background);
    std::cout << "Standard blend:\n";
    printColor("background", background);
    printColor("overlay   ", overlay);
    printColor("result    ", result1);
    std::cout << "  Expected: purple-ish (0.5 red + 0.5 blue)\n\n";

    // Same with premultiplied
    Color overlay_pre = toPremultiplied(overlay);
    Color result2 = blendPremultiplied(overlay_pre, background);
    std::cout << "Premultiplied blend (same visual result):\n";
    printColor("overlay_pre", overlay_pre);
    printColor("result     ", result2);
    std::cout << "  Same result as standard when properly converted\n\n";
}

static void darkFringeDemo() {
    std::cout << "-- Dark fringe artifact with standard alpha (bilinear filter sim) --\n";
    std::cout << "Imagine a red circle on transparent background, with bilinear edge filter.\n";
    std::cout << "At the border: half pixel is (1,0,0,1), half is (0,0,0,0)\n\n";

    // Bilinear interpolation of two pixels at t=0.5
    Color opaque     = {1, 0, 0, 1};    // red, fully opaque
    Color transparent= {0, 0, 0, 0};    // transparent black (the WRONG storage)
    Color transparent_pre = {0, 0, 0, 0}; // transparent in premultiplied is the same here

    // Standard: interpolate rgba naively (what happens in bilinear filter)
    Color filtered_standard = {
        (opaque.r + transparent.r) * 0.5,
        (opaque.g + transparent.g) * 0.5,
        (opaque.b + transparent.b) * 0.5,
        (opaque.a + transparent.a) * 0.5
    };
    // Premultiplied: transparent pixel stores (0,0,0,0), opaque stores (1,0,0,1)
    Color filtered_pre = {
        (opaque.r + transparent_pre.r) * 0.5,  // 0.5 (weighted properly)
        0, 0,
        (opaque.a + transparent_pre.a) * 0.5   // a=0.5
    };  // After blend this reads as r=0.5/0.5=1.0

    std::cout << "Standard filtered edge:     rgb=(" << filtered_standard.r
              << "," << filtered_standard.g << "," << filtered_standard.b
              << ") a=" << filtered_standard.a
              << " -> display r=" << filtered_standard.r * filtered_standard.a
              << " (DARK FRINGE: should be 0.5 not " << filtered_standard.r * filtered_standard.a << ")\n";
    std::cout << "Premultiplied filtered edge: r=" << filtered_pre.r
              << " a=" << filtered_pre.a
              << " -> display r=" << filtered_pre.r
              << " (CORRECT: 0.5 as expected)\n\n";
    std::cout << "Key: premultiplied alpha eliminates the dark fringe at transparent edges.\n\n";
}

static void orderMattersDemo() {
    std::cout << "-- Order matters for standard alpha blending --\n";
    Color dst = {0, 0, 1, 1};        // blue background
    Color c1  = {1, 0, 0, 0.5};      // red, 50% transparent
    Color c2  = {0, 1, 0, 0.5};      // green, 50% transparent

    std::cout << "Background: blue\n";
    std::cout << "Layer A: red 50%\nLayer B: green 50%\n\n";

    // Correct order: A then B on top (draw A first, B is closer to camera)
    Color r1 = blendStandard(c2, blendStandard(c1, dst));
    // Wrong order
    Color r2 = blendStandard(c1, blendStandard(c2, dst));

    std::cout << "B(green) over A(red) over blue: rgb=("
              << r1.r << "," << r1.g << "," << r1.b << ")\n";
    std::cout << "A(red) over B(green) over blue: rgb=("
              << r2.r << "," << r2.g << "," << r2.b << ")\n";
    std::cout << "Different results! Must render back-to-front.\n\n";
}

static void additiveDemo() {
    std::cout << "-- Additive blending (particles/glow) --\n";
    Color bg = {0.1, 0.1, 0.1, 1};   // dark background
    Color fire = {1, 0.5, 0, 0.8};    // orange fire particle

    Color r1 = blendAdditive(fire, bg);
    Color r2 = blendAdditive(bg, fire);  // reversed order
    std::cout << "fire over bg: rgb=(" << r1.r << "," << r1.g << "," << r1.b << ")\n";
    std::cout << "bg over fire: rgb=(" << r2.r << "," << r2.g << "," << r2.b << ")\n";
    std::cout << "Same result (within float precision) — order doesn't matter for additive!\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] glEnable(GL_BLEND) + glBlendFunc(...) to enable alpha blending\n";
    std::cout << "  [ ] Why transparent objects are drawn AFTER opaque? Depth writes would block them\n";
    std::cout << "  [ ] Alpha testing vs alpha blending: test is a threshold (discards), blend is smooth\n";
    std::cout << "  [ ] OIT (Order-Independent Transparency): A-buffer, weighted OIT for no-sort blending\n";
}

int main() {
    printTheory();
    blendingDemo();
    darkFringeDemo();
    orderMattersDemo();
    additiveDemo();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why does premultiplied alpha reduce dark halos around sprite edges?
A key points:
- RGB is already weighted by alpha before filtering/mipmapping.
- Texture filtering then mixes physically consistent premultiplied colors.
- Standard alpha often blends with implicit black edge colors, causing dark fringes.
*/
