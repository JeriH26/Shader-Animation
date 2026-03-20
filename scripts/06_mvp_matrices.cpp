// ================================================================
// 06_mvp_matrices.cpp  -- Model / View / Projection pipeline
// ================================================================
// Full vertex pipeline in OpenGL:
//   clip_pos = Projection * View * Model * local_pos
//   ndc_pos  = clip_pos.xyz / clip_pos.w       (perspective divide)
//   screen   = (ndc+1)*0.5 * viewport
//
// Model  matrix: local space -> world space  (translate, rotate, scale)
// View   matrix: world space -> camera space (camera at origin, -Z forward)
// Proj   matrix: camera space -> clip space  (maps frustum to NDC cube)
//
// All matrices are 4x4, column-major (OpenGL convention).
// m[col][row] = element in column `col`, row `row`.
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

static const double PI = 3.14159265358979;

struct Vec3 { double x=0, y=0, z=0; };
struct Vec4 { double x=0, y=0, z=0, w=1; };

// ---- 4x4 column-major matrix (m[col][row]) ----
struct Mat4 {
    double m[4][4] = {};

    static Mat4 identity() {
        Mat4 r;
        r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0;
        return r;
    }

    // Matrix * Vector
    Vec4 operator*(const Vec4& v) const {
        return {m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]*v.w,
                m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]*v.w,
                m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]*v.w,
                m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]*v.w};
    }

    // Matrix * Matrix  (C = A * B -> applied B first, then A)
    Mat4 operator*(const Mat4& rhs) const {
        Mat4 r;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    r.m[col][row] += m[k][row] * rhs.m[col][k];
        return r;
    }
};

// ---- Standard matrix builders ----
static Mat4 translate(double tx, double ty, double tz) {
    Mat4 r = Mat4::identity();
    r.m[3][0] = tx; r.m[3][1] = ty; r.m[3][2] = tz;
    return r;
}

static Mat4 scale(double sx, double sy, double sz) {
    Mat4 r = Mat4::identity();
    r.m[0][0] = sx; r.m[1][1] = sy; r.m[2][2] = sz;
    return r;
}

static Mat4 rotateY(double rad) {
    Mat4 r = Mat4::identity();
    double c = std::cos(rad), s = std::sin(rad);
    r.m[0][0] =  c;  r.m[2][0] = s;
    r.m[0][2] = -s;  r.m[2][2] = c;
    return r;
}

static double dot3(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vec3  cross3(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
static Vec3  norm3(Vec3 v) { double l=std::sqrt(dot3(v,v)); return {v.x/l,v.y/l,v.z/l}; }

// LookAt: build view matrix (camera at eye, looking toward center)
static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = norm3({center.x-eye.x, center.y-eye.y, center.z-eye.z});
    Vec3 r = norm3(cross3(f, up));
    Vec3 u = cross3(r, f);
    Mat4 m = Mat4::identity();
    m.m[0][0]=r.x; m.m[1][0]=r.y; m.m[2][0]=r.z;
    m.m[0][1]=u.x; m.m[1][1]=u.y; m.m[2][1]=u.z;
    m.m[0][2]=-f.x;m.m[1][2]=-f.y;m.m[2][2]=-f.z;
    m.m[3][0]=-dot3(r,eye);
    m.m[3][1]=-dot3(u,eye);
    m.m[3][2]= dot3(f,eye);
    return m;
}

// Perspective projection (fovY in radians, OpenGL convention)
static Mat4 perspective(double fovY, double aspect, double near, double far) {
    double f = 1.0 / std::tan(fovY * 0.5);
    Mat4 m;
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far + near) / (near - far);
    m.m[2][3] = -1.0;   // sets clip.w = -viewZ
    m.m[3][2] = (2.0 * far * near) / (near - far);
    return m;
}

