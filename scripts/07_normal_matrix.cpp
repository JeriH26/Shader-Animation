// ================================================================
// 07_normal_matrix.cpp  -- Why normals need the Inverse-Transpose
// ================================================================
// Problem: applying a non-uniform scale to a surface BREAKS the normal.
//
// Example: surface in XZ plane, normal points up (0,1,0).
//          Apply scale(3,1,1) (stretch X by 3).
//   If we naively apply scale to normal: (0,1,0) -> (0,1,0)  ok so far.
//   But consider a diagonal normal on a tilted surface...
//
// Better example: triangle with vertices A(0,0,0), B(1,0,0), C(0,1,0).
//   Surface tangent T = B-A = (1,0,0).
//   Normal N = (0,0,1) (into screen, for 2D illustration use (0,1,0)).
//
//   The constraint is: N must be perpendicular to any tangent T on the surface.
//   After transform M: T' = M*T,  N' must satisfy: dot(N', T') = 0
//
//   If N' = M*N: dot(M*N, M*T) != 0 in general for non-uniform M.
//   Correct:  N' = (M^-1)^T * N  (inverse-transpose of M)
//
// For uniform scale or pure rotation: (M^-1)^T = M, so no difference.
// For non-uniform scale or shear: MUST use inverse-transpose.
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
};
static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static double len(const Vec3& a) { return std::sqrt(dot(a,a)); }
static Vec3 normalize(const Vec3& a) { double l=len(a); return {a.x/l, a.y/l, a.z/l}; }
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

// Simple 3x3 matrix for normal transformation
struct Mat3 {
    double m[3][3] = {};
    static Mat3 identity() { Mat3 r; r.m[0][0]=r.m[1][1]=r.m[2][2]=1; return r; }
    Vec3 operator*(const Vec3& v) const {
        return {m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z,
                m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z,
                m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z};
    }
};

// Build non-uniform scale matrix
static Mat3 scaleM(double sx, double sy, double sz) {
    Mat3 m = Mat3::identity();
    m.m[0][0]=sx; m.m[1][1]=sy; m.m[2][2]=sz;
    return m;
}

// Inverse of a diagonal (scale) matrix = reciprocal of each diagonal element
static Mat3 invertScale(const Mat3& m) {
    Mat3 r = Mat3::identity();
    r.m[0][0] = 1.0/m.m[0][0];
    r.m[1][1] = 1.0/m.m[1][1];
    r.m[2][2] = 1.0/m.m[2][2];
    return r;
}

// Transpose of a 3x3 matrix
static Mat3 transpose(const Mat3& m) {
    Mat3 r;
    for (int i=0;i<3;i++) for (int j=0;j<3;j++) r.m[i][j]=m.m[j][i];
    return r;
}

static void printTheory() {
    std::cout << "================ Normal Matrix (Inverse-Transpose) ================\n";
    std::cout << "Problem: non-uniform scale breaks normals.\n\n";
    std::cout << "Constraint: N must remain perpendicular to surface tangents T.\n";
    std::cout << "  Before transform: dot(N, T) = 0\n";
    std::cout << "  After naive transform M: dot(M*N, M*T) != 0  (wrong!)\n\n";
    std::cout << "Proof of correct formula:\n";
    std::cout << "  We need N' such that: dot(N', M*T) = 0 for all tangents T\n";
    std::cout << "  dot(N', M*T) = N'^T * M * T\n";
    std::cout << "  If N' = (M^-1)^T * N:\n";
    std::cout << "    N'^T * M = ((M^-1)^T * N)^T * M\n";
    std::cout << "             = N^T * (M^-1) * M\n";
    std::cout << "             = N^T * I = N^T\n";
    std::cout << "  So dot(N', M*T) = N^T * T = dot(N, T) = 0  ✓\n\n";
    std::cout << "Normal matrix = transpose(inverse(M3x3))\n";
    std::cout << "  - For uniform scale or pure rotation: M^-1 = M^T, so normal_mat = M\n";
    std::cout << "  - For non-uniform scale or shear: MUST use inverse-transpose\n";
    std::cout << "====================================================================\n\n";
}

