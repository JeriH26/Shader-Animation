// ================================================================
// 12_cross_product.cpp  -- Cross Product & Its Uses in Graphics
// ================================================================
// Cross product of two 3D vectors A and B:
//   A x B = (ay*bz - az*by,  az*bx - ax*bz,  ax*by - ay*bx)
//
// Key properties:
//   1. The result is PERPENDICULAR to both A and B.
//   2. |A x B| = |A| * |B| * sin(theta)  (area of parallelogram spanned by A,B)
//   3. Direction follows right-hand rule.
//   4. A x B = -(B x A)  (anti-commutative)
//
// Uses in 3D graphics:
//   1. Surface normal:  N = normalize(cross(E1, E2))  where E1,E2 are triangle edges
//   2. Triangle area:   area = |cross(E1, E2)| * 0.5
//   3. Winding order:   sign of (cross(B-A, C-A)).z  tells CW vs CCW in XY plane
//   4. Tangent space:   build T, B, N basis for normal mapping
//   5. Camera right:    right = normalize(cross(forward, up))
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
    Vec3 operator-(const Vec3& r) const { return {x-r.x, y-r.y, z-r.z}; }
    Vec3 operator*(double s)      const { return {x*s, y*s, z*s}; }
};

static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x};
}
static double len(const Vec3& a) { return std::sqrt(dot(a,a)); }
static Vec3 normalize(const Vec3& a) { double l=len(a); return {a.x/l,a.y/l,a.z/l}; }

static void printVec(const char* label, const Vec3& v) {
    std::cout << "  " << std::setw(28) << std::left << label
              << " = (" << v.x << ", " << v.y << ", " << v.z << ")\n";
}

static void printTheory() {
    std::cout << "================ Cross Product in Graphics ================\n";
    std::cout << "A x B = (ay*bz-az*by,  az*bx-ax*bz,  ax*by-ay*bx)\n\n";
    std::cout << "Properties:\n";
    std::cout << "  perpendicular: dot(A x B, A) = 0, dot(A x B, B) = 0\n";
    std::cout << "  magnitude:     |A x B| = |A||B|sin(theta)\n";
    std::cout << "  direction:     right-hand rule (curl A toward B, thumb = A x B)\n";
    std::cout << "  anti-comm:     A x B = -(B x A)\n\n";
    std::cout << "Uses:\n";
    std::cout << "  1. Triangle normal:  N = normalize(cross(V1-V0, V2-V0))\n";
    std::cout << "  2. Triangle area:    area = |cross(E1,E2)| * 0.5\n";
    std::cout << "  3. Winding order:    Z-component of cross(B-A, C-A)\n";
    std::cout << "  4. Camera axes:      right = normalize(cross(forward, up))\n";
    std::cout << "  5. Tangent basis:    for normal mapping (TBN matrix)\n";
    std::cout << "===========================================================\n\n";
}

int main() {
    printTheory();
    std::cout << std::fixed << std::setprecision(4);

    // ---- 1. Basic cross product properties ----
    std::cout << "1. Basic properties:\n";
    Vec3 A = {1, 0, 0};  // X axis
    Vec3 B = {0, 1, 0};  // Y axis
    Vec3 AxB = cross(A, B);
    printVec("A", A); printVec("B", B); printVec("A x B", AxB);
    std::cout << "  dot(AxB, A) = " << dot(AxB, A) << "  (should be 0)\n";
    std::cout << "  dot(AxB, B) = " << dot(AxB, B) << "  (should be 0)\n";
    std::cout << "  |A x B| = " << len(AxB) << "  (= |A||B|sin90° = 1)\n\n";

    // ---- 2. Surface normal from triangle ----
    std::cout << "2. Triangle surface normal:\n";
    Vec3 V0 = {0, 0, 0};
    Vec3 V1 = {2, 0, 0};
    Vec3 V2 = {0, 2, 0};
    Vec3 E1 = V1 - V0;
    Vec3 E2 = V2 - V0;
    Vec3 N  = normalize(cross(E1, E2));
    printVec("V0", V0); printVec("V1", V1); printVec("V2", V2);
    printVec("E1=V1-V0", E1); printVec("E2=V2-V0", E2);
    printVec("N = normalize(E1 x E2)", N);
    std::cout << "  N points UP (+Z) for CCW triangle in XY plane\n\n";

    // ---- 3. Triangle area ----
    std::cout << "3. Triangle area:\n";
    double area = len(cross(E1, E2)) * 0.5;
    std::cout << "  |E1 x E2| * 0.5 = " << area << "\n";
    std::cout << "  Expected (2x2 right triangle): 0.5 * base * height = " << 0.5*2*2 << "\n\n";

    // ---- 4. Winding order ----
    std::cout << "4. Winding order (CCW = front face in OpenGL default):\n";
    auto windingStr = [](double z) -> const char* {
        return z > 0 ? "CCW (front, normal toward +Z)" :
               z < 0 ? "CW  (back,  normal toward -Z)" : "collinear";
    };
    Vec3 A_ccw = {0,0,0}, B_ccw = {1,0,0}, C_ccw = {0,1,0};
    Vec3 A_cw  = {0,0,0}, B_cw  = {0,1,0}, C_cw  = {1,0,0};
    double z_ccw = cross(B_ccw-A_ccw, C_ccw-A_ccw).z;
    double z_cw  = cross(B_cw - A_cw,  C_cw - A_cw).z;
    std::cout << "  CCW triangle (A,B,C = V0->V1->V2): cross.z = " << z_ccw
              << " -> " << windingStr(z_ccw) << "\n";
    std::cout << "  CW  triangle (A,B,C flipped):      cross.z = " << z_cw
              << " -> " << windingStr(z_cw)  << "\n\n";

    // ---- 5. Camera right vector ----
    std::cout << "5. Camera right vector (lookAt component):\n";
    Vec3 forward = normalize({0, 0, -1});   // camera looks -Z
    Vec3 up      = {0, 1, 0};
    Vec3 right   = normalize(cross(forward, up));
    printVec("forward", forward); printVec("up", up); printVec("right", right);
    std::cout << "  This is the first column of the view matrix\n\n";

    // ---- 6. TBN matrix (intro to normal mapping) ----
    std::cout << "6. Tangent / Bitangent / Normal (TBN) matrix:\n";
    Vec3 T = {1, 0, 0};    // tangent along U axis
    Vec3 Bn = normalize(cross(N, T));  // bitangent
    std::cout << "  T  = (1,0,0)  -> U texture direction\n";
    printVec("  B (N x T)", Bn);
    printVec("  N", N);
    std::cout << "  TBN matrix transforms normal-map sample from tangent space to world space\n\n";

    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] A x A = (0,0,0) -- parallel vectors have zero cross product\n";
    std::cout << "  [ ] Area of screen-space triangle: Shoelace formula uses 2D cross\n";
    std::cout << "  [ ] Normal map: sample (dx,dy,dz), transform with TBN to world N\n";
    std::cout << "  [ ] Handedness: left-hand vs right-hand coord systems flip cross sign\n";
    return 0;
}

/*
Interview Follow-up Q&A:
Q: How do you determine triangle winding direction using cross product?
A key points:
- Compute n = cross(v1-v0, v2-v0).
- The sign of n.z (in view/screen-aligned setup) indicates clockwise vs counterclockwise.
- This is the basis of backface culling with consistent coordinate conventions.
*/
