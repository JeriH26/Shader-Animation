// ================================================================
// 19_bvh_simple.cpp  -- Bounding Volume Hierarchy (BVH)
// ================================================================
// BVH is a tree structure for accelerating ray-object intersection tests.
//
// Without BVH: ray must test against ALL N triangles -> O(N)
// With BVH:    ray skips entire subtrees that miss the bounding box -> O(log N)
//
// Structure:
//   Each node has:
//   - An AABB that bounds all primitives in its subtree
//   - Left and right children (or a list of triangles if leaf)
//
// Build (simple median split):
//   1. Compute AABB of all primitive centroids
//   2. Find longest axis of the centroid AABB
//   3. Sort primitives by centroid along that axis
//   4. Split at median: left half -> left child, right half -> right child
//   5. Recurse until leaf threshold (e.g., <= 2 triangles per leaf)
//
// Traverse:
//   1. Test ray against node's AABB
//   2. If miss: return (skip entire subtree)
//   3. If leaf: test all triangles in the leaf
//   4. If interior: recurse left and right children
//      (optionally: test closer child first for early termination)
//
// Better build strategies:
//   - SAH (Surface Area Heuristic): minimize expected traversal cost
//   - HLBVH: Morton code sort for GPU-friendly parallel build
// ================================================================

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

struct Vec3 {
    float x=0,y=0,z=0;
    Vec3 operator+(Vec3 r) const { return {x+r.x,y+r.y,z+r.z}; }
    Vec3 operator-(Vec3 r) const { return {x-r.x,y-r.y,z-r.z}; }
    Vec3 operator*(float s) const { return {x*s,y*s,z*s}; }
    float& operator[](int i) { return i==0?x:(i==1?y:z); }
    float  operator[](int i) const { return i==0?x:(i==1?y:z); }
};

static float dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
static Vec3 vmin(Vec3 a, Vec3 b) { return {std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z)}; }
static Vec3 vmax(Vec3 a, Vec3 b) { return {std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)}; }

// ---- AABB ----
struct AABB {
    Vec3 mn = { 1e30f,  1e30f,  1e30f};
    Vec3 mx = {-1e30f, -1e30f, -1e30f};

    void expand(Vec3 p) { mn = vmin(mn,p); mx = vmax(mx,p); }
    void expand(const AABB& o) { mn = vmin(mn,o.mn); mx = vmax(mx,o.mx); }
    Vec3 centroid() const { return (mn + mx) * 0.5f; }
    int longestAxis() const {
        Vec3 d = mx - mn;
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }

    bool rayIntersect(Vec3 orig, Vec3 dir, float& tNear, float& tFar) const {
        tNear = -1e30f; tFar = 1e30f;
        for (int i = 0; i < 3; ++i) {
            float invD = (std::abs(dir[i]) > 1e-8f) ? (1.0f / dir[i]) : 1e30f;
            float t1 = (mn[i] - orig[i]) * invD;
            float t2 = (mx[i] - orig[i]) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tNear = std::max(tNear, t1);
            tFar  = std::min(tFar,  t2);
            if (tNear > tFar) return false;
        }
        return tFar >= 0.0f;
    }
};

// ---- Triangle ----
struct Triangle {
    Vec3 v[3];
    int id;

    Vec3 centroid() const { return (v[0]+v[1]+v[2]) * (1.0f/3.0f); }
    AABB bounds() const { AABB b; for (auto& p:v) b.expand(p); return b; }

    // Möller–Trumbore
    bool intersect(Vec3 orig, Vec3 dir, float& t_out) const {
        Vec3 e1 = v[1]-v[0], e2 = v[2]-v[0];
        Vec3 h  = cross(dir, e2);
        float a = dot(e1, h);
        if (std::abs(a) < 1e-8f) return false;
        float f = 1.0f / a;
        Vec3  s = orig - v[0];
        float u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f) return false;
        Vec3  q = cross(s, e1);
        float v2= f * dot(dir, q);
        if (v2 < 0.0f || u+v2 > 1.0f) return false;
        float t = f * dot(e2, q);
        if (t < 1e-6f) return false;
        t_out = t;
        return true;
    }
};

