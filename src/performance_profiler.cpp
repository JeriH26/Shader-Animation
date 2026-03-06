#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "performance_profiler.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

namespace {

#ifndef GL_TIME_ELAPSED
#define GL_TIME_ELAPSED 0x88BF
#endif

std::int64_t nowNs() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count();
}

bool hasExtension(const char* target) {
    if (!target || std::strlen(target) == 0) return false;

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    if (numExtensions > 0) {
        for (GLint i = 0; i < numExtensions; ++i) {
            const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
            if (ext && std::strcmp(ext, target) == 0) {
                return true;
            }
        }
        return false;
    }

    const char* extList = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!extList) return false;

    std::string all(extList);
    std::string token = std::string(" ") + target + " ";
    all = " " + all + " ";
    return all.find(token) != std::string::npos;
}

bool supportsTimerQuery() {
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    if (major > 3 || (major == 3 && minor >= 3)) return true;
    return hasExtension("GL_ARB_timer_query") || hasExtension("GL_EXT_timer_query");
}

} // namespace

void PerformanceProfiler::Stats::add(double value) {
    ++count;
    sum += value;
    min = std::min(min, value);
    max = std::max(max, value);
}

double PerformanceProfiler::Stats::avg() const {
    return count > 0 ? (sum / static_cast<double>(count)) : 0.0;
}

PerformanceProfiler::PerformanceProfiler(Config config) : config_(std::move(config)) {
    startNs_ = nowNs();
    frameStartNs_ = startNs_;
    cpuSubmitEndNs_ = frameStartNs_;
    lastPrintNs_ = startNs_;

    if (!config_.csvPath.empty()) {
        csv_.open(config_.csvPath, std::ios::out | std::ios::trunc);
        if (!csv_) {
            std::cerr << "[Profiler] Failed to open CSV file: " << config_.csvPath << std::endl;
        }
    }
}

PerformanceProfiler::~PerformanceProfiler() {
    finalize();
}

void PerformanceProfiler::initializeGpuTimer() {
    if (!config_.enabled || !config_.enableGpu) return;

    if (!supportsTimerQuery()) {
        std::cout << "[Profiler] GPU timer query not supported on this context. GPU timing disabled." << std::endl;
        gpuSupported_ = false;
        return;
    }

    queryPool_.resize(16, 0);
    glGenQueries(static_cast<GLsizei>(queryPool_.size()), queryPool_.data());
    gpuSupported_ = true;
}

void PerformanceProfiler::beginFrame() {
    if (!config_.enabled) return;
    frameStartNs_ = nowNs();
    cpuSubmitEndNs_ = frameStartNs_;
}

void PerformanceProfiler::markCpuSubmitEnd() {
    if (!config_.enabled) return;
    cpuSubmitEndNs_ = nowNs();
}

void PerformanceProfiler::beginGpuSection() {
    if (!config_.enabled || !gpuSupported_ || queryPool_.empty()) return;

    unsigned int queryId = queryPool_[nextQueryIndex_];
    nextQueryIndex_ = (nextQueryIndex_ + 1) % queryPool_.size();

    glBeginQuery(GL_TIME_ELAPSED, queryId);
    pendingQueries_.push_back(PendingQuery{queryId, frameCounter_ + 1});
}

void PerformanceProfiler::endGpuSection() {
    if (!config_.enabled || !gpuSupported_) return;
    glEndQuery(GL_TIME_ELAPSED);
}

void PerformanceProfiler::endFrame() {
    if (!config_.enabled) return;

    ++frameCounter_;
    const std::int64_t now = nowNs();
    const double cpuSubmitMs = static_cast<double>(cpuSubmitEndNs_ - frameStartNs_) / 1e6;
    const double cpuFrameMs = static_cast<double>(now - frameStartNs_) / 1e6;
    cpuSubmitMs_.add(cpuSubmitMs);
    cpuFrameMs_.add(cpuFrameMs);

    flushAvailableGpuQueries(false);

    if (csv_) {
        if (!csvHeaderWritten_) {
            csv_ << "frame,cpu_submit_ms,cpu_frame_ms,gpu_ms\n";
            csvHeaderWritten_ = true;
        }

        double lastGpuMs = -1.0;
        const auto gpuIt = resolvedGpuMsByFrame_.find(frameCounter_);
        if (gpuIt != resolvedGpuMsByFrame_.end()) {
            lastGpuMs = gpuIt->second;
            resolvedGpuMsByFrame_.erase(gpuIt);
        }
        csv_ << frameCounter_ << ',' << cpuSubmitMs << ',' << cpuFrameMs << ',' << lastGpuMs << '\n';
    }

    printPeriodic();
}

