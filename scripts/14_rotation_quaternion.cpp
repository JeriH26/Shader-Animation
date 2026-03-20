// ================================================================
// 14_rotation_quaternion.cpp  -- Rotation Representations
// ================================================================
// Three ways to represent 3D rotation:
//
//  1. Euler Angles (rx, ry, rz)
//     - Intuitive, but ORDER MATTERS (Rz*Ry*Rx != Rx*Ry*Rz)
//     - Subject to GIMBAL LOCK: two axes align => lose a DOF
//     - Not suitable for interpolation
//
//  2. Rotation Matrix (3x3, SO(3))
//     - R^T * R = I, det(R) = 1
//     - Efficient for transforming many points
//     - Hard to interpolate, 9 values but only 3 DOF
//
//  3. Quaternion q = (w, x, y, z)
//     - |q| = 1 (unit quaternion for rotation)
//     - q = (cos(theta/2), sin(theta/2)*axis)
//     - Compose rotations: q_total = q2 * q1  (apply q1 first)
//     - Smooth interpolation: slerp
//     - Compact (4 values), no gimbal lock
//
// Gimbal Lock demo:
//   Apply Ry(90°), then try to rotate around X or Z:
//   After Ry(90°) maps X→-Z and Z→X, Rx and Rz now rotate around same axis!
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

static const double DEG = 3.14159265358979 / 180.0;

struct Vec3 { double x=0, y=0, z=0; };

static Vec3 normalize(Vec3 v) {
    double l = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    return l>1e-9 ? Vec3{v.x/l, v.y/l, v.z/l} : v;
}
static double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

// ---- 3x3 Rotation Matrix ----
struct Mat3 {
    double m[3][3] = {};  // m[col][row]
    static Mat3 identity() {
        Mat3 r; r.m[0][0]=r.m[1][1]=r.m[2][2]=1; return r;
    }
    Vec3 operator*(Vec3 v) const {
        return {
            m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z,
            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z,
            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z
        };
    }
    Mat3 operator*(const Mat3& rhs) const {
        Mat3 r;
        for (int col=0;col<3;++col)
            for (int row=0;row<3;++row)
                for (int k=0;k<3;++k)
                    r.m[col][row] += m[k][row]*rhs.m[col][k];
        return r;
    }
};

static Mat3 Rx(double rad) {
    Mat3 r = Mat3::identity();
    double c=cos(rad), s=sin(rad);
    r.m[1][1]=c; r.m[2][1]=-s;
    r.m[1][2]=s; r.m[2][2]= c;
    return r;
}
static Mat3 Ry(double rad) {
    Mat3 r = Mat3::identity();
    double c=cos(rad), s=sin(rad);
    r.m[0][0]= c; r.m[2][0]=s;
    r.m[0][2]=-s; r.m[2][2]=c;
    return r;
}
static Mat3 Rz(double rad) {
    Mat3 r = Mat3::identity();
    double c=cos(rad), s=sin(rad);
    r.m[0][0]=c; r.m[1][0]=-s;
    r.m[0][1]=s; r.m[1][1]= c;
    return r;
}

// ---- Quaternion ----
struct Quat {
    double w=1, x=0, y=0, z=0;

    // Build from axis-angle: q = (cos(theta/2), sin(theta/2)*axis)
    static Quat fromAxisAngle(Vec3 axis, double rad) {
        Vec3 n = normalize(axis);
        double half = rad * 0.5;
        double s = std::sin(half);
        return {std::cos(half), n.x*s, n.y*s, n.z*s};
    }

    double norm() const { return std::sqrt(w*w+x*x+y*y+z*z); }

    // Quaternion multiplication: q1*q2 = apply q1 after q2
    Quat operator*(const Quat& r) const {
        return {
            w*r.w - x*r.x - y*r.y - z*r.z,
            w*r.x + x*r.w + y*r.z - z*r.y,
            w*r.y - x*r.z + y*r.w + z*r.x,
            w*r.z + x*r.y - y*r.x + z*r.w
        };
    }

    // Rotate vector v by this quaternion: v' = q*v*q^-1
    Vec3 rotate(Vec3 v) const {
        // Extract vector part
        Vec3 qv = {x, y, z};
        Vec3 t  = cross(qv, v);
        t = Vec3{t.x*2, t.y*2, t.z*2};
        Vec3 tx = cross(qv, t);
        return {
            v.x + w*t.x + tx.x,
            v.y + w*t.y + tx.y,
            v.z + w*t.z + tx.z
        };
    }
};

static void printVec3(const char* name, Vec3 v) {
    std::cout << std::fixed << std::setprecision(4)
              << "  " << name << " = (" << v.x << ", " << v.y << ", " << v.z << ")\n";
}

