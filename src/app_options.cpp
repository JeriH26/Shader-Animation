#include "app_options.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage() {
    std::cout
        << "Usage: shader_app [fragment_shader] [options]\n"
        << "Options:\n"
        << "  --no-profile               Disable profiler output\n"
        << "  --gpu-profile              Enable GPU timer query\n"
        << "  --no-gpu-profile           Disable GPU timer query\n"
        << "  --profile-frames N         Auto-exit after N frames\n"
        << "  --profile-csv PATH         Write per-frame CSV report\n"
        << "  --profile-interval SEC     Print interval stats every SEC seconds\n";
}

} // namespace

ParseStatus parseAppOptions(int argc, char** argv, AppOptions& outOptions) {
#if defined(__APPLE__)
    outOptions.profilerConfig.enableGpu = false; // safer default on macOS OpenGL
#endif

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage();
            return ParseStatus::ExitSuccess;
        }

        if (arg == "--no-profile") {
            outOptions.profilerConfig.enabled = false;
            continue;
        }
        if (arg == "--gpu-profile") {
            outOptions.profilerConfig.enableGpu = true;
            continue;
        }
        if (arg == "--no-gpu-profile") {
            outOptions.profilerConfig.enableGpu = false;
            continue;
        }
        if (arg == "--profile-frames" && i + 1 < argc) {
            try {
                outOptions.profilerConfig.maxFrames = static_cast<std::size_t>(std::stoull(argv[++i]));
            } catch (const std::exception&) {
                std::cerr << "Invalid value for --profile-frames: " << argv[i] << std::endl;
                return ParseStatus::ExitFailure;
            }
            continue;
        }
        if (arg == "--profile-csv" && i + 1 < argc) {
            outOptions.profilerConfig.csvPath = argv[++i];
            continue;
        }
        if (arg == "--profile-interval" && i + 1 < argc) {
            try {
                outOptions.profilerConfig.printIntervalSeconds = std::stod(argv[++i]);
            } catch (const std::exception&) {
                std::cerr << "Invalid value for --profile-interval: " << argv[i] << std::endl;
                return ParseStatus::ExitFailure;
            }
            continue;
        }

        if (arg.rfind("--", 0) == 0) {
            std::cerr << "Unknown option: " << arg << std::endl;
            return ParseStatus::ExitFailure;
        }

        outOptions.fragPath = arg;
    }

    return ParseStatus::Ok;
}
