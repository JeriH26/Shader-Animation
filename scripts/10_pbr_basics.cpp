// ================================================================
// 10_pbr_basics.cpp  -- PBR: Cook-Torrance BRDF
// ================================================================
// PBR = Physically Based Rendering
// BRDF = Bidirectional Reflectance Distribution Function
//
// Cook-Torrance BRDF (specular term):
//   f_r = (D * F * G) / (4 * dot(N,V) * dot(N,L))
//
// where:
//   D = Normal Distribution Function (GGX/Trowbridge-Reitz)
//       How many microfacets are aligned with H (the half-vector)
//   F = Fresnel Function (Schlick approximation)
//       How much light is reflected vs refracted at the surface
//   G = Geometry / Shadowing-Masking Function (Smith + GGX)
//       Accounts for microfacets that block each other
//
// Full rendering equation for direct lighting:
//   Lo = (kd * albedo/pi + ks * f_cook-torrance) * Li * dot(N,L)
//   where kd = 1 - ks,  ks = F
//
// Parameters:
//   roughness  in [0, 1]: 0 = mirror, 1 = completely rough/diffuse
//   metallic   in [0, 1]: 0 = dielectric, 1 = metal
//   albedo          : base color
//   F0             : base reflectivity when looking head-on
//                    dielectrics: F0 = 0.04
//                    metals:      F0 = albedo
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3 operator+(const Vec3& r) const { return {x+r.x, y+r.y, z+r.z}; }
    Vec3 operator-(const Vec3& r) const { return {x-r.x, y-r.y, z-r.z}; }
    Vec3 operator*(double s)      const { return {x*s, y*s, z*s}; }
    Vec3 operator*(const Vec3& r) const { return {x*r.x, y*r.y, z*r.z}; }
    Vec3 lerp(const Vec3& b, double t) const { return {x+(b.x-x)*t, y+(b.y-y)*t, z+(b.z-z)*t}; }
};

static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static double len(const Vec3& v) { return std::sqrt(dot(v,v)); }
static Vec3 normalize(const Vec3& v) { double l=len(v); return {v.x/l,v.y/l,v.z/l}; }
static double clamp(double v, double lo, double hi) { return v<lo?lo:v>hi?hi:v; }
static const double PI = 3.14159265358979;