// ---- BVH Node ----
struct BVHNode {
    AABB bounds;
    int leftChild = -1, rightChild = -1;  // index into node array
    std::vector<int> triIndices;           // only for leaf nodes
    bool isLeaf() const { return leftChild == -1; }
};

// ---- BVH ----
struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<Triangle> tris;
    int triTestCount = 0;
    int nodeTestCount = 0;

    void build(std::vector<Triangle> triangles) {
        tris = std::move(triangles);
        std::vector<int> indices(tris.size());
        std::iota(indices.begin(), indices.end(), 0);
        nodes.reserve(tris.size() * 2);
        buildRecursive(indices, 0, (int)indices.size());
    }

    int buildRecursive(std::vector<int>& indices, int start, int end) {
        int nodeIdx = (int)nodes.size();
        nodes.push_back({});
        BVHNode& node = nodes.back();

        // Compute AABB for all triangles in this range
        for (int i = start; i < end; ++i)
            node.bounds.expand(tris[indices[i]].bounds());

        int count = end - start;
        // Leaf if <= 2 triangles
        if (count <= 2) {
            for (int i = start; i < end; ++i)
                node.triIndices.push_back(indices[i]);
            return nodeIdx;
        }

        // Find split axis: longest axis of centroid AABB
        AABB centroidBounds;
        for (int i = start; i < end; ++i)
            centroidBounds.expand(tris[indices[i]].centroid());
        int axis = centroidBounds.longestAxis();

        // Sort by centroid along chosen axis, split at median
        std::sort(indices.begin()+start, indices.begin()+end,
            [&](int a, int b) {
                return tris[a].centroid()[axis] < tris[b].centroid()[axis];
            });
        int mid = start + count / 2;

        // Need to get nodeIdx after recursion (nodes may realloc)
        int leftIdx  = buildRecursive(indices, start, mid);
        int rightIdx = buildRecursive(indices, mid, end);

        nodes[nodeIdx].leftChild  = leftIdx;
        nodes[nodeIdx].rightChild = rightIdx;
        return nodeIdx;
    }

    // Traverse BVH: find closest intersection
    bool intersect(Vec3 orig, Vec3 dir, float& tHit, int& hitTri) {
        triTestCount = nodeTestCount = 0;
        tHit = 1e30f; hitTri = -1;
        intersectNode(0, orig, dir, tHit, hitTri);
        return hitTri >= 0;
    }

    void intersectNode(int nodeIdx, Vec3 orig, Vec3 dir, float& tHit, int& hitTri) {
        ++nodeTestCount;
        const BVHNode& node = nodes[nodeIdx];
        float tN, tF;
        if (!node.bounds.rayIntersect(orig, dir, tN, tF)) return;
        if (tN > tHit) return;  // this subtree can't improve current best

        if (node.isLeaf()) {
            for (int ti : node.triIndices) {
                ++triTestCount;
                float t;
                if (tris[ti].intersect(orig, dir, t) && t < tHit) {
                    tHit = t; hitTri = ti;
                }
            }
        } else {
            intersectNode(node.leftChild,  orig, dir, tHit, hitTri);
            intersectNode(node.rightChild, orig, dir, tHit, hitTri);
        }
    }
};

static void printTheory() {
    std::cout << "================ BVH (Bounding Volume Hierarchy) ================\n";
    std::cout << "Goal: speed up ray-scene intersection from O(N) to O(log N)\n\n";
    std::cout << "Tree structure:\n";
    std::cout << "  Each node: AABB + left/right children\n";
    std::cout << "  Leaf node: contains a small list of triangles (e.g., ≤2)\n\n";
    std::cout << "Build (median split):\n";
    std::cout << "  1. Compute AABB of all primitive centroids\n";
    std::cout << "  2. Split along longest axis at median\n";
    std::cout << "  3. Recurse until leaf size reached\n\n";
    std::cout << "Traverse:\n";
    std::cout << "  1. Test ray vs node AABB\n";
    std::cout << "  2. Miss -> skip entire subtree\n";
    std::cout << "  3. Hit + leaf -> test each triangle\n";
    std::cout << "  4. Hit + interior -> recurse both children\n";
    std::cout << "==================================================================\n\n";
}

