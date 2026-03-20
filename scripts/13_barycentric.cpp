// ================================================================
// 13_barycentric.cpp  -- Barycentric Coordinates
// ================================================================
// Any point P inside (or on) triangle V0,V1,V2 can be written as:
//   P = alpha*V0 + beta*V1 + gamma*V2
//   where alpha + beta + gamma = 1
//
// If alpha, beta, gamma are ALL in [0, 1]: P is inside the triangle.
// If any is < 0 or > 1: P is outside.
//
// Geometric interpretation (area method):
//   alpha = area(P,  V1, V2) / area(V0, V1, V2)   <- weight of V0
//   beta  = area(V0, P,  V2) / area(V0, V1, V2)   <- weight of V1
//   gamma = area(V0, V1, P ) / area(V0, V1, V2)   <- weight of V2
//   (use the |cross product| / 2 formula for each area)
//
// Key uses in graphics:
//   1. Point-in-triangle test
//   2. Interpolate UV coordinates across a triangle face
//   3. Interpolate vertex colors, normals, any attribute
//   4. Used inside Möller–Trumbore (the u,v in that algorithm ARE beta, gamma)
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Vec2 { double x=0, y=0; };
struct Vec3 { double x=0, y=0, z=0; };

static double cross2D(const Vec2& a, const Vec2& b) { return a.x*b.y - a.y*b.x; }
static Vec2 sub2(const Vec2& a, const Vec2& b) { return {a.x-b.x, a.y-b.y}; }

struct BaryCoords { double a=0, b=0, c=0; bool inside=false; };

// ---- Compute barycentric coordinates using area method (2D) ----
BaryCoords barycentric(const Vec2& P, const Vec2& V0, const Vec2& V1, const Vec2& V2) {
    // Signed area of full triangle (using cross product)
    double areaFull = cross2D(sub2(V1,V0), sub2(V2,V0));
    if (std::abs(areaFull) < 1e-9) return {};  // degenerate triangle

    // alpha = area(P, V1, V2) / areaFull
    double alpha = cross2D(sub2(V1,P), sub2(V2,P)) / areaFull;
    double beta  = cross2D(sub2(V2,P), sub2(V0,P)) / areaFull;
    double gamma = 1.0 - alpha - beta;

    BaryCoords r;
    r.a = alpha; r.b = beta; r.c = gamma;
    r.inside = (alpha >= 0 && beta >= 0 && gamma >= 0);
    return r;
}

// ---- Interpolate a scalar value using barycentric weights ----
static double baryLerp(double v0, double v1, double v2, double a, double b, double c) {
    return a*v0 + b*v1 + c*v2;
}

// ---- Direct solve for barycentric (faster, no area normalizing) ----
// Solve system: P - V0 = beta*(V1-V0) + gamma*(V2-V0)
// 2 equations, 2 unknowns (beta, gamma) in 2D
BaryCoords barycentricSolve(const Vec2& P, const Vec2& V0, const Vec2& V1, const Vec2& V2) {
    Vec2 e1 = sub2(V1, V0);  // V1 - V0
    Vec2 e2 = sub2(V2, V0);  // V2 - V0
    Vec2 ep = sub2(P,  V0);  // P  - V0

    double denom = e1.x*e2.y - e1.y*e2.x;
    if (std::abs(denom) < 1e-9) return {};

    double beta  = (ep.x*e2.y - ep.y*e2.x) / denom;
    double gamma = (e1.x*ep.y - e1.y*ep.x) / denom;
    double alpha = 1.0 - beta - gamma;

    BaryCoords r;
    r.a = alpha; r.b = beta; r.c = gamma;
    r.inside = (alpha >= 0 && beta >= 0 && gamma >= 0);
    return r;
}

static void printTheory() {
    std::cout << "================ Barycentric Coordinates ================\n";
    std::cout << "P = alpha*V0 + beta*V1 + gamma*V2,  alpha+beta+gamma = 1\n\n";
    std::cout << "Inside test: all of alpha, beta, gamma must be in [0,1]\n\n";
    std::cout << "Area method (intuition):\n";
    std::cout << "  alpha = area(P, V1, V2) / area(V0, V1, V2)  <- P is 'close to V0'\n";
    std::cout << "  beta  = area(V0, P, V2) / area(V0, V1, V2)  <- P is 'close to V1'\n";
    std::cout << "  gamma = 1 - alpha - beta               <- P is 'close to V2'\n\n";
    std::cout << "At vertex V0: alpha=1, beta=0, gamma=0\n";
    std::cout << "At centroid:  alpha=beta=gamma=1/3\n\n";
    std::cout << "Interpolation: any attribute X = alpha*X0 + beta*X1 + gamma*X2\n";
    std::cout << "  UV coords, vertex colors, normals, anything\n";
    std::cout << "=========================================================\n\n";
}