int main() {
    printTheory();
    std::cout << std::fixed << std::setprecision(4);

    // Scenario: tilted surface with vertices
    //   V0 = (0,0,0), V1 = (1,0,0), V2 = (0,1,0)
    // Tangent T = V1 - V0 = (1,0,0)
    // Normal N from cross product: cross(T, V2-V0) = cross((1,0,0),(0,1,0)) = (0,0,1)
    Vec3 T = {1, 0, 0};
    Vec3 N = {0, 0, 1};

    std::cout << "Surface:\n";
    std::cout << "  Tangent T = (1,0,0)\n";
    std::cout << "  Normal  N = (0,0,1)\n";
    std::cout << "  dot(N,T) = " << dot(N,T) << "  (should be 0)\n\n";

    // Apply non-uniform scale: scale X by 3, Y by 1, Z by 1
    Mat3 M = scaleM(3, 1, 1);
    std::cout << "Applying non-uniform scale(3,1,1):\n\n";

    Vec3 Tprime  = M * T;
    Vec3 Nnaive  = M * N;                                    // Wrong: direct transform
    Vec3 NcorrRaw = transpose(invertScale(M)) * N;           // Correct: inv-transpose
    Vec3 Ncorr   = normalize(NcorrRaw);

    std::cout << "  T' = M * T = (" << Tprime.x << "," << Tprime.y << "," << Tprime.z << ")\n";
    std::cout << "  N_naive  (M * N)          = ("
              << Nnaive.x  << "," << Nnaive.y  << "," << Nnaive.z  << ")\n";
    std::cout << "  N_correct ((M^-1)^T * N)  = ("
              << NcorrRaw.x << "," << NcorrRaw.y << "," << NcorrRaw.z << ") -> normalized: ("
              << Ncorr.x   << "," << Ncorr.y   << "," << Ncorr.z   << ")\n\n";

    std::cout << "Perpendicularity check dot(N', T'):\n";
    std::cout << "  naive:   dot(N_naive, T')   = " << dot(Nnaive, Tprime)
              << "  (should be 0 -> " << (std::abs(dot(Nnaive, Tprime))<1e-9?"OK":"WRONG") << ")\n";
    std::cout << "  correct: dot(N_correct, T') = " << dot(Ncorr, Tprime)
              << "  (should be 0 -> " << (std::abs(dot(Ncorr, Tprime))<1e-9?"OK":"WRONG") << ")\n\n";

    // Also show for a diagonal surface where the difference is more obvious
    std::cout << "--- Diagonal surface (normal NOT axis-aligned) ---\n";
    Vec3 T2 = normalize({1, 1, 0});
    Vec3 N2 = normalize({-1, 1, 0});   // perpendicular to T2 in XY
    std::cout << "  Tangent T2 = (0.707, 0.707, 0)\n";
    std::cout << "  Normal  N2 = (-0.707, 0.707, 0)  dot=" << dot(N2,T2) << "\n\n";

    Vec3 T2prime    = M * T2;
    Vec3 N2naive    = normalize(M * N2);
    Vec3 N2correct  = normalize(transpose(invertScale(M)) * N2);
    std::cout << "  After scale(3,1,1):\n";
    std::cout << "  T2' = (" << T2prime.x << "," << T2prime.y << "," << T2prime.z << ")\n";
    std::cout << "  dot(N2_naive, T2')   = " << dot(N2naive, T2prime)
              << "  -> " << (std::abs(dot(N2naive, T2prime))<1e-9?"OK":"WRONG!  <-- broken normal") << "\n";
    std::cout << "  dot(N2_correct, T2') = " << dot(N2correct, T2prime)
              << "  -> " << (std::abs(dot(N2correct, T2prime))<1e-9?"OK":"WRONG") << "\n\n";

    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] In GLSL: normalMatrix = transpose(inverse(mat3(modelView)))\n";
    std::cout << "  [ ] For uniform scale or pure rotation: normalMatrix == model3x3\n";
    std::cout << "  [ ] After computing N', always normalize before lighting\n";
    std::cout << "  [ ] Only the 3x3 submatrix matters (translation doesn't affect vectors)\n";
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why is inverse-transpose needed for normals but not for positions?
A key points:
- Normals represent planes (covectors), not points.
- Under non-uniform scale, direct multiplication breaks orthogonality with tangent vectors.
- Inverse-transpose preserves the dot(normal, tangent) = 0 constraint.
*/
