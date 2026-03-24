#pragma once

#include "performance_profiler.h"

class FrameProfiler {
public:
    using Config = PerformanceProfiler::Config;

    explicit FrameProfiler(Config config);

    void initialize();
    void beginFrame();
    void beginGpuTiming();
    void endGpuTiming();
    void markCpuSubmitEnd();
    void markSwapBegin();
    void markSwapEnd();
    void markEventPollEnd();
    void endFrame();

    bool shouldStop() const;
    void finalize();

private:
    PerformanceProfiler profiler_;
};
