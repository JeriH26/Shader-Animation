// ================================================================
// 04_ray_aabb.cpp  -- Ray vs AABB (Slab Method)
// ================================================================
// AABB (Axis-Aligned Bounding Box) is the intersection of 3 pairs
// of infinite parallel planes ("slabs"), each perpendicular to an axis.
//
// For each axis i, the ray enters slab i at t_near_i and exits at t_far_i:
//   t_near_i = (min_i - O_i) / D_i
//   t_far_i  = (max_i - O_i) / D_i
//   (swap if t_near > t_far, because D_i could be negative)
//
// Overall:
//   tEnter = max(t_near_x, t_near_y, t_near_z)   <- enter when inside ALL slabs
//   tExit  = min(t_far_x,  t_far_y,  t_far_z)     <- exit when outside ANY slab
//
// Hit condition:  tEnter <= tExit  AND  tExit >= 0
// ================================================================

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
    Vec3 operator*(double s)      const { return {x*s,   y*s,   z*s  }; }
    double operator[](int i) const { return i==0?x : i==1?y : z; }
};

struct Ray  { Vec3 origin, direction; };
struct AABB { Vec3 mn, mx; };   // min and max corners

struct HitInfo {
    bool   hit   = false;
    double tNear = -1, tFar = -1;
    int    hitAxis = -1;   // 0=X, 1=Y, 2=Z (the axis that determined tEnter)
    Vec3   point{};
};

// ---- Core slab test ----
HitInfo intersectRayAABB(const Ray& ray, const AABB& box) {
    double tEnter = -1e30;
    double tExit  =  1e30;
    int    enterAxis = -1;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(ray.direction[i]) < 1e-9) {
            // Ray parallel to this slab: origin must be inside
            if (ray.origin[i] < box.mn[i] || ray.origin[i] > box.mx[i]) return {};
            continue;
        }
        double t1 = (box.mn[i] - ray.origin[i]) / ray.direction[i];
        double t2 = (box.mx[i] - ray.origin[i]) / ray.direction[i];
        if (t1 > t2) std::swap(t1, t2);
        if (t1 > tEnter) { tEnter = t1; enterAxis = i; }
        tExit = std::min(tExit, t2);
        if (tEnter > tExit) return {};   // slabs don't overlap
    }

    if (tExit < 0.0) return {};   // box entirely behind ray

    HitInfo r;
    r.hit      = true;
    r.tNear    = (tEnter >= 0.0) ? tEnter : 0.0;   // 0 if origin is inside
    r.tFar     = tExit;
    r.hitAxis  = enterAxis;
    r.point    = ray.origin + ray.direction * r.tNear;
    return r;
}

// ---- Expand AABB to include a point ----
static AABB expand(const AABB& box, const Vec3& p) {
    return {{std::min(box.mn.x,p.x), std::min(box.mn.y,p.y), std::min(box.mn.z,p.z)},
            {std::max(box.mx.x,p.x), std::max(box.mx.y,p.y), std::max(box.mx.z,p.z)}};
}

// ---- Theory printout ----
static void printTheory() {
    std::cout << "================ Ray-AABB (Slab Method) ================\n";
    std::cout << "AABB = 3 axis-aligned slab pairs (6 planes total)\n\n";
    std::cout << "For each axis i:\n";
    std::cout << "  t1 = (min_i - O_i) / D_i\n";
    std::cout << "  t2 = (max_i - O_i) / D_i\n";
    std::cout << "  sort -> [tNear_i, tFar_i]\n\n";
    std::cout << "Combined:\n";
    std::cout << "  tEnter = max(tNear_x, tNear_y, tNear_z)  <- inside all slabs\n";
    std::cout << "  tExit  = min(tFar_x,  tFar_y,  tFar_z)   <- outside any slab\n\n";
    std::cout << "Hit if: tEnter <= tExit  AND  tExit >= 0\n";
    std::cout << "=========================================================\n\n";
}

struct TestCase { std::string name, note; Ray ray; AABB box; bool expectedHit; };

static void runTests() {
    AABB box{{-1,-1,-1},{1,1,1}};

    std::vector<TestCase> tests = {
        {"Case A: direct hit",
         "Ray along +Z through center of box",
         {{0,0,-5},{0,0,1}}, box, true},

        {"Case B: miss",
         "Ray passes beside box (x=3)",
         {{3,0,-5},{0,0,1}}, box, false},

        {"Case C: origin inside",
         "Ray starts inside box -> tEnter<0, still a hit",
         {{0,0,0},{0,0,1}}, box, true},

        {"Case D: diagonal hit",
         "Ray along (+1,+1,+1) direction from far corner",
         {{-3,-3,-3},{1,1,1}}, box, true},

        {"Case E: box behind ray",
         "Box is at negative Z but ray shoots +Z from positive Z",
         {{0,0,5},{0,0,1}}, box, false},
    };

    std::cout << std::fixed << std::setprecision(4);
    int pass = 0;
    for (auto& t : tests) {
        auto h = intersectRayAABB(t.ray, t.box);
        bool ok = (h.hit == t.expectedHit);
        if (ok) ++pass;
        std::cout << t.name << "\n  " << t.note << "\n";
        std::cout << "  Expected:" << (t.expectedHit?"Hit":"Miss")
                  << "  Actual:" << (h.hit?"Hit":"Miss")
                  << " -> " << (ok?"PASS":"FAIL") << "\n";
        if (h.hit) {
            const char* axName[] = {"X","Y","Z","none"};
            std::cout << "  tNear=" << h.tNear << "  tFar=" << h.tFar
                      << "  enterFace=" << axName[h.hitAxis==-1?3:h.hitAxis] << "\n";
            std::cout << "  nearPoint=(" << h.point.x << "," << h.point.y << "," << h.point.z << ")\n";
        }
        std::cout << "\n";
    }
    std::cout << "Summary: " << pass << "/" << tests.size() << " passed\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Store invDir = 1/D per ray, reuse across many AABB tests (BVH)\n";
    std::cout << "  [ ] Build AABB from triangle: expand with each vertex\n";
    std::cout << "  [ ] Hit face normal: hitAxis gives which face; sign of D_hitAxis gives direction\n";
    std::cout << "  [ ] AABB is cheapest intersection test; use as first pass before triangle test\n";
}

int main() {
    printTheory();
    runTests();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: What optimization is commonly used for very frequent ray-AABB tests in BVH traversal?
A key points:
- Precompute invDir = 1 / dir once per ray.
- Precompute sign bits for each axis to avoid swaps.
- Use these cached values to reduce per-node arithmetic and branches.
*/
