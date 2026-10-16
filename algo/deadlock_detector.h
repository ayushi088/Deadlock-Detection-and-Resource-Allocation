#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include "rag_cycle_detection.h"

#include <string>
#include <vector>

struct CycleReport
{
    std::vector<std::string> path;
    std::vector<int> processIds;
    bool allResourcesSingleInstance = false;
};

struct DeadlockReport
{
    bool deadlockDetected = false;
    std::vector<int> deadlockedProcesses;
    std::vector<CycleReport> cycles;
    bool matrixDetectionUsed = false;
};

DeadlockReport runDeadlockDetection(
    const std::vector<int>& available,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request);

void printDeadlockReport(const DeadlockReport& report);

#endif