static void printVec4(const char* label, const Vec4& v) {
    std::cout << "  " << label << " = ("
              << std::setw(8) << v.x << ", "
              << std::setw(8) << v.y << ", "
              << std::setw(8) << v.z << ", "
              << std::setw(8) << v.w << ")\n";
}

static void printTheory() {
    std::cout << "================ MVP Matrix Pipeline ================\n";
    std::cout << "clip_pos = Projection * View * Model * local_pos\n\n";
    std::cout << "Model  matrix: local -> world\n";
    std::cout << "  translate(tx,ty,tz) * rotateY(angle) * scale(sx,sy,sz)\n";
    std::cout << "  Applied right-to-left: scale first, then rotate, then translate\n\n";
    std::cout << "View   matrix: world -> camera space\n";
    std::cout << "  lookAt(eye, center, up)\n";
    std::cout << "  Camera looks down -Z; world moves to put camera at origin\n\n";
    std::cout << "Projection matrix: camera -> clip space\n";
    std::cout << "  perspective(fovY, aspect, near, far)\n";
    std::cout << "  Stores -viewZ in clip.w, for perspective divide\n\n";
    std::cout << "Perspective divide:  ndc = clip.xyz / clip.w\n";
    std::cout << "  ndc in [-1,1]^3 means the point is visible\n";
    std::cout << "=====================================================\n\n";
}

int main() {
    printTheory();
    std::cout << std::fixed << std::setprecision(4);

    // --- Build matrices ---
    // Object: at world pos (2,0,0), rotated 45° around Y
    Mat4 model = translate(2,0,0) * rotateY(PI/4.0) * scale(1,1,1);
    // Camera: at (0, 3, 8) looking at origin
    Mat4 view  = lookAt({0,3,8}, {0,0,0}, {0,1,0});
    // Projection: 60° FOV, 800x600 window, near=0.1, far=100
    Mat4 proj  = perspective(60.0*PI/180.0, 800.0/600.0, 0.1, 100.0);

    // --- Transform a point through each stage ---
    Vec4 localPos = {0, 0, 0, 1};   // object's local origin

    Vec4 worldPos = model * localPos;
    Vec4 viewPos  = view  * worldPos;
    Vec4 clipPos  = proj  * viewPos;
    Vec4 ndcPos   = {clipPos.x/clipPos.w, clipPos.y/clipPos.w,
                     clipPos.z/clipPos.w, 1.0};

    // Viewport transform (800x600)
    double screenX = (ndcPos.x * 0.5 + 0.5) * 800.0;
    double screenY = (ndcPos.y * 0.5 + 0.5) * 600.0;

    std::cout << "Transforming object origin (0,0,0,1) through pipeline:\n";
    printVec4("local ", localPos);
    printVec4("world ", worldPos);
    printVec4("view  ", viewPos);
    printVec4("clip  ", clipPos);
    printVec4("ndc   ", ndcPos);
    std::cout << "  screen = (" << screenX << ", " << screenY << ")  [px in 800x600]\n\n";

    std::cout << "Matrix order matters! M * v means:\n";
    std::cout << "  1. scale(1,1,1) applied first (scale local)\n";
    std::cout << "  2. rotateY(45°) applied second\n";
    std::cout << "  3. translate(2,0,0) applied last (move in world)\n\n";

    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Why is clip.w = -viewZ? Setup for perspective divide\n";
    std::cout << "  [ ] What is gl_Position? The clip space vec4 output by vertex shader\n";
    std::cout << "  [ ] Viewport transform: pixel = (ndc.xy * 0.5 + 0.5) * viewport_size\n";
    std::cout << "  [ ] Depth in NDC: z_ndc = clip.z / clip.w  (non-linear!)\n";
    std::cout << "  [ ] Why column-major? GPU stores matrix columns contiguously in memory\n";
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why does changing multiplication order of model/view/projection break results?
A key points:
- Matrix multiplication is not commutative.
- Each matrix is defined in a specific coordinate space transition.
- Correct pipeline is local -> world -> view -> clip with consistent convention.
*/
