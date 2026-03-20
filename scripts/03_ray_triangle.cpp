// ================================================================
// 03_ray_triangle.cpp  -- Ray vs Triangle (Möller–Trumbore)
// ================================================================
// Key idea: express hit point in barycentric coordinates
//   P = (1-u-v)*V0 + u*V1 + v*V2
//   Point is inside triangle if: u>=0, v>=0, u+v<=1
//
// Substituting ray P(t) = O + tD:
//   O + tD = V0 + u*E1 + v*E2    (E1=V1-V0, E2=V2-V0)
//   O - V0 = -t*D + u*E1 + v*E2  let S = O-V0
//
// Solve using Cramer's rule (cross product formulation):
//   h = cross(D, E2)
//   a = dot(E1, h)           <- if ~0: ray parallel to triangle
//   f = 1.0 / a
//   u = f * dot(S, h)
//   q = cross(S, E1)
//   v = f * dot(D, q)
//   t = f * dot(E2, q)
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
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
static double len(const Vec3& a) { return std::sqrt(dot(a,a)); }
static Vec3 normalize(const Vec3& a) { double l=len(a); return {a.x/l,a.y/l,a.z/l}; }

struct Ray      { Vec3 origin, direction; };
struct Triangle { Vec3 v0, v1, v2; };

struct HitInfo {
    bool   hit = false;
    double t = -1, u = -1, v = -1;  // u,v barycentric, w=1-u-v
    Vec3   point{}, normal{};
};

// ---- Core intersection function (Möller–Trumbore) ----
HitInfo intersectRayTriangle(const Ray& ray, const Triangle& tri, bool cullBack = false) {
    const Vec3 E1 = tri.v1 - tri.v0;
    const Vec3 E2 = tri.v2 - tri.v0;
    const Vec3 h  = cross(ray.direction, E2);
    const double a = dot(E1, h);   // determinant

    if (cullBack && a < 1e-9) return {};   // backface culling
    if (std::abs(a) < 1e-9)   return {};   // ray parallel to triangle

    const double f  = 1.0 / a;
    const Vec3   s  = ray.origin - tri.v0;
    const double u  = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return {};

    const Vec3   q  = cross(s, E1);
    const double v  = f * dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0) return {};

    const double t = f * dot(E2, q);
    if (t < 1e-9) return {};   // behind or too close

    HitInfo r;
    r.hit    = true;
    r.t = t; r.u = u; r.v = v;
    r.point  = ray.origin + ray.direction * t;
    r.normal = normalize(cross(E1, E2));   // face normal
    return r;
}

// ---- Interpolate a value using barycentric coords ----
static Vec3 baryInterp(const Vec3& a, const Vec3& b, const Vec3& c, double u, double v) {
    double w = 1.0 - u - v;
    return {w*a.x + u*b.x + v*c.x,
            w*a.y + u*b.y + v*c.y,
            w*a.z + u*b.z + v*c.z};
}

// ---- Theory printout ----
static void printTheory() {
    std::cout << "================ Ray-Triangle (Möller–Trumbore) ================\n";
    std::cout << "Express hit point in barycentric coordinates:\n";
    std::cout << "  P = (1-u-v)*V0 + u*V1 + v*V2\n";
    std::cout << "  Inside triangle: u>=0, v>=0, u+v<=1\n\n";
    std::cout << "Setup: E1=V1-V0, E2=V2-V0, S=O-V0\n";
    std::cout << "  h  = cross(D, E2)\n";
    std::cout << "  a  = dot(E1, h)      <- determinant; if ~0: parallel\n";
    std::cout << "  f  = 1/a\n";
    std::cout << "  u  = f * dot(S, h)   <- 1st barycentric coord\n";
    std::cout << "  q  = cross(S, E1)\n";
    std::cout << "  v  = f * dot(D, q)   <- 2nd barycentric coord\n";
    std::cout << "  t  = f * dot(E2, q)  <- ray distance\n";
    std::cout << "=================================================================\n\n";
}

struct TestCase { std::string name, note; Ray ray; Triangle tri; bool expectedHit; };

static void runTests() {
    // Triangle in the z=0 plane
    Triangle tri{{-1,0,0}, {1,0,0}, {0,1,0}};

    // Vertex colors for barycentric interpolation demo
    Vec3 col0{1,0,0}, col1{0,1,0}, col2{0,0,1};

    std::vector<TestCase> tests = {
        {"Case A: center hit",
         "Ray shoots at centroid (0, 0.33, 0)",
         {{0.0, 0.33, -3.0},{0,0,1}}, tri, true},

        {"Case B: miss (outside edge)",
         "Ray hits z=0 plane but u+v>1, outside triangle",
         {{2.0, 0.5, -3.0},{0,0,1}}, tri, false},

        {"Case C: backface (no culling)",
         "Ray hits back side of triangle",
         {{0.0, 0.33, 3.0},{0,0,-1}}, tri, true},

        {"Case D: parallel ray",
         "Ray parallel to triangle plane -> miss",
         {{0.0, 0.33, -3.0},{1,0,0}}, tri, false},

        {"Case E: edge case",
         "Ray hits exactly at V0 vertex",
         {{-1.0, 0.0, -3.0},{0,0,1}}, tri, true},
    };

    std::cout << std::fixed << std::setprecision(4);
    int pass = 0;
    for (auto& t : tests) {
        auto h = intersectRayTriangle(t.ray, t.tri);
        bool ok = (h.hit == t.expectedHit);
        if (ok) ++pass;
        std::cout << t.name << "\n  " << t.note << "\n";
        std::cout << "  Expected:" << (t.expectedHit?"Hit":"Miss")
                  << "  Actual:" << (h.hit?"Hit":"Miss")
                  << " -> " << (ok?"PASS":"FAIL") << "\n";
        if (h.hit) {
            double w = 1.0 - h.u - h.v;
            std::cout << "  t=" << h.t << "  u=" << h.u << "  v=" << h.v << "  w=" << w << "\n";
            std::cout << "  point=(" << h.point.x << "," << h.point.y << "," << h.point.z << ")\n";
            Vec3 color = baryInterp(col0, col1, col2, h.u, h.v);
            std::cout << "  interpolated_color=(" << color.x << "," << color.y << "," << color.z << ")\n";
        }
        std::cout << "\n";
    }
    std::cout << "Summary: " << pass << "/" << tests.size() << " passed\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Backface culling: add 'if (a < eps) return miss;'\n";
    std::cout << "  [ ] Use u,v to interpolate UV/normals/colors\n";
    std::cout << "  [ ] Surface normal = normalize(cross(E1, E2)); flip if backface\n";
    std::cout << "  [ ] Shadow ray: t_min = small epsilon (1e-4) to avoid self-intersection\n";
    std::cout << "  [ ] w = 1-u-v is the weight of V0 (vertex opposite to u and v)\n";
}

int main() {
    printTheory();
    runTests();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why is Moller-Trumbore still widely used in ray tracing kernels?
A key points:
- It avoids precomputing plane equations and uses only vector ops.
- It directly returns barycentric u,v for interpolation.
- It is branch-light and efficient for SIMD/SIMT implementations.
*/