static void printTheory() {
    std::cout << "================ Rotation Representations ================\n";
    std::cout << "Euler angles: Ry(b)*Rx(a)*Rz(c) etc — order matters!\n";
    std::cout << "  Problem: Gimbal Lock when two rotation axes align\n\n";
    std::cout << "Rotation matrix: 3x3, R^T=R^-1, det=1 (9 values, 3 DOF)\n";
    std::cout << "  Good for transforming many points, bad for interpolation\n\n";
    std::cout << "Quaternion q = (w, x, y, z) = (cos(θ/2), sin(θ/2)*axis)\n";
    std::cout << "  |q| = 1 for unit quaternion (rotation)\n";
    std::cout << "  Compose: q_total = q2 * q1  (q1 applied first)\n";
    std::cout << "  Rotate v: q * v * q_conjugate (or use Hamilton product shortcut)\n";
    std::cout << "  No gimbal lock, compact, smooth slerp interpolation\n";
    std::cout << "==========================================================\n\n";
}

static void gimbalLockDemo() {
    std::cout << "---- Gimbal Lock Demo ----\n";
    std::cout << "Start with identity. Apply Ry(90°) first.\n";
    std::cout << "Then try Rx(45°) vs Rz(45°) — they should move DIFFERENT axes.\n\n";

    Vec3 fwd = {0, 0, 1};  // forward vector

    // Without Ry: Rx and Rz affect different axes
    Vec3 xTilt = Rx(45.0*DEG) * fwd;
    Vec3 zRoll = Rz(45.0*DEG) * fwd;
    std::cout << "  Without Ry: Rx(45°)*fwd vs Rz(45°)*fwd:\n";
    printVec3("Rx(45°)*fwd", xTilt);
    printVec3("Rz(45°)*fwd", zRoll);
    std::cout << "  -> they move in DIFFERENT directions (good, no lock)\n\n";

    // After Ry(90°): X axis maps to -Z axis
    Mat3 afterRy = Ry(90.0*DEG);
    Vec3 xTiltAfter = Rx(45.0*DEG) * (afterRy * fwd);
    Vec3 zRollAfter = Ry(90.0*DEG) * (Rz(45.0*DEG) * fwd);

    // Actually let's show the Euler ZYX gimbal lock properly
    // If pitch (Y) is 90°, roll (Z) and yaw (X) become the same axis
    Mat3 Rzval = Rz(30.0*DEG);
    Mat3 Ry90  = Ry(90.0*DEG);
    Mat3 Rxval = Rx(30.0*DEG);

    Vec3 testV = {1, 0, 0};  // X axis
    Vec3 rYX = (Ry90 * Rxval) * testV;    // Ry90 after Rx
    Vec3 rYZ = (Ry90 * Rzval) * testV;    // Ry90 after Rz
    std::cout << "  With Ry(90°) applied after each:\n";
    std::cout << "  Ry(90°)*Rx(30°)*(1,0,0):\n";
    printVec3("result", rYX);
    std::cout << "  Ry(90°)*Rz(30°)*(1,0,0):\n";
    printVec3("result", rYZ);
    std::cout << "  -> Rx and Rz both rotate around the same world axis => GIMBAL LOCK\n\n";
    std::cout << "  Quaternion solution: use slerp and compose in quaternion space\n\n";
}

static void quaternionDemo() {
    std::cout << "---- Quaternion Demo ----\n";

    // Rotation 90° around Y axis
    Quat qY = Quat::fromAxisAngle({0,1,0}, 90.0*DEG);
    std::cout << "  qY (90° around Y): w=" << qY.w << " x=" << qY.x
              << " y=" << qY.y << " z=" << qY.z << "\n";
    std::cout << "  |qY| = " << qY.norm() << "\n\n";

    Vec3 fwd = {0,0,1};
    Vec3 rotated = qY.rotate(fwd);
    std::cout << "  qY.rotate( (0,0,1) ) = should be (-1, 0, 0):\n";
    printVec3("result", rotated);

    // Compose: 90° Y then 90° X
    Quat qX = Quat::fromAxisAngle({1,0,0}, 90.0*DEG);
    Quat qComposed = qX * qY;  // qY applied first, then qX
    Vec3 up = {0,1,0};
    Vec3 result = qComposed.rotate(up);
    std::cout << "\n  qX(90°)*qY(90°) applied to (0,1,0):\n";
    printVec3("result", result);
    std::cout << "  (should be pointing along some rotated direction)\n\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Gimbal lock: middle Euler rotation at +-90° locks two axes\n";
    std::cout << "  [ ] Quaternion to rotation matrix: 9 elements from w,x,y,z components\n";
    std::cout << "  [ ] Quaternion conjugate q* = (w, -x, -y, -z) = inverse for unit q\n";
    std::cout << "  [ ] Why normalize quaternion after many multiplications? Accumulate float error\n";
    std::cout << "  [ ] glm::quat, Eigen::Quaternion, Unity Quaternion all use (w,x,y,z) or (x,y,z,w)\n";
}

int main() {
    printTheory();
    gimbalLockDemo();
    quaternionDemo();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: What does quaternion normalization protect against in real engines?
A key points:
- Repeated multiply/interpolate accumulates floating-point drift.
- Non-unit quaternions introduce scaling/shear artifacts in derived rotation matrices.
- Periodic renormalization keeps rotations pure and numerically stable.
*/
