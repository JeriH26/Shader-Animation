// ================================================================
// 15_interpolation.cpp  -- lerp and slerp
// ================================================================
// lerp (Linear Interpolation):
//   lerp(a, b, t) = a*(1-t) + b*t  = a + t*(b-a)
//   - Simple, fast
//   - For scalars: works perfectly
//   - For unit vectors: DOES NOT maintain unit length, and angular
//     velocity is NOT constant (faster near endpoints)
//
// nlerp (Normalized lerp):
//   nlerp(q0, q1, t) = normalize(lerp(q0, q1, t))
//   - Faster than slerp, approximate slerp
//   - Maintains unit length, but still non-constant angular velocity
//
// slerp (Spherical Linear Interpolation):
//   slerp(q0, q1, t) = sin((1-t)*θ)/sin(θ)*q0 + sin(t*θ)/sin(θ)*q1
//   where θ = acos(dot(q0, q1))
//   - Constant angular velocity along great circle arc
//   - Used for quaternion rotation interpolation
//   - When θ ≈ 0: use lerp (degenerate case)
//
// Practical tip:
//   If dot(q0,q1) < 0: negate q1 to take the SHORT path (slerp would
//   go the long way around the sphere otherwise)
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec2 { double x=0, y=0; };
struct Quat { double w=1, x=0, y=0, z=0; };

static double dot2(Vec2 a, Vec2 b) { return a.x*b.x + a.y*b.y; }
static double len2(Vec2 a) { return std::sqrt(a.x*a.x + a.y*a.y); }
static Vec2 normalize2(Vec2 a) { double l=len2(a); return {a.x/l,a.y/l}; }

static double dotQ(Quat a, Quat b) { return a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z; }
static double lenQ(Quat a) { return std::sqrt(a.w*a.w+a.x*a.x+a.y*a.y+a.z*a.z); }
static Quat normalizeQ(Quat a) { double l=lenQ(a); return {a.w/l,a.x/l,a.y/l,a.z/l}; }

// ---- Scalar lerp ----
static double lerp(double a, double b, double t) { return a + t*(b-a); }

// ---- 2D vector lerp (NOT unit-preserving) ----
static Vec2 lerpVec(Vec2 a, Vec2 b, double t) {
    return {lerp(a.x,b.x,t), lerp(a.y,b.y,t)};
}

// ---- 2D vector slerp (for unit vectors) ----
static Vec2 slerpVec(Vec2 a, Vec2 b, double t) {
    double cosTheta = dot2(a, b);
    // Clamp to avoid acos domain error
    if (cosTheta > 0.9999) return normalizeQ({0, lerpVec(a,b,t).x, lerpVec(a,b,t).y, 0}),
                                  lerpVec(a,b,t);  // nearly same, use lerp
    if (cosTheta < -0.9999) {
        // 180° apart: undefined, use lerp
        return lerpVec(a, b, t);
    }
    double theta = std::acos(cosTheta);
    double s0 = std::sin((1-t)*theta) / std::sin(theta);
    double s1 = std::sin(t*theta)     / std::sin(theta);
    return {s0*a.x + s1*b.x, s0*a.y + s1*b.y};
}

// ---- Quaternion slerp ----
static Quat slerpQuat(Quat q0, Quat q1, double t) {
    double cosTheta = dotQ(q0, q1);

    // Take short path: if dot < 0, negate q1
    if (cosTheta < 0.0) {
        q1 = {-q1.w, -q1.x, -q1.y, -q1.z};
        cosTheta = -cosTheta;
    }

    // When nearly identical: fallback to lerp+normalize
    if (cosTheta > 0.9999) {
        Quat r = { lerp(q0.w,q1.w,t), lerp(q0.x,q1.x,t),
                   lerp(q0.y,q1.y,t), lerp(q0.z,q1.z,t) };
        return normalizeQ(r);
    }

    double theta = std::acos(cosTheta);
    double s0 = std::sin((1-t)*theta) / std::sin(theta);
    double s1 = std::sin(t*theta)     / std::sin(theta);
    return {
        s0*q0.w + s1*q1.w,
        s0*q0.x + s1*q1.x,
        s0*q0.y + s1*q1.y,
        s0*q0.z + s1*q1.z
    };
}