bool PerformanceProfiler::shouldStop() const {
    if (!config_.enabled || config_.maxFrames == 0) return false;
    return frameCounter_ >= config_.maxFrames;
}

void PerformanceProfiler::flushAvailableGpuQueries(bool blockUntilDone) {
    if (!gpuSupported_) return;

    while (!pendingQueries_.empty()) {
        const PendingQuery q = pendingQueries_.front();

        GLint available = GL_FALSE;
        if (!blockUntilDone) {
            glGetQueryObjectiv(q.id, GL_QUERY_RESULT_AVAILABLE, &available);
            if (available != GL_TRUE) {
                break;
            }
        }

        GLuint64 elapsedNs = 0;
        glGetQueryObjectui64v(q.id, GL_QUERY_RESULT, &elapsedNs);
        const double gpuMs = static_cast<double>(elapsedNs) / 1e6;
        gpuFrameMs_.add(gpuMs);
        resolvedGpuMsByFrame_[q.frameIndex] = gpuMs;
        pendingQueries_.pop_front();
    }
}

void PerformanceProfiler::printPeriodic() {
    const std::int64_t now = nowNs();
    const double elapsedSec = static_cast<double>(now - lastPrintNs_) / 1e9;
    if (elapsedSec < config_.printIntervalSeconds) return;

    const double totalSec = static_cast<double>(now - startNs_) / 1e9;
    const double fps = totalSec > 0.0 ? static_cast<double>(frameCounter_) / totalSec : 0.0;

    std::cout
        << "[Profiler] frames=" << frameCounter_
        << " fps=" << fps
        << " cpu_submit_avg_ms=" << cpuSubmitMs_.avg()
        << " cpu_frame_avg_ms=" << cpuFrameMs_.avg();

    if (gpuSupported_ && gpuFrameMs_.count > 0) {
        std::cout << " gpu_avg_ms=" << gpuFrameMs_.avg();
    }

    std::cout << std::endl;
    lastPrintNs_ = now;
}

void PerformanceProfiler::printFinalReport() const {
    const std::int64_t endNs = nowNs();
    const double totalSec = static_cast<double>(endNs - startNs_) / 1e9;
    const double fps = totalSec > 0.0 ? static_cast<double>(frameCounter_) / totalSec : 0.0;

    std::cout << "\n=== Performance Report ===" << std::endl;
    std::cout << "Frames: " << frameCounter_ << std::endl;
    std::cout << "Total time (s): " << totalSec << std::endl;
    std::cout << "Average FPS: " << fps << std::endl;

    if (cpuFrameMs_.count > 0) {
        std::cout
            << "CPU submit ms (avg/min/max): "
            << cpuSubmitMs_.avg() << "/" << cpuSubmitMs_.min << "/" << cpuSubmitMs_.max
            << std::endl;
        std::cout
            << "CPU frame ms (avg/min/max): "
            << cpuFrameMs_.avg() << "/" << cpuFrameMs_.min << "/" << cpuFrameMs_.max
            << std::endl;
    }

    if (gpuSupported_ && gpuFrameMs_.count > 0) {
        std::cout
            << "GPU frame ms (avg/min/max): "
            << gpuFrameMs_.avg() << "/" << gpuFrameMs_.min << "/" << gpuFrameMs_.max
            << std::endl;
    } else if (config_.enableGpu) {
        std::cout << "GPU frame ms: unavailable" << std::endl;
    }

    if (csv_) {
        std::cout << "CSV written to: " << config_.csvPath << std::endl;
    }
    std::cout << "==========================\n" << std::endl;
}

void PerformanceProfiler::finalize() {
    if (finalized_) return;
    finalized_ = true;

    if (config_.enabled) {
        flushAvailableGpuQueries(true);
        printFinalReport();
    }

    if (!queryPool_.empty()) {
        glDeleteQueries(static_cast<GLsizei>(queryPool_.size()), queryPool_.data());
        queryPool_.clear();
    }

    if (csv_) {
        csv_.flush();
        csv_.close();
    }
}
