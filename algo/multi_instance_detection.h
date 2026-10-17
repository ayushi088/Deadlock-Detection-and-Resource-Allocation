#ifndef MULTI_INSTANCE_DETECTION_H
#define MULTI_INSTANCE_DETECTION_H

#include <vector>

using namespace std;

struct MultiInstanceResult{
    bool deadlockDetected;
    vector<int> deadlockedProcesses;
};

class MultipleInstanceDetection{
public:
    MultiInstanceResult detectDeadlock(
        const vector<int>& available,
        const vector<vector<int>>& allocation,
        const vector<vector<int>>& request) const;
};

#endif
