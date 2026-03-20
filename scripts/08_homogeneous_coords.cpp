// ================================================================
// 08_homogeneous_coords.cpp  -- Homogeneous Coordinates & w
// ================================================================
// 3D graphics uses 4D (homogeneous) coordinates: (x, y, z, w).
//
//  w = 1  ->  a POINT     - translation IS applied
//  w = 0  ->  a DIRECTION - translation is NOT applied
//
// Why? The translation column of a 4x4 matrix only affects the result
// when w != 0.  Directions (vectors) should not be translated.
//
// Perspective divide: after clip space, divide by w to get NDC:
//   ndc = (x/w, y/w, z/w)
//   This performs the perspective projection (far objects become small).
//   clip.w = -view_z (set by the projection matrix).
//
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec4 { double x=0, y=0, z=0, w=1; };

struct Mat4 {
    double m[4][4] = {};
    static Mat4 identity() {
        Mat4 r; r.m[0][0]=r.m[1][1]=r.m[2][2]=r.m[3][3]=1.0; return r;
    }
    Vec4 operator*(const Vec4& v) const {
        return {m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z+m[3][0]*v.w,
                m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z+m[3][1]*v.w,
                m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z+m[3][2]*v.w,
                m[0][3]*v.x+m[1][3]*v.y+m[2][3]*v.z+m[3][3]*v.w};
    }
};

static Mat4 translate(double tx, double ty, double tz) {
    Mat4 r = Mat4::identity();
    r.m[3][0]=tx; r.m[3][1]=ty; r.m[3][2]=tz;
    return r;
}

static void printVec4(const char* label, const Vec4& v) {
    std::cout << "  " << std::setw(30) << std::left << label
              << " = (" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")\n";
}

static void printTheory() {
    std::cout << "================ Homogeneous Coordinates ================\n";
    std::cout << "4D (x, y, z, w) where:\n";
    std::cout << "  w = 1  ->  POINT     (translation applies)\n";
    std::cout << "  w = 0  ->  DIRECTION (translation does NOT apply)\n\n";
    std::cout << "Why w=0 means direction:\n";
    std::cout << "  Translation in 4x4 matrix is in column 3 (m[3][row])\n";
    std::cout << "  result += m[3][row] * v.w\n";
    std::cout << "  When w=0: translation contribution = 0  -> preserved direction\n\n";
    std::cout << "Perspective divide (clip -> NDC):\n";
    std::cout << "  ndc.xyz = clip.xyz / clip.w\n";
    std::cout << "  Projection matrix sets clip.w = -view_z\n";
    std::cout << "  Far objects have large |view_z| -> small ndc.xy -> appear small\n";
    std::cout << "=========================================================\n\n";
}

int main() {
    printTheory();
    std::cout << std::fixed << std::setprecision(4);

    Mat4 T = translate(10, 20, 30);

    // --- Demonstrate w=1 (point) vs w=0 (direction) ---
    Vec4 point     = {1, 2, 3, 1};   // a point in space
    Vec4 direction = {1, 2, 3, 0};   // a direction vector (same xyz, but w=0)

    Vec4 tPoint  = T * point;
    Vec4 tDir    = T * direction;

    std::cout << "Translation by (10, 20, 30):\n";
    printVec4("point     (w=1) before", point);
    printVec4("point     (w=1) after ", tPoint);
    std::cout << "  -> Point moved by (+10,+20,+30) as expected\n\n";
    printVec4("direction (w=0) before", direction);
    printVec4("direction (w=0) after ", tDir);
    std::cout << "  -> Direction unchanged (w=0 blocks translation)\n\n";

    // --- Demonstrate perspective divide ---
    std::cout << "Perspective divide (view -> NDC):\n";
    std::cout << "  Objects at different depths in view space:\n\n";

    struct DepthCase { const char* label; double viewZ; };
    DepthCase depths[] = {
        {"Near object (z=-2)",  -2.0},
        {"Mid  object (z=-10)", -10.0},
        {"Far  object (z=-50)", -50.0},
    };

    double fov    = 60.0 * 3.14159/180.0;
    double aspect = 16.0/9.0;
    double near   = 0.1, far = 100.0;
    double f      = 1.0 / std::tan(fov * 0.5);

    for (auto& d : depths) {
        // Suppose the object is at ndc_x=0.5 (world space), and at different z
        // Build the clip position manually
        double viewX = 0.5 * (-d.viewZ);  // simulating perspective: x_view at half the z
        double clipX = (f / aspect) * viewX;
        double clipW = -d.viewZ;          // projection sets clip.w = -viewZ
        double ndcX  = clipX / clipW;     // perspective divide
        std::cout << "  " << d.label << ":  viewX=" << viewX
                  << "  clipX=" << clipX
                  << "  clipW=" << clipW
                  << "  ndcX=" << ndcX << "\n";
    }
    std::cout << "  -> The deeper object has a smaller ndcX (appears smaller)\n\n";

    // --- w != 0,1: homogeneous representation of the same point ---
    std::cout << "Homogeneous equivalence: (2,4,6,2) == (1,2,3,1)\n";
    Vec4 h1 = {2, 4, 6, 2};
    Vec4 h2 = {1, 2, 3, 1};
    std::cout << "  (2/2, 4/2, 6/2) = (" << h1.x/h1.w << "," << h1.y/h1.w << "," << h1.z/h1.w << ")\n";
    std::cout << "  (1/1, 2/1, 3/1) = (" << h2.x/h2.w << "," << h2.y/h2.w << "," << h2.z/h2.w << ")\n";
    std::cout << "  -> Same 3D point\n\n";

    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Pass normals as (nx,ny,nz,0) to avoid translation in shaders\n";
    std::cout << "  [ ] Clip space w holds depth for perspective: clip.w = -view_z\n";
    std::cout << "  [ ] NDC = clip.xyz / clip.w  (done automatically by GPU after vertex shader)\n";
    std::cout << "  [ ] w < 0 means point is behind camera (will be clipped)\n";
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why are directions stored with w=0 and points with w=1 in homogeneous coordinates?
A key points:
- Translation should affect points but not pure directions.
- With w=0, translation terms vanish in affine transform.
- This unifies transform math for points, vectors, and perspective projection.
*/
