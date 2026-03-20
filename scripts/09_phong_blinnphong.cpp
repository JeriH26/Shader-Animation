// ================================================================
// 09_phong_blinnphong.cpp  -- Phong & Blinn-Phong Lighting
// ================================================================
// Both models compute: Ambient + Diffuse + Specular
//
// PHONG:
//   I = ka*Ia + kd * max(0, dot(N,L)) * Id + ks * max(0, dot(R,V))^n * Is
//   R = reflect(-L, N) = 2*dot(N,L)*N - L
//
// BLINN-PHONG (physically more plausible, cheaper):
//   H = normalize(L + V)   <- half-way vector between light and view
//   I = ka*Ia + kd * max(0, dot(N,L)) * Id + ks * max(0, dot(N,H))^n * Is
//
// Components:
//   N  = surface normal (unit vector)
//   L  = direction TO the light source (unit vector)
//   V  = direction TO the viewer/camera (unit vector)
//   R  = perfect reflection of -L around N
//   ka,kd,ks = ambient/diffuse/specular material coefficients
//   n  = shininess exponent (higher = sharper specular)
//   Ia,Id,Is = ambient/diffuse/specular light intensities
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec3 {
    double r=0, g=0, b=0;
    Vec3 operator+(const Vec3& o) const { return {r+o.r, g+o.g, b+o.b}; }
    Vec3 operator*(double s)      const { return {r*s, g*s, b*s}; }
    Vec3 operator*(const Vec3& o) const { return {r*o.r, g*o.g, b*o.b}; }
};

using Color = Vec3;

