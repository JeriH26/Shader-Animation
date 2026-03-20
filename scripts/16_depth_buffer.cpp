// ================================================================
// 16_depth_buffer.cpp  -- Depth Buffer Precision & Z-Fighting
// ================================================================
// Hardware stores depth as a value in [0,1] (or [-1,1] for OpenGL NDC).
// The mapping from view-space z to NDC z is NON-LINEAR:
//
//   z_ndc = (f+n)/(f-n) + (2*f*n)/((f-n)*z_view)    (OpenGL, z_view < 0)
//
// Simplified for z > 0 (reversing sign convention):
//   z_ndc = (f+n)/(f-n) - (2*f*n)/((f-n)*z)
//
// Result:
//   Most depth buffer precision is concentrated near the NEAR PLANE.
//   Objects far from camera suffer from low precision -> Z-FIGHTING.
//
// Quantization:
//   With D-bit depth buffer: 2^D discrete levels.
//   The "depth resolution" at a given z:
//     delta_z = (f-n) / (2^D * (f+n - 2*f*n/z_view))  (approximate)
//
// Practical numbers (24-bit, near=0.1, far=1000):
//   At z=0.2:  resolution ≈ submicron   <- excellent
//   At z=500:  resolution ≈ 0.5 meters  <- terrible! only ≈2 levels/meter
//
// Z-Fighting: two coplanar (or nearly so) surfaces map to the same
//   depth value -> they flicker as rounding alternates which is "in front"
//
// Solutions:
//   1. Increase near plane (most effective — moving near 0.1->1 gives 10x improvement)
//   2. Decrease far/near ratio (improve precision spread)
//   3. Use 32-bit float depth buffer
//   4. Reverse-Z: map near->1, far->0 for better float precision
//   5. Polygon offset (glPolygonOffset) for decals
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

// Convert view-space z (positive = in front of camera) to NDC z in [0,1]
// (DirectX convention, z_ndc = (f / (f-n)) + (-f*n / (f-n)) / z)
static double viewZtoNDC_DX(double z, double n, double f) {
    return (f / (f - n)) * (1.0 - n / z);
}

// OpenGL convention: NDC z in [-1,1]
// z_ndc = (f+n)/(f-n) - (2fn)/((f-n)*z)  where z is positive view depth
static double viewZtoNDC_GL(double z, double n, double f) {
    return (f + n) / (f - n) - (2.0 * f * n) / ((f - n) * z);
}

// Convert NDC z back to view z (OpenGL convention)
static double NDCtoViewZ_GL(double z_ndc, double n, double f) {
    // z_ndc*(f-n)*z = (f+n)*z - 2fn
    // z * (z_ndc*(f-n) - (f+n)) = -2fn
    return (2.0 * f * n) / ((f + n) - z_ndc * (f - n));
}

static void printTheory() {
    std::cout << "================ Depth Buffer Precision ================\n";
    std::cout << "View z -> NDC z mapping (OpenGL, z > 0 = in front):\n";
    std::cout << "  z_ndc = (f+n)/(f-n) - 2fn / ((f-n)*z)\n\n";
    std::cout << "Key insight: this is HYPERBOLIC, not linear!\n";
    std::cout << "  Most precision is near the NEAR plane.\n";
    std::cout << "  Far objects get almost NO precision -> Z-fighting.\n\n";
    std::cout << "Z-fighting: two surfaces so close their NDC z values\n";
    std::cout << "  round to the same integer depth -> flickering\n\n";
    std::cout << "Fixes: increase near plane, 32-bit depth, reverse-Z\n";
    std::cout << "========================================================\n\n";
}