struct TestCase { std::string name, note; Vec2 P, V0, V1, V2; bool expectedInside; };

static void runTests() {
    Vec2 V0={0,0}, V1={4,0}, V2={0,4};

    std::vector<TestCase> tests = {
        {"At V0 vertex",       "alpha=1, beta=0, gamma=0",  {0,0},   V0,V1,V2, true},
        {"At V1 vertex",       "alpha=0, beta=1, gamma=0",  {4,0},   V0,V1,V2, true},
        {"At centroid",        "alpha=beta=gamma=1/3",       {1.33,1.33}, V0,V1,V2, true},
        {"Outside (beyond V2)","gamma > 1",                  {0,5},   V0,V1,V2, false},
        {"Outside (beyond X)", "beta > 1",                   {5,0},   V0,V1,V2, false},
        {"On hypotenuse edge", "gamma=0, inside boundary",   {2,2},   V0,V1,V2, true},
        {"Outside corner",     "negative coords",             {-1,-1}, V0,V1,V2, false},
    };

    std::cout << std::fixed << std::setprecision(4);
    int pass = 0;
    for (auto& t : tests) {
        auto bc = barycentric(t.P, t.V0, t.V1, t.V2);
        bool ok = (bc.inside == t.expectedInside);
        if (ok) ++pass;
        std::cout << t.name << "\n  " << t.note << "\n";
        std::cout << "  alpha=" << bc.a << "  beta=" << bc.b << "  gamma=" << bc.c
                  << "  sum=" << (bc.a+bc.b+bc.c) << "\n";
        std::cout << "  Expected:" << (t.expectedInside?"Inside":"Outside")
                  << "  Actual:" << (bc.inside?"Inside":"Outside")
                  << " -> " << (ok?"PASS":"FAIL") << "\n\n";
    }
    std::cout << "Summary: " << pass << "/" << tests.size() << " passed\n\n";
}

static void runInterpolationDemo() {
    std::cout << "Attribute interpolation demo (UV coordinates):\n";
    Vec2 V0={0,0}, V1={4,0}, V2={0,4};
    // UV at each vertex
    Vec2 UV0={0,0}, UV1={1,0}, UV2={0,1};
    // Color at each vertex (RGB)
    Vec3 col0={1,0,0}, col1={0,1,0}, col2={0,0,1};

    Vec2 testPoints[] = {{2,0}, {0,2}, {1,1}};
    const char* ptNames[] = {"midpoint of edge V0-V1", "midpoint of edge V0-V2", "interior (1,1)"};

    for (int i = 0; i < 3; ++i) {
        auto bc = barycentric(testPoints[i], V0, V1, V2);
        double u = baryLerp(UV0.x, UV1.x, UV2.x, bc.a, bc.b, bc.c);
        double v = baryLerp(UV0.y, UV1.y, UV2.y, bc.a, bc.b, bc.c);
        double cr= baryLerp(col0.x, col1.x, col2.x, bc.a, bc.b, bc.c);
        double cg= baryLerp(col0.y, col1.y, col2.y, bc.a, bc.b, bc.c);
        double cb= baryLerp(col0.z, col1.z, col2.z, bc.a, bc.b, bc.c);
        std::cout << "  P at " << ptNames[i] << ":\n";
        std::cout << "    UV    = (" << u << ", " << v << ")\n";
        std::cout << "    color = (" << cr << ", " << cg << ", " << cb << ")\n";
    }
    std::cout << "\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] In Möller–Trumbore: u=beta, v=gamma, alpha=1-u-v\n";
    std::cout << "  [ ] Perspective-correct interpolation: divide by w before interp, mul after\n";
    std::cout << "  [ ] GPU does barycentric interpolation automatically for varying/out variables\n";
    std::cout << "  [ ] gl_FragCoord: GPU gives you the barycentric-interpolated fragment position\n";
}

int main() {
    printTheory();
    runTests();
    runInterpolationDemo();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why is perspective-correct interpolation needed instead of plain barycentric interpolation in screen space?
A key points:
- Screen-space barycentrics are linear after projection, but attributes in 3D are not.
- Interpolate attribute/w and 1/w first, then divide to recover correct value.
- Without this, UVs and specular highlights warp on triangles at depth variation.
*/