struct Dir3 {
    double x=0, y=0, z=0;
    Dir3 operator+(const Dir3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Dir3 operator*(double s)      const { return {x*s, y*s, z*s}; }
};

static double dot(const Dir3& a, const Dir3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static double len(const Dir3& a) { return std::sqrt(dot(a,a)); }
static Dir3 normalize(const Dir3& a) { double l=len(a); return {a.x/l, a.y/l, a.z/l}; }
static double clamp01(double v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

// Reflect incident direction I around normal N: R = 2*dot(N,I)*N - I
static Dir3 reflect(const Dir3& I, const Dir3& N) {
    double dn = dot(N, I);
    return normalize(Dir3{2*dn*N.x - I.x, 2*dn*N.y - I.y, 2*dn*N.z - I.z});
}

struct Material {
    Color  ka, kd, ks;       // ambient, diffuse, specular coefficients
    double shininess = 32.0;  // n exponent
};

struct Light {
    Dir3  direction;  // direction FROM surface TOWARD light
    Color ambient, diffuse, specular;
};

// ---- Phong lighting ----
Color phong(const Dir3& N, const Dir3& L, const Dir3& V,
            const Material& mat, const Light& light) {
    double NdotL = clamp01(dot(N, L));
    Dir3   R     = reflect(L, N);       // reflect L around N
    double RdotV = clamp01(dot(R, V));

    Color ambient  = mat.ka * light.ambient;
    Color diffuse  = mat.kd * light.diffuse  * NdotL;
    Color specular = mat.ks * light.specular * std::pow(RdotV, mat.shininess);

    return ambient + diffuse + specular;
}

// ---- Blinn-Phong lighting ----
Color blinnPhong(const Dir3& N, const Dir3& L, const Dir3& V,
                 const Material& mat, const Light& light) {
    double NdotL = clamp01(dot(N, L));
    Dir3   H     = normalize(Dir3{L.x+V.x, L.y+V.y, L.z+V.z});  // half-vector
    double NdotH = clamp01(dot(N, H));

    Color ambient  = mat.ka * light.ambient;
    Color diffuse  = mat.kd * light.diffuse  * NdotL;
    Color specular = mat.ks * light.specular * std::pow(NdotH, mat.shininess);

    return ambient + diffuse + specular;
}

static void printColor(const char* label, const Color& c) {
    std::cout << "  " << std::setw(28) << std::left << label
              << " = (" << std::setw(6) << c.r
              << ", "   << std::setw(6) << c.g
              << ", "   << std::setw(6) << c.b << ")\n";
}

static void printTheory() {
    std::cout << "================ Phong & Blinn-Phong Lighting ================\n";
    std::cout << "Both = Ambient + Diffuse + Specular\n\n";
    std::cout << "PHONG:\n";
    std::cout << "  R = 2*dot(N,L)*N - L             (reflect L around N)\n";
    std::cout << "  I = ka*Ia + kd*max(0,N.L)*Id + ks*max(0,R.V)^n*Is\n\n";
    std::cout << "BLINN-PHONG:\n";
    std::cout << "  H = normalize(L + V)             (half-way vector)\n";
    std::cout << "  I = ka*Ia + kd*max(0,N.L)*Id + ks*max(0,N.H)^n*Is\n\n";
    std::cout << "Why Blinn-Phong is preferred:\n";
    std::cout << "  1. Physically, H stays valid even at grazing angles\n";
    std::cout << "  2. dot(N,H) > 0 as long as both L and V are on the same side as N\n";
    std::cout << "  3. Phong's dot(R,V) can go negative, clamping to 0 causes hard cutoff\n";
    std::cout << "  4. Blinn-Phong is the model used in OpenGL fixed-function pipeline\n";
    std::cout << "================================================================\n\n";
}

int main() {
    printTheory();
    std::cout << std::fixed << std::setprecision(4);

    // Surface normal pointing up
    Dir3 N = {0, 1, 0};
    // Light coming from above-left
    Dir3 L = normalize({0.5, 1.0, 0.3});
    // Viewer looking from above-right
    Dir3 V = normalize({0.5, 1.0, -0.3});

    Material mat;
    mat.ka        = {0.1, 0.1, 0.1};
    mat.kd        = {0.7, 0.4, 0.2};   // warm orange surface
    mat.ks        = {1.0, 1.0, 1.0};   // white specular
    mat.shininess = 32.0;

    Light light;
    light.direction = L;
    light.ambient   = {0.2, 0.2, 0.2};
    light.diffuse   = {1.0, 1.0, 1.0};
    light.specular  = {1.0, 1.0, 1.0};

    std::cout << "Scene:\n";
    std::cout << "  N (normal)  = (0, 1, 0)  pointing up\n";
    std::cout << "  L (light)   = normalized(0.5, 1.0, 0.3)\n";
    std::cout << "  V (viewer)  = normalized(0.5, 1.0, -0.3)\n";
    std::cout << "  Material: orange surface, shininess=32\n\n";

    Color ph  = phong(N, L, V, mat, light);
    Color bph = blinnPhong(N, L, V, mat, light);

    std::cout << "Results:\n";
    printColor("Phong     result",      ph);
    printColor("Blinn-Phong result",    bph);

    std::cout << "\nShininess comparison (Blinn-Phong N.H):\n";
    std::cout << "  Shininess | Specular feel\n";
    double NdotH = clamp01(dot(N, normalize(Dir3{L.x+V.x,L.y+V.y,L.z+V.z})));
    for (double shin : {4.0, 16.0, 64.0, 256.0}) {
        mat.shininess = shin;
        Color c = blinnPhong(N, L, V, mat, light);
        std::cout << "  " << std::setw(9) << shin
                  << " | spec=" << c.r << " (N.H=" << NdotH << ")\n";
    }
    std::cout << "\n";

    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] Changing shininess: 4=rough/wide, 256=shiny/narrow highlight\n";
    std::cout << "  [ ] Multiple lights: sum up diffuse+specular for each, one ambient total\n";
    std::cout << "  [ ] Distance attenuation: multiply by 1/(a + b*d + c*d^2)\n";
    std::cout << "  [ ] Specular in view space vs world space: same result, different convenience\n";
    std::cout << "  [ ] Phong is per-fragment; Gouraud shading is per-vertex (cheaper)\n";
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why does Blinn-Phong often look more stable than Phong at grazing angles?
A key points:
- Blinn-Phong uses half vector H = normalize(L+V), numerically stable.
- It avoids computing explicit reflection vector R each time.
- It usually gives broader, less noisy highlights for equivalent exponent ranges.
*/
