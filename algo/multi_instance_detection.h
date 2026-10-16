#ifndef MULTI_INSTANCE_DETECTION_H
#define MULTI_INSTANCE_DETECTION_H

#include <vector>

struct MultiInstanceResult
{
    bool deadlockDetected;
    std::vector<int> deadlockedProcesses;
};

class MultipleInstanceDetection
{
public:
    MultiInstanceResult detectDeadlock(
        const std::vector<int>& available,
        const std::vector<std::vector<int>>& allocation,
        const std::vector<std::vector<int>>& request) const;
};

#endif
