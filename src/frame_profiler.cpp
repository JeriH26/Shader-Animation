#include "frame_profiler.h"

FrameProfiler::FrameProfiler(Config config) : profiler_(std::move(config)) {}

void FrameProfiler::initialize() {
    profiler_.initializeGpuTimer();
}

void FrameProfiler::beginFrame() {
    profiler_.beginFrame();
}

void FrameProfiler::beginGpuTiming() {
    profiler_.beginGpuSection();
}

void FrameProfiler::endGpuTiming() {
    profiler_.endGpuSection();
}

void FrameProfiler::markCpuSubmitEnd() {
    profiler_.markCpuSubmitEnd();
}

void FrameProfiler::markSwapBegin() {
    profiler_.markSwapBegin();
}

void FrameProfiler::markSwapEnd() {
    profiler_.markSwapEnd();
}

void FrameProfiler::markEventPollEnd() {
    profiler_.markEventPollEnd();
}

void FrameProfiler::endFrame() {
    profiler_.endFrame();
}

bool FrameProfiler::shouldStop() const {
    return profiler_.shouldStop();
}

void FrameProfiler::finalize() {
    profiler_.finalize();
}
