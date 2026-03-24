#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

class PerformanceProfiler {
public:
    struct Config {
        bool enabled = true;
        bool enableGpu = true;
        std::string csvPath;
        std::size_t maxFrames = 0; // 0 means unlimited
        double printIntervalSeconds = 1.0;
    };

    explicit PerformanceProfiler(Config config);
    ~PerformanceProfiler();

    void initializeGpuTimer();
    void beginFrame();
    void markCpuSubmitEnd();
    void markSwapBegin();
    void markSwapEnd();
    void markEventPollEnd();
    void beginGpuSection();
    void endGpuSection();
    void endFrame();

    bool shouldStop() const;
    void finalize();

private:
    struct Stats {
        std::size_t count = 0;
        double sum = 0.0;
        double min = 1e30;
        double max = 0.0;

        void add(double value);
        double avg() const;
    };

    struct PendingQuery {
        unsigned int id = 0;
        std::size_t frameIndex = 0;
    };

    void flushAvailableGpuQueries(bool blockUntilDone);
    void printPeriodic();
    void printFinalReport() const;

    Config config_;
    bool finalized_ = false;
    bool gpuSupported_ = false;

    std::vector<unsigned int> queryPool_;
    std::size_t nextQueryIndex_ = 0;
    std::deque<PendingQuery> pendingQueries_;
    std::unordered_map<std::size_t, double> resolvedGpuMsByFrame_;

    std::ofstream csv_;
    bool csvHeaderWritten_ = false;

    std::size_t frameCounter_ = 0;
    std::int64_t startNs_ = 0;
    std::int64_t frameStartNs_ = 0;
    std::int64_t cpuSubmitEndNs_ = 0;
    std::int64_t swapStartNs_ = 0;
    std::int64_t swapEndNs_ = 0;
    std::int64_t eventPollEndNs_ = 0;
    std::int64_t lastPrintNs_ = 0;

    Stats cpuSubmitMs_;
    Stats cpuFrameMs_;
    Stats swapWaitMs_;
    Stats eventPollMs_;
    Stats gpuFrameMs_;
};