static void precisionTable() {
    const double near = 0.1, far = 1000.0;
    const int BITS = 24;
    const long LEVELS = (1L << BITS);  // 16777216

    std::cout << "Depth precision table (near=0.1, far=1000, 24-bit depth buffer)\n";
    std::cout << "  Depth levels available: " << LEVELS << " (2^24)\n\n";
    std::cout << "  View z  | NDC z   | Buffer val | Metres/level\n";
    std::cout << "  --------|---------|------------|-------------\n";

    double zValues[] = {0.11, 0.2, 0.5, 1.0, 5.0, 10.0, 50.0, 100.0, 500.0, 999.0};
    for (double z : zValues) {
        double ndc = viewZtoNDC_GL(z, near, far);
        double ndc01 = (ndc + 1.0) * 0.5;  // map [-1,1] to [0,1]
        long bufVal = (long)(ndc01 * LEVELS);

        // Depth resolution: increase buffer value by 1, how much does z change?
        double ndc_next = (bufVal + 1.0) / LEVELS * 2.0 - 1.0;
        double z_next   = NDCtoViewZ_GL(ndc_next, near, far);
        double resolution = std::abs(z_next - z);

        std::cout << std::fixed << std::setprecision(4)
                  << "  z=" << std::setw(7) << z
                  << " | ndc=" << std::setw(7) << ndc01
                  << " | buf=" << std::setw(10) << bufVal
                  << " | " << std::scientific << resolution << " m/level\n";
    }
    std::cout << "\n";
}

static void zFightingDemo() {
    std::cout << "Z-Fighting demo: two planes at z=500.0 and z=500.1\n";
    const double near = 0.1, far = 1000.0;
    const int BITS = 24;
    const long LEVELS = 1L << BITS;

    auto toBuffer = [&](double z) -> long {
        double ndc01 = (viewZtoNDC_GL(z, near, far) + 1.0) * 0.5;
        return (long)(ndc01 * LEVELS);
    };

    long buf500_0 = toBuffer(500.0);
    long buf500_1 = toBuffer(500.1);

    std::cout << "  z=500.0 -> depth buffer value = " << buf500_0 << "\n";
    std::cout << "  z=500.1 -> depth buffer value = " << buf500_1 << "\n";
    if (buf500_0 == buf500_1) {
        std::cout << "  SAME VALUE -> Z-FIGHTING will occur!\n\n";
    } else {
        std::cout << "  Different values, diff=" << (buf500_1-buf500_0) << " levels\n\n";
    }

    // Compare near plane precision
    std::cout << "Near-plane check: two planes at z=0.10 and z=0.11\n";
    long bufN0 = toBuffer(0.10);
    long bufN1 = toBuffer(0.11);
    std::cout << "  z=0.10 -> depth buffer value = " << bufN0 << "\n";
    std::cout << "  z=0.11 -> depth buffer value = " << bufN1 << "\n";
    std::cout << "  diff = " << (bufN1 - bufN0) << " levels (HUGE precision near the near plane)\n\n";
}

static void reverseZDemo() {
    std::cout << "Reverse-Z insight:\n";
    std::cout << "  Standard: near=0 -> 0.0, far=1000 -> 1.0 (most precision near 0)\n";
    std::cout << "  Reverse-Z: near=1000 -> 0.0, far=0.1 -> 1.0\n";
    std::cout << "  With float32 depth, floats have MORE precision near 0.0\n";
    std::cout << "  Reverse-Z puts far objects near 0.0 -> better far precision!\n";
    std::cout << "  In practice: flip projection matrix sign + change depth test to GREATER\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Why is depth non-linear? Need to store 1/z for perspective-correct interpolation\n";
    std::cout << "  [ ] near=0.1, far=10000 is a bad ratio: near=1.0, far=10000 is 10x better\n";
    std::cout << "  [ ] glDepthRange(0,1): map [-1,1] NDC to [0,1] depth buffer\n";
    std::cout << "  [ ] glPolygonOffset(1.0, 1.0): push depth slightly for decals/wireframe\n";
    std::cout << "  [ ] Logarithmic depth buffer: z_buf = log(1+z/near) / log(1+far/near)\n";
}

int main() {
    printTheory();
    precisionTable();
    zFightingDemo();
    reverseZDemo();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why does increasing near plane usually help depth precision much more than reducing far plane?
A key points:
- Depth mapping is hyperbolic with precision concentrated near near plane.
- Far/near ratio dominates quantization distribution.
- Raising near plane directly redistributes more usable precision across scene depth.
*/
