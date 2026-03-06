#pragma once

#include "frame_profiler.h"

#include <string>

struct AppOptions {
    std::string fragPath = "./Shadertoy_Fragment/morph_sphere_cube.frag";
    FrameProfiler::Config profilerConfig;
};

enum class ParseStatus {
    Ok,
    ExitSuccess,
    ExitFailure
};

ParseStatus parseAppOptions(int argc, char** argv, AppOptions& outOptions);