static std::vector<Triangle> makeTestScene() {
    // 12 triangles: a simple box (2 triangles per face x 6 faces)
    std::vector<Triangle> tris;
    int id = 0;
    // Floor (y=0)
    tris.push_back({{{-1.f,0.f,-1.f},{1.f,0.f,-1.f},{1.f,0.f,1.f}}, id++});
    tris.push_back({{{-1.f,0.f,-1.f},{1.f,0.f, 1.f},{-1.f,0.f,1.f}}, id++});
    // Ceiling (y=2)
    tris.push_back({{{-1.f,2.f,-1.f},{1.f,2.f,1.f},{1.f,2.f,-1.f}}, id++});
    tris.push_back({{{-1.f,2.f,-1.f},{-1.f,2.f,1.f},{1.f,2.f,1.f}}, id++});
    // Back wall (z=-1)
    tris.push_back({{{-1.f,0.f,-1.f},{-1.f,2.f,-1.f},{1.f,2.f,-1.f}}, id++});
    tris.push_back({{{-1.f,0.f,-1.f},{1.f,2.f,-1.f},{1.f,0.f,-1.f}}, id++});
    // Left wall
    tris.push_back({{{-1.f,0.f,-1.f},{-1.f,0.f,1.f},{-1.f,2.f,1.f}}, id++});
    tris.push_back({{{-1.f,0.f,-1.f},{-1.f,2.f,1.f},{-1.f,2.f,-1.f}}, id++});
    // Right wall
    tris.push_back({{{1.f,0.f,-1.f},{1.f,2.f,-1.f},{1.f,2.f,1.f}}, id++});
    tris.push_back({{{1.f,0.f,-1.f},{1.f,2.f, 1.f},{1.f,0.f,1.f}}, id++});
    // Small triangle in the middle
    tris.push_back({{{-.2f,.9f,0.f},{.2f,.9f,0.f},{0.f,1.2f,0.f}}, id++});
    tris.push_back({{{-.1f,.8f,0.f},{.1f,.8f,0.f},{0.f,1.1f,0.f}}, id++});
    return tris;
}

int main() {
    printTheory();

    auto scene = makeTestScene();
    std::cout << "Test scene: " << scene.size() << " triangles (box + 2 interior tris)\n\n";

    BVH bvh;
    bvh.build(scene);

    std::cout << "BVH built: " << bvh.nodes.size() << " nodes\n";
    int leafCount = 0;
    for (auto& n : bvh.nodes) if (n.isLeaf()) ++leafCount;
    std::cout << "  Leaf nodes: " << leafCount << "\n\n";

    // Test several rays
    struct RayTest { std::string name; Vec3 orig, dir; };
    std::vector<RayTest> rays = {
        {"Ray through center",     {0,1,-5},{0,0,1}},
        {"Ray through small tri",  {0,1,-5},{0.01f,0.05f,1.0f}},
        {"Ray that misses scene",  {5,5,-5},{0,0,1}},
    };

    for (auto& rt : rays) {
        float tHit; int triIdx;
        bool hit = bvh.intersect(rt.orig, rt.dir, tHit, triIdx);
        std::cout << rt.name << ":\n";
        std::cout << "  Hit=" << (hit?"YES":"NO");
        if (hit) std::cout << " t=" << tHit << " tri=" << triIdx;
        std::cout << "\n  BVH node tests=" << bvh.nodeTestCount
                  << " tri tests=" << bvh.triTestCount
                  << " (brute force would be " << scene.size() << ")\n\n";
    }

    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] SAH (Surface Area Heuristic): better split point than median\n";
    std::cout << "  [ ] LBVH: sort by Morton code for GPU-friendly parallel build\n";
    std::cout << "  [ ] BVH refitting: update AABBs after animation (cheaper than rebuild)\n";
    std::cout << "  [ ] BVH for collision detection: store OBBs instead of AABBs\n";
    std::cout << "  [ ] Two-level BVH: TLAS (top-level) + BLAS (bottom-level) in DXR/Vulkan RT\n";

    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why does SAH-built BVH usually outperform median-split BVH for ray tracing?
A key points:
- SAH minimizes expected traversal cost using node surface areas and primitive counts.
- It tends to reduce overlap and traversal divergence.
- Build is slower, but runtime savings are substantial for many rays.
*/