static void printTheory() {
    std::cout << "================ Interpolation: lerp & slerp ================\n";
    std::cout << "lerp(a, b, t) = a + t*(b-a)\n";
    std::cout << "  Works for scalars and vectors, but NOT for unit vectors or rotations\n\n";
    std::cout << "slerp on unit vectors: spherical interpolation\n";
    std::cout << "  θ = acos(dot(a,b))\n";
    std::cout << "  slerp = sin((1-t)*θ)/sin(θ) * a  +  sin(t*θ)/sin(θ) * b\n";
    std::cout << "  Maintains unit length, constant angular velocity\n\n";
    std::cout << "Quaternion slerp: same formula, but check dot < 0 first!\n";
    std::cout << "  If dot(q0,q1) < 0: negate q1 to take the shorter arc\n";
    std::cout << "==============================================================\n\n";
}

static void lerpVsSlerpDemo() {
    std::cout << "---- Lerp vs Slerp on unit 2D vectors ----\n";
    std::cout << "(0°) a=(1,0)   vs   (90°) b=(0,1)\n\n";

    Vec2 a = {1, 0};
    Vec2 b = {0, 1};

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "   t    |  lerp (len)  |  slerp (len)  |  angle\n";
    std::cout << "--------|-------------|---------------|--------\n";
    for (double t = 0.0; t <= 1.001; t += 0.25) {
        Vec2 L = lerpVec(a, b, t);
        Vec2 S = slerpVec(a, b, t);
        double angleL = std::atan2(L.y, L.x) * 180.0 / 3.14159;
        double angleS = std::atan2(S.y, S.x) * 180.0 / 3.14159;
        std::cout << "  t=" << t
                  << " | len=" << len2(L)
                  << " | len=" << len2(S)
                  << " | lerp_angle=" << angleL << "°"
                  << " slerp_angle=" << angleS << "°\n";
    }
    std::cout << "\nObservation:\n";
    std::cout << "  lerp: length dips below 1.0 in the middle (it's not unit!).\n";
    std::cout << "  slerp: length stays 1.0, angle increases uniformly by 22.5° each step.\n\n";
}

static void quaternionSlerpDemo() {
    std::cout << "---- Quaternion slerp demo ----\n";
    // q0 = identity (0° rotation)
    // q1 = 90° rotation around Z axis: (cos45°, 0, 0, sin45°)
    Quat q0 = {1, 0, 0, 0};
    Quat q1 = {std::cos(45.0*3.14159/180.0), 0, 0, std::sin(45.0*3.14159/180.0)};

    std::cout << "  q0 = identity (0° rotation)\n";
    std::cout << "  q1 = 90° around Z: w=" << q1.w << " z=" << q1.z << "\n\n";

    std::cout << "   t   |  w       x       y       z       |  |q|\n";
    std::cout << "-------|-----------------------------------|-------\n";
    for (double t = 0.0; t <= 1.001; t += 0.25) {
        Quat q = slerpQuat(q0, q1, t);
        double angle = 2.0 * std::acos(q.w) * 180.0 / 3.14159;
        std::cout << "  t=" << t
                  << " | " << q.w << "  " << q.x << "  " << q.y << "  " << q.z
                  << " | " << lenQ(q)
                  << "  angle=" << angle << "°\n";
    }
    std::cout << "\nObservation: angle increases linearly (constant angular velocity)\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] When to use nlerp vs slerp? nlerp: faster, fine for small angles\n";
    std::cout << "  [ ] Lerp on euler angles causes 'shortest path' issues — use slerp on quaternions\n";
    std::cout << "  [ ] Smooth step: lerp(a,b, t*t*(3-2*t)) for ease-in, ease-out\n";
    std::cout << "  [ ] In GLSL: mix(a,b,t) == lerp(a,b,t)\n";
}

int main() {
    printTheory();
    lerpVsSlerpDemo();
    quaternionSlerpDemo();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: When is nlerp preferred over slerp in production code?
A key points:
- nlerp is cheaper and often sufficient for small-angle blends.
- It preserves unit length after normalization but not constant angular velocity.
- slerp is preferred for cinematic-quality or large-angle, timing-sensitive rotation.
*/
