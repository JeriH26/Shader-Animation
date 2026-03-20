#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vec3 operator+(const Vec3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vec3 operator-(const Vec3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
};

static double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct Sphere {
    Vec3 center;
    double radius = 1.0;
};

struct HitInfo {
    bool hit = false;
    double tNear = -1.0;
    double tFar = -1.0;
    Vec3 pointNear{};
    Vec3 pointFar{};
};

static HitInfo intersectRaySphere(const Ray& ray, const Sphere& sphere) {
    const Vec3 oc = ray.origin - sphere.center;

    const double a = dot(ray.direction, ray.direction);
    const double halfB = dot(oc, ray.direction);
    const double c = dot(oc, oc) - sphere.radius * sphere.radius;

    const double discriminant = halfB * halfB - a * c;
    if (discriminant < 0.0 || a == 0.0) {
        return {};
    }

    const double sqrtD = std::sqrt(discriminant);
    double t0 = (-halfB - sqrtD) / a;
    double t1 = (-halfB + sqrtD) / a;
    if (t0 > t1) std::swap(t0, t1);

    if (t1 < 0.0) {
        return {};
    }

    HitInfo result;
    result.hit = true;
    result.tNear = (t0 >= 0.0) ? t0 : t1;
    result.tFar = t1;
    result.pointNear = ray.origin + ray.direction * result.tNear;
    result.pointFar = ray.origin + ray.direction * result.tFar;
    return result;
}

struct TestCase {
    std::string name;
    Ray ray;
    Sphere sphere;
    bool expectedHit;
    std::string note;
};

static void printTheory() {
    std::cout << "================ Ray-Sphere Interview Script ================\n";
    std::cout << "1) Ray: P(t) = O + tD, t >= 0\n";
    std::cout << "2) Sphere: |P - C|^2 = R^2\n";
    std::cout << "3) Substitute ray into sphere => quadratic\n";
    std::cout << "   a = dot(D, D)\n";
    std::cout << "   halfB = dot(O - C, D)\n";
    std::cout << "   c = dot(O - C, O - C) - R^2\n";
    std::cout << "   discriminant = halfB^2 - a*c\n";
    std::cout << "4) discriminant < 0 -> miss, ==0 -> tangent, >0 -> 2 roots\n";
    std::cout << "5) choose smallest non-negative root as first visible hit\n";
    std::cout << "=============================================================\n\n";
}

static void runTests() {
    const std::vector<TestCase> tests = {
        {
            "Case A: hit from outside",
            {{0.0, 0.0, -5.0}, {0.0, 0.0, 1.0}},
            {{0.0, 0.0, 0.0}, 1.0},
            true,
            "Ray aims at sphere center, should hit"
        },
        {
            "Case B: miss",
            {{0.0, 2.0, -5.0}, {0.0, 0.0, 1.0}},
            {{0.0, 0.0, 0.0}, 1.0},
            false,
            "Ray stays above sphere"
        },
        {
            "Case C: origin inside sphere",
            {{0.0, 0.0, 0.2}, {0.0, 0.0, 1.0}},
            {{0.0, 0.0, 0.0}, 1.0},
            true,
            "Near root is negative, far root is exiting point"
        },
        {
            "Case D: tangent",
            {{1.0, 0.0, -5.0}, {0.0, 0.0, 1.0}},
            {{0.0, 0.0, 0.0}, 1.0},
            true,
            "Touches sphere at one point"
        }
    };

    std::cout << std::fixed << std::setprecision(4);
    int passCount = 0;

    for (const auto& t : tests) {
        const HitInfo hit = intersectRaySphere(t.ray, t.sphere);
        const bool pass = (hit.hit == t.expectedHit);
        if (pass) ++passCount;

        std::cout << t.name << "\n";
        std::cout << "  Note: " << t.note << "\n";
        std::cout << "  Expected: " << (t.expectedHit ? "Hit" : "Miss")
                  << ", Actual: " << (hit.hit ? "Hit" : "Miss")
                  << " -> " << (pass ? "PASS" : "FAIL") << "\n";

        if (hit.hit) {
            std::cout << "  tNear=" << hit.tNear
                      << ", tFar=" << hit.tFar << "\n";
            std::cout << "  nearPoint=(" << hit.pointNear.x << ", " << hit.pointNear.y << ", " << hit.pointNear.z << ")\n";
        }
        std::cout << "\n";
    }

    std::cout << "Summary: " << passCount << "/" << tests.size() << " cases passed\n\n";
}

static void printPracticeChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Try normalized direction D and compare t values\n";
    std::cout << "  [ ] Add epsilon for floating-point comparisons\n";
    std::cout << "  [ ] Return surface normal N = normalize(hitPoint - C)\n";
    std::cout << "  [ ] Extend to ray-plane and ray-AABB intersection\n";
}

int main() {
    printTheory();
    runTests();
    printPracticeChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why do many implementations use halfB instead of full b in ray-sphere intersection?
A key points:
- Start from a*t^2 + b*t + c = 0, where b = 2*dot(oc, d).
- Define halfB = dot(oc, d), then discriminant is halfB^2 - a*c.
- This removes repeated factor 2 operations and improves numerical stability/readability.
*/
