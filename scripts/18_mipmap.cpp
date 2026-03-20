// ================================================================
// 18_mipmap.cpp  -- Mipmap & Texture Filtering
// ================================================================
// Problem: when a large texture is mapped onto a far-away (small) polygon,
// many texels map to the same screen pixel -> aliasing (shimmering).
//
// Solution: Mipmap (Multi In Partes Minutas Aequas Divisum)
//   Pre-compute a chain of progressively downsampled textures:
//   Level 0: 256x256   (full resolution)
//   Level 1: 128x128
//   Level 2:  64x64
//   Level k:  256/2^k  x  256/2^k
//   (Total storage: 4/3 * original size)
//
// LOD (Level of Detail) selection:
//   LOD = log2( max(|du/dx|, |dv/dy|) * texSize )
//   - |du/dx|, |dv/dy|: how fast UV changes per screen pixel (derivatives)
//   - When far (large derivatives): high LOD (blurry, small texture)
//   - When near (small derivatives): low LOD (sharp, full-res texture)
//
// Filtering modes:
//   Nearest:      sample exact texel (pixelated)
//   Bilinear:     interpolate 4 texels within one mip level
//   Trilinear:    bilinear on two adjacent mip levels + lerp between them
//   Anisotropic:  multiple samples along anisotropic footprint (best quality)
//
// In OpenGL:
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
//   glGenerateMipmap(GL_TEXTURE_2D)
// ================================================================

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

// Simulate a single-channel texture (grayscale mipmap chain)
struct MipLevel {
    int width, height;
    std::vector<float> data;  // row-major

    MipLevel(int w, int h) : width(w), height(h), data(w*h, 0.5f) {}

    float sample(float u, float v) const {
        // Nearest-neighbor
        int x = (int)(u * width)  % width;
        int y = (int)(v * height) % height;
        if (x < 0) x += width;
        if (y < 0) y += height;
        return data[y * width + x];
    }

    float bilinear(float u, float v) const {
        float fx = u * width  - 0.5f;
        float fy = v * height - 0.5f;
        int x0 = (int)std::floor(fx) % width;
        int y0 = (int)std::floor(fy) % height;
        int x1 = (x0 + 1) % width;
        int y1 = (y0 + 1) % height;
        if (x0 < 0) x0 += width;
        if (y0 < 0) y0 += height;
        float tx = fx - std::floor(fx);
        float ty = fy - std::floor(fy);
        float s00 = data[y0*width+x0], s10 = data[y0*width+x1];
        float s01 = data[y1*width+x0], s11 = data[y1*width+x1];
        return (1-tx)*(1-ty)*s00 + tx*(1-ty)*s10 + (1-tx)*ty*s01 + tx*ty*s11;
    }
};

