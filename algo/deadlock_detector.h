#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include "rag_cycle_detection.h"

#include <string>
#include <vector>

using namespace std;

struct CycleReport{
    vector<string> path;
    vector<int> processIds;
    bool allResourcesSingleInstance = false;
};

struct DeadlockReport{
    bool deadlockDetected = false;
    vector<int> deadlockedProcesses;
    vector<CycleReport> cycles;
    bool matrixDetectionUsed = false;
};

DeadlockReport runDeadlockDetection(
    const vector<int>& available,
    const vector<vector<int>>& allocation,
    const vector<vector<int>>& request);

void printDeadlockReport(const DeadlockReport& report);

#endif
