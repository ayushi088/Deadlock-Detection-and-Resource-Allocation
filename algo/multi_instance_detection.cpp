#include "multi_instance_detection.h"

#include <stdexcept>
#include <vector>

using namespace std;

MultiInstanceResult MultipleInstanceDetection::detectDeadlock(
    const vector<int>& available,
    const vector<vector<int>>& allocation,
    const vector<vector<int>>& request) const
{
    const size_t resourceCount = available.size();
    const size_t processCount = allocation.size();

    if (request.size() != processCount)
        throw invalid_argument("Allocation and request must have the same process count.");

    for (size_t process = 0; process < processCount; ++process){
        if (allocation[process].size() != resourceCount ||
            request[process].size() != resourceCount)
            throw invalid_argument("Every matrix row must match the resource count.");
    }

    for (int count : available)
        if (count < 0)
            throw invalid_argument("Available resource counts cannot be negative.");

    vector<long long> work(available.begin(), available.end());
    vector<bool> finish(processCount, false);

    for (size_t process = 0; process < processCount; ++process){
        bool holdingResources = false;
        for (size_t resource = 0; resource < resourceCount; ++resource){
            if (allocation[process][resource] < 0 || request[process][resource] < 0)
                throw invalid_argument("Allocation and request counts cannot be negative.");
            if (allocation[process][resource] > 0)
                holdingResources = true;
        }

        if (!holdingResources)
            finish[process] = true;
    }

    bool progress = true;
    while (progress){
        progress = false;

        for (size_t process = 0; process < processCount; ++process){
            if (finish[process])
                continue;

            bool canFinish = true;
            for (size_t resource = 0; resource < resourceCount; ++resource){
                if (request[process][resource] > work[resource]){
                    canFinish = false;
                    break;
                }
            }

            if (canFinish){
                for (size_t resource = 0; resource < resourceCount; ++resource)
                    work[resource] += allocation[process][resource];
                finish[process] = true;
                progress = true;
            }
        }
    }

    MultiInstanceResult result{false, {}};
    for (size_t process = 0; process < processCount; ++process){
        if (!finish[process])
            result.deadlockedProcesses.push_back(static_cast<int>(process));
    }
    result.deadlockDetected = !result.deadlockedProcesses.empty();
    return result;
}