// Build mipmap chain from a base texture
std::vector<MipLevel> buildMipchain(int baseW, int baseH) {
    std::vector<MipLevel> chain;
    int w = baseW, h = baseH;
    // Fill level 0 with a simple checkerboard pattern
    MipLevel base(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            base.data[y*w+x] = ((x/8 + y/8) % 2 == 0) ? 1.0f : 0.0f;
    chain.push_back(base);

    // Downsample each level (box filter: average 2x2 block)
    while (w > 1 || h > 1) {
        int nw = std::max(1, w/2), nh = std::max(1, h/2);
        MipLevel lvl(nw, nh);
        MipLevel& prev = chain.back();
        for (int y = 0; y < nh; ++y)
            for (int x = 0; x < nw; ++x) {
                int px = x*2, py = y*2;
                float sum = prev.data[py*w+px];
                int cnt = 1;
                if (px+1 < w) { sum += prev.data[py*w+px+1];    ++cnt; }
                if (py+1 < h) { sum += prev.data[(py+1)*w+px];  ++cnt; }
                if (px+1<w && py+1<h) { sum += prev.data[(py+1)*w+px+1]; ++cnt; }
                lvl.data[y*nw+x] = sum / cnt;
            }
        chain.push_back(lvl);
        w = nw; h = nh;
    }
    return chain;
}

// Compute LOD: how fast is the UV coordinate changing per pixel?
static float computeLOD(float dudx, float dvdy, int texW, int texH) {
    float maxDeriv = std::max(std::abs(dudx) * texW, std::abs(dvdy) * texH);
    if (maxDeriv < 1e-6f) return 0.0f;
    return std::log2(maxDeriv);
}

// Trilinear sample: bilinear on two adjacent mip levels, lerp between
static float trilinearSample(const std::vector<MipLevel>& chain, float u, float v, float lod) {
    lod = std::max(0.0f, std::min(lod, (float)(chain.size()-1)));
    int level0 = (int)std::floor(lod);
    int level1 = std::min(level0 + 1, (int)chain.size()-1);
    float t     = lod - level0;  // fractional part for level blend

    float s0 = chain[level0].bilinear(u, v);
    float s1 = chain[level1].bilinear(u, v);
    return s0*(1-t) + s1*t;
}

static void printTheory() {
    std::cout << "================ Mipmap & Texture Filtering ================\n";
    std::cout << "Problem: far-away surface -> many texels per pixel -> aliasing\n";
    std::cout << "Solution: precompute downsampled levels (mipmap)\n\n";
    std::cout << "LOD = log2( max(|du/dx|, |dv/dy|) * texSize )\n";
    std::cout << "  Large LOD = far/small object = use smaller mip level\n";
    std::cout << "  LOD=0: 1 texel per pixel (near), LOD=k: 2^k texels per pixel (far)\n\n";
    std::cout << "Filtering quality (worst->best performance tradeoff):\n";
    std::cout << "  Nearest -> Bilinear -> Trilinear -> Anisotropic\n";
    std::cout << "=============================================================\n\n";
}

static void mipmapChainDemo() {
    auto chain = buildMipchain(256, 256);

    std::cout << "Mipmap chain for 256x256 texture:\n";
    std::cout << "  Level | Size      | Texels  | StoragePct\n";
    std::cout << "  ------|-----------|---------|----------\n";
    long total = 0;
    long base  = chain[0].width * chain[0].height;
    for (int i = 0; i < (int)chain.size(); ++i) {
        long sz = chain[i].width * chain[i].height;
        total += sz;
        std::cout << "  " << std::setw(5) << i
                  << " | " << std::setw(4) << chain[i].width << "x" << std::setw(4) << chain[i].height
                  << " | " << std::setw(7) << sz
                  << " | " << std::fixed << std::setprecision(1)
                  << (100.0f*sz/base) << "%\n";
    }
    std::cout << "  Total storage: " << total << " texels ("
              << std::fixed << std::setprecision(1)
              << (100.0f*total/base) << "% of base)\n";
    std::cout << "  -> 4/3 * base size as expected\n\n";
}

static void lodDemo() {
    std::cout << "LOD selection based on distance:\n";
    std::cout << "  Scene: 256x256 texture on a quad\n";
    std::cout << "  UV derivative (du/dx) simulates camera distance\n\n";

    auto chain = buildMipchain(256, 256);

    float conditions[] = {1.0f/256, 1.0f/64, 1.0f/16, 1.0f/4, 1.0f/1};
    const char* descs[] = {"very close (1px=1 texel)", "1px=4 texels",
                            "1px=16 texels", "1px=64 texels", "1px=256 texels (very far)"};

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  du/dx       | LOD  | Sample\n";
    std::cout << "  ------------|------|-------\n";
    for (int i = 0; i < 5; ++i) {
        float dudx = conditions[i];
        float lod = computeLOD(dudx, dudx, 256, 256);
        float val = trilinearSample(chain, 0.5f, 0.5f, lod);
        std::cout << "  " << std::setw(11) << dudx
                  << " | " << std::setw(4) << lod
                  << " | " << val
                  << "  <- " << descs[i] << "\n";
    }
    std::cout << "\n";
}

static void printChecklist() {
    std::cout << "Practice Checklist:\n";
    std::cout << "  [ ] glGenerateMipmap() auto-builds the chain after uploading base texture\n";
    std::cout << "  [ ] GL_LINEAR_MIPMAP_LINEAR = trilinear: best common quality\n";
    std::cout << "  [ ] GL_LINEAR_MIPMAP_NEAREST = bilinear (no level blend): cheaper\n";
    std::cout << "  [ ] Anisotropic: glTexParameterf(..., GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f)\n";
    std::cout << "  [ ] Why 4/3 storage? Sum of geometric series 1 + 1/4 + 1/16 + ... = 4/3\n";
    std::cout << "  [ ] Cube mipmap: 6 faces each with own mip chain (environment maps)\n";
}

int main() {
    printTheory();
    mipmapChainDemo();
    lodDemo();
    printChecklist();
    return 0;
}

/*
Interview Follow-up Q&A:
Q: Why is trilinear filtering often not enough for grazing-angle surfaces?
A key points:
- Footprint becomes anisotropic (elongated), not approximately square.
- Trilinear assumes isotropic footprint and cannot sample enough along long axis.
- Anisotropic filtering adds directional samples to preserve detail and reduce shimmer.
*/
