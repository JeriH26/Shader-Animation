// ================================================================
// 02_ray_plane.cpp  -- Ray vs Plane
// ================================================================
// Plane equation: dot(N, P) = d
//   N : unit normal of the plane
//   d : dot(N, any point on the plane)
//
// Ray: P(t) = O + tD, t >= 0
//
// Substituting ray into plane equation:
//   dot(N, O + tD) = d
//   dot(N,O) + t * dot(N,D) = d
//   t = (d - dot(N,O)) / dot(N,D)
//
// Special cases:
//   dot(N,D) == 0  -> ray is parallel to plane -> miss
//   t < 0          -> intersection behind origin -> miss
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
    Vec3 operator-(const Vec3& r) const { return {x-r.x, y-r.y, z-r.z}; }
    Vec3 operator*(double s)      const { return {x*s,   y*s,   z*s  }; }
};

static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static double len(const Vec3& a) { return std::sqrt(dot(a,a)); }
static Vec3 normalize(const Vec3& a) { double l=len(a); return {a.x/l, a.y/l, a.z/l}; }
static Vec3 reflect(const Vec3& d, const Vec3& n) { return d - n * (2.0*dot(d,n)); }

struct Ray   { Vec3 origin, direction; };
// Plane: dot(N, P) = d
struct Plane { Vec3 normal; double d; };

struct HitInfo { bool hit=false; double t=-1; Vec3 point{}, normal{}; };

// ---- Core intersection function ----
HitInfo intersectRayPlane(const Ray& ray, const Plane& plane, bool oneSided=false) {
    double denom = dot(plane.normal, ray.direction);
    // Parallel check
    if (std::abs(denom) < 1e-9) return {};
    // One-sided: skip backface
    if (oneSided && denom > 0.0) return {};

    double t = (plane.d - dot(plane.normal, ray.origin)) / denom;
    if (t < 0.0) return {};

    HitInfo r;
    r.hit    = true;
    r.t      = t;
    r.point  = ray.origin + ray.direction * t;
    r.normal = plane.normal;
    return r;
}

// ---- Build plane from 3 points ----
Plane planeFrom3Points(const Vec3& a, const Vec3& b, const Vec3& c) {
    Vec3 n = normalize(Vec3{
        (b.y-a.y)*(c.z-a.z) - (b.z-a.z)*(c.y-a.y),
        (b.z-a.z)*(c.x-a.x) - (b.x-a.x)*(c.z-a.z),
        (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x)
    });
    return {n, dot(n, a)};
}

// ---- Theory printout ----
static void printTheory() {
    std::cout << "================ Ray-Plane Intersection ================\n";
    std::cout << "Plane: dot(N, P) = d\n";
    std::cout << "Ray:   P(t) = O + tD\n\n";
    std::cout << "Derivation:\n";
    std::cout << "  dot(N, O + tD) = d\n";
    std::cout << "  dot(N,O) + t*dot(N,D) = d\n";
    std::cout << "  t = (d - dot(N,O)) / dot(N,D)\n\n";
    std::cout << "Cases:\n";
    std::cout << "  dot(N,D) == 0  -> parallel -> miss\n";
    std::cout << "  t < 0          -> behind origin -> miss\n";
    std::cout << "  denom > 0      -> hit backface (use oneSided flag to cull)\n";
    std::cout << "========================================================\n\n";
}

// ---- Test cases ----
struct TestCase { std::string name, note; Ray ray; Plane plane; bool expectedHit; };

static void runTests() {
    Plane ground  = {{0,1,0},  0.0};   // y=0 floor, normal up
    Plane wall    = {{0,0,1},  0.0};   // z=0 wall,  normal toward camera
    Plane tilted  = planeFrom3Points({0,0,0},{1,0,1},{0,1,0});

    std::vector<TestCase> tests = {
        {"Case A: ray hits ground",
         "Shooting down at y=0 plane",
         {{0,5,0},{0,-1,0}}, ground, true},

        {"Case B: ray parallel to ground",
         "Horizontal ray, dot(N,D)=0",
         {{0,1,0},{1,0,0}}, ground, false},

        {"Case C: ray points away",
         "t would be negative",
         {{0,5,0},{0,1,0}}, ground, false},

        {"Case D: diagonal to wall",
         "Ray going along +Z hits z=0 wall",
         {{0,0,-5},{0,0,1}}, wall, true},

        {"Case E: tilted plane",
         "Plane built from 3 points",
         {{0,3,0},{0,-1,0.1}}, tilted, true},
    };

    std::cout << std::fixed << std::setprecision(4);
    int pass = 0;
    for (auto& t : tests) {
        auto h = intersectRayPlane(t.ray, t.plane);
        bool ok = (h.hit == t.expectedHit);
        if (ok) ++pass;
        std::cout << t.name << "\n  " << t.note << "\n";
        std::cout << "  Expected:" << (t.expectedHit?"Hit":"Miss")
                  << "  Actual:" << (h.hit?"Hit":"Miss")
                  << " -> " << (ok?"PASS":"FAIL") << "\n";
        if (h.hit) {
            std::cout << "  t=" << h.t
                      << "  point=(" << h.point.x << "," << h.point.y << "," << h.point.z << ")\n";
            Vec3 rm = reflect(t.ray.direction, h.normal);
            std::cout << "  reflected_dir=(" << rm.x << "," << rm.y << "," << rm.z << ")\n";
        }
        std::cout << "\n";
    }
    std::cout << "Summary: " << pass << "/" << tests.size() << " passed\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Build plane from 3 points: N = normalize(cross(B-A, C-A))\n";
    std::cout << "  [ ] One-sided plane (backface cull): only hit if denom < 0\n";
    std::cout << "  [ ] Reflect ray off plane: R = D - N * 2 * dot(D, N)\n";
    std::cout << "  [ ] Infinite plane vs finite quad: after t, check x/z within bounds\n";
}

int main() {
    printTheory();
    runTests();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: How do you turn infinite plane intersection into finite quad intersection?
A key points:
- First solve ray-plane t, then compute hit point P.
- Build local basis on plane (U,V) and project P to local coordinates.
- Check projected coordinates against quad half extents (or barycentric for two triangles).
*/