// ---- D: GGX Normal Distribution Function ----
// D(N,H,a) = a^2 / (pi * ((N.H)^2 * (a^2 - 1) + 1)^2)
// a = roughness^2  (remapped roughness for better visual linearity)
double distributionGGX(const Vec3& N, const Vec3& H, double roughness) {
    double a  = roughness * roughness;
    double a2 = a * a;
    double NdotH  = clamp(dot(N, H), 0.0, 1.0);
    double NdotH2 = NdotH * NdotH;
    double denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// ---- F: Fresnel-Schlick approximation ----
// F(V,H) = F0 + (1 - F0) * (1 - dot(V,H))^5
Vec3 fresnelSchlick(const Vec3& V, const Vec3& H, const Vec3& F0) {
    double cosTheta = clamp(dot(V, H), 0.0, 1.0);
    double x = 1.0 - cosTheta;
    double x5 = x*x*x*x*x;
    return F0 + (Vec3{1,1,1} - F0) * x5;
}

// ---- G: Smith Geometry Function (GGX Schlick-Beckmann) ----
// G_sub(N,X,k) = dot(N,X) / (dot(N,X)*(1-k) + k)
// k = (roughness+1)^2 / 8  for direct lighting
// G = G_sub(N,V,k) * G_sub(N,L,k)
double geometrySmith(const Vec3& N, const Vec3& V, const Vec3& L, double roughness) {
    auto gsub = [&](const Vec3& X) {
        double k     = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        double NdotX = clamp(dot(N, X), 0.0, 1.0);
        return NdotX / (NdotX * (1.0 - k) + k);
    };
    return gsub(V) * gsub(L);
}

// ---- Cook-Torrance radiance contribution ----
Vec3 cookTorrance(const Vec3& N, const Vec3& L, const Vec3& V,
                  const Vec3& albedo, double roughness, double metallic,
                  const Vec3& lightColor) {
    Vec3 H = normalize(Vec3{L.x+V.x, L.y+V.y, L.z+V.z});

    // Base reflectivity F0
    Vec3 F0_dielectric = {0.04, 0.04, 0.04};
    Vec3 F0 = F0_dielectric.lerp(albedo, metallic);

    double D = distributionGGX(N, H, roughness);
    Vec3   F = fresnelSchlick(V, H, F0);
    double G = geometrySmith(N, V, L, roughness);

    double NdotL = clamp(dot(N, L), 0.0, 1.0);
    double NdotV = clamp(dot(N, V), 1e-4, 1.0);   // avoid /0

    // Cook-Torrance specular
    Vec3 ks = F;
    Vec3 kd = (Vec3{1,1,1} - ks) * (1.0 - metallic);   // metals have no diffuse

    Vec3 specular = (ks * D * G) * (1.0 / (4.0 * NdotV * NdotL + 1e-4));
    Vec3 diffuse  = kd * albedo * (1.0 / PI);

    return (diffuse + specular) * lightColor * NdotL;
}

static void printTheory() {
    std::cout << "================ PBR: Cook-Torrance BRDF ================\n";
    std::cout << "Specular BRDF = (D * F * G) / (4 * NdotV * NdotL)\n\n";
    std::cout << "D = Normal Distribution Function (GGX)\n";
    std::cout << "  How many microfacets face the half-vector H\n";
    std::cout << "  D(N,H,a) = a^2 / (pi * ((N.H)^2*(a^2-1)+1)^2)  where a=roughness^2\n\n";
    std::cout << "F = Fresnel (Schlick)\n";
    std::cout << "  How much light is reflected (vs refracted/transmitted)\n";
    std::cout << "  F = F0 + (1-F0)*(1-dot(V,H))^5\n";
    std::cout << "  F0 = 0.04 for dielectrics, F0 = albedo for metals\n\n";
    std::cout << "G = Geometry (Smith + GGX Schlick-Beckmann)\n";
    std::cout << "  Accounts for microfacets shadowing/masking each other\n";
    std::cout << "  G = G_sub(N,V,k) * G_sub(N,L,k),  k=(roughness+1)^2/8\n\n";
    std::cout << "Lo = (kd*albedo/pi + ks*DFG/(4*NdotV*NdotL)) * Li * NdotL\n";
    std::cout << "=========================================================\n\n";
}

int main() {
    printTheory();
    std::cout << std::fixed << std::setprecision(4);

    Vec3 N = normalize({0,1,0.2});      // slight tilt
    Vec3 L = normalize({0.5,1.0,0.3});  // light direction
    Vec3 V = normalize({0.3,1.0,-0.5}); // view direction
    Vec3 lightColor = {5.0, 5.0, 5.0};

    std::cout << "Roughness & metalness sweep (red dielectric surface):\n";
    std::cout << std::setw(10) << "roughness" << std::setw(10) << "metallic"
              << std::setw(12) << "Lo.r" << std::setw(12) << "Lo.g" << std::setw(12) << "Lo.b\n";
    std::cout << "  " << std::string(50, '-') << "\n";

    Vec3 albedo = {0.8, 0.2, 0.1};
    for (double rough : {0.1, 0.3, 0.5, 0.8, 1.0}) {
        for (double metal : {0.0, 1.0}) {
            Vec3 lo = cookTorrance(N, L, V, albedo, rough, metal, lightColor);
            std::cout << "  " << std::setw(8) << rough
                      << std::setw(10) << metal
                      << std::setw(12) << lo.x
                      << std::setw(12) << lo.y
                      << std::setw(12) << lo.z << "\n";
        }
    }

    std::cout << "\nD term alone (how sharp/wide the highlight is):\n";
    Vec3 H = normalize(Vec3{L.x+V.x, L.y+V.y, L.z+V.z});
    std::cout << "  N.H = " << dot(N,H) << "\n";
    for (double r : {0.05, 0.1, 0.3, 0.5, 0.9}) {
        std::cout << "  roughness=" << r << "  D=" << distributionGGX(N,H,r) << "\n";
    }

    std::cout << "\nPractice Checklist:\n";
    std::cout << "  [ ] Metallic workflow: metals use albedo as F0, dielectrics use 0.04\n";
    std::cout << "  [ ] Roughness 0 -> mirror-like; roughness 1 -> Lambertian\n";
    std::cout << "  [ ] D determines specular shape; F controls color; G prevents over-bright edges\n";
    std::cout << "  [ ] For IBL: replace Li * NdotL with split-sum: prefiltered env + BRDF LUT\n";
    return 0;
}

/*
Interview Follow-up Q&A:
Q: In PBR, how do roughness and metallic affect visual output physically?
A key points:
- Roughness controls microfacet normal variance, widening and dimming specular peaks.
- Metallic blends F0 from dielectric baseline (~0.04) toward albedo-colored specular.
- Metals have near-zero diffuse term, while dielectrics keep diffuse energy.
*/
