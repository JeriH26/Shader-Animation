// ================================================================
// 05_ray_obb.cpp  -- Ray vs OBB (Oriented Bounding Box)
// ================================================================
// OBB = like AABB, but can be rotated arbitrarily.
// Defined by: center C, 3 orthonormal axes A0/A1/A2, half-extents h0/h1/h2
//
// The slab test is identical to AABB, but we project the ray onto
// each OBB axis instead of the world axes.
//
// For axis i:
//   e  = dot(Ai, O - C)   <- ray origin projected onto OBB axis i
//   f  = dot(Ai, D)        <- ray direction projected onto OBB axis i
//
//   slab range in axis i: t1 = (-hi - e)/f,  t2 = (hi - e)/f
//   (same slab logic as AABB after this point)
//
// Key insight: the dot projection effectively transforms the ray
// into the OBB's local coordinate frame.
// ================================================================

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator-(const Vec3& r) const { return {x-r.x, y-r.y, z-r.z}; }
};
static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static double len(const Vec3& a) { return std::sqrt(dot(a,a)); }
static Vec3 normalize(const Vec3& a) { double l=len(a); return {a.x/l,a.y/l,a.z/l}; }

struct Ray { Vec3 origin, direction; };

// OBB: center + 3 orthonormal axes + 3 half-extents
struct OBB {
    Vec3   center;
    Vec3   axis[3];   // must be unit vectors
    double half[3];
};

struct HitInfo { bool hit=false; double tNear=-1, tFar=-1; };

// ---- Core slab test (projected onto OBB axes) ----
HitInfo intersectRayOBB(const Ray& ray, const OBB& obb) {
    Vec3   delta  = ray.origin - obb.center;   // vector from OBB center to ray origin
    double tEnter = -1e30;
    double tExit  =  1e30;

    for (int i = 0; i < 3; ++i) {
        double e = dot(obb.axis[i], delta);           // origin projected onto axis
        double f = dot(obb.axis[i], ray.direction);   // direction projected onto axis

        if (std::abs(f) < 1e-9) {
            // Ray parallel to this slab pair
            if (e < -obb.half[i] || e > obb.half[i]) return {};   // outside -> miss
            continue;
        }

        double t1 = (-obb.half[i] - e) / f;
        double t2 = ( obb.half[i] - e) / f;
        if (t1 > t2) std::swap(t1, t2);

        tEnter = std::max(tEnter, t1);
        tExit  = std::min(tExit,  t2);
        if (tEnter > tExit) return {};
    }

    if (tExit < 0.0) return {};

    HitInfo r;
    r.hit   = true;
    r.tNear = (tEnter >= 0.0) ? tEnter : 0.0;
    r.tFar  = tExit;
    return r;
}

// ---- Helper: build OBB from rotation angle around Y ----
static OBB makeOBB(Vec3 center, double rotY, double hx, double hy, double hz) {
    double c = std::cos(rotY), s = std::sin(rotY);
    OBB o;
    o.center   = center;
    o.axis[0]  = { c, 0,  s};   // local X rotated
    o.axis[1]  = { 0, 1,  0};   // local Y unchanged
    o.axis[2]  = {-s, 0,  c};   // local Z rotated
    o.half[0]  = hx; o.half[1] = hy; o.half[2] = hz;
    return o;
}

static void printTheory() {
    std::cout << "================ Ray-OBB (Oriented Bounding Box) ================\n";
    std::cout << "Difference from AABB: OBB can be rotated (has 3 arbitrary unit axes)\n\n";
    std::cout << "For each local axis Ai:\n";
    std::cout << "  e  = dot(Ai, O - center)   <- origin projected onto Ai\n";
    std::cout << "  f  = dot(Ai, D)            <- direction projected onto Ai\n";
    std::cout << "  t1 = (-half_i - e) / f\n";
    std::cout << "  t2 = ( half_i - e) / f\n";
    std::cout << "  sort t1,t2 and apply slab combine (same as AABB)\n\n";
    std::cout << "This is equivalent to: transform ray to OBB local space, do AABB test\n";
    std::cout << "==================================================================\n\n";
}

struct TC { std::string name, note; Ray ray; OBB obb; bool exp; };

static void runTests() {
    static const double PI = 3.14159265358979;

    OBB aligned = makeOBB({0,0,0}, 0.0,      1.0, 1.0, 1.0);  // same as AABB
    OBB rotated = makeOBB({0,0,0}, PI*0.25,  1.0, 1.0, 1.0);  // rotated 45° around Y

    // Aligned box max x-reach = 1.0
    // Rotated box max x-reach = 1*cos45 + 1*sin45 = sqrt(2) ≈ 1.414

    std::vector<TC> tests = {
        {"Aligned  | x=0 direct hit",
         "Shoot through center",
         {{0,0,-5},{0,0,1}}, aligned, true},

        {"Aligned  | x=1.2 miss",
         "1.2 > half=1.0, misses aligned box",
         {{1.2,0,-5},{0,0,1}}, aligned, false},

        {"Rotated  | x=0 direct hit",
         "Shoot through center of rotated box",
         {{0,0,-5},{0,0,1}}, rotated, true},

        {"Rotated  | x=1.2 HIT  (AABB would miss)",
         "Rotated box extends to x=1.414, so x=1.2 is inside",
         {{1.2,0,-5},{0,0,1}}, rotated, true},

        {"Rotated  | x=1.5 miss",
         "1.5 > sqrt(2)=1.414: outside even the rotated box",
         {{1.5,0,-5},{0,0,1}}, rotated, false},
    };

    std::cout << std::fixed << std::setprecision(4);
    int pass = 0;
    for (auto& t : tests) {
        auto h = intersectRayOBB(t.ray, t.obb);
        bool ok = (h.hit == t.exp);
        if (ok) ++pass;
        std::cout << t.name << "\n  " << t.note << "\n";
        std::cout << "  Expected:" << (t.exp?"Hit":"Miss")
                  << "  Actual:" << (h.hit?"Hit":"Miss")
                  << " -> " << (ok?"PASS":"FAIL") << "\n";
        if (h.hit)
            std::cout << "  tNear=" << h.tNear << "  tFar=" << h.tFar << "\n";
        std::cout << "\n";
    }
    std::cout << "Summary: " << pass << "/" << tests.size() << " passed\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] OBB axes come from the columns of the object's rotation matrix\n";
    std::cout << "  [ ] OBB gives tighter fit than AABB for rotated objects -> fewer BVH false hits\n";
    std::cout << "  [ ] For dynamic objects, OBB needs to be rebuilt each frame (expensive)\n";
    std::cout << "  [ ] Common trade-off: AABB = cheap rebuild, OBB = tighter fit\n";
    std::cout << "  [ ] Separating Axis Theorem (SAT) is the general form of this test\n";
}

int main() {
    printTheory();
    runTests();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Is ray-OBB intersection fundamentally different from ray-AABB intersection?
A key points:
- No, it is the same slab idea in a different basis.
- Either project ray onto OBB local axes or transform ray into OBB local space.
- Then perform standard AABB slab intersection with local half extents.
*/
