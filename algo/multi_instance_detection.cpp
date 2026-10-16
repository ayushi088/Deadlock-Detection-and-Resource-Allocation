#include "multi_instance_detection.h"

#include <stdexcept>
#include <vector>

MultiInstanceResult MultipleInstanceDetection::detectDeadlock(
    const std::vector<int>& available,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request) const
{
    const std::size_t resourceCount = available.size();
    const std::size_t processCount = allocation.size();

    if (request.size() != processCount)
        throw std::invalid_argument("Allocation and request must have the same process count.");

    for (std::size_t process = 0; process < processCount; ++process)
    {
        if (allocation[process].size() != resourceCount ||
            request[process].size() != resourceCount)
            throw std::invalid_argument("Every matrix row must match the resource count.");
    }

    for (int count : available)
        if (count < 0)
            throw std::invalid_argument("Available resource counts cannot be negative.");

    std::vector<long long> work(available.begin(), available.end());
    std::vector<bool> finish(processCount, false);

    for (std::size_t process = 0; process < processCount; ++process)
    {
        bool holdingResources = false;
        for (std::size_t resource = 0; resource < resourceCount; ++resource)
        {
            if (allocation[process][resource] < 0 || request[process][resource] < 0)
                throw std::invalid_argument("Allocation and request counts cannot be negative.");
            if (allocation[process][resource] > 0)
                holdingResources = true;
        }

        if (!holdingResources)
            finish[process] = true;
    }

    bool progress = true;
    while (progress)
    {
        progress = false;

        for (std::size_t process = 0; process < processCount; ++process)
        {
            if (finish[process])
                continue;

            bool canFinish = true;
            for (std::size_t resource = 0; resource < resourceCount; ++resource)
            {
                if (request[process][resource] > work[resource])
                {
                    canFinish = false;
                    break;
                }
            }

            if (canFinish)
            {
                for (std::size_t resource = 0; resource < resourceCount; ++resource)
                    work[resource] += allocation[process][resource];
                finish[process] = true;
                progress = true;
            }
        }
    }

    MultiInstanceResult result{false, {}};
    for (std::size_t process = 0; process < processCount; ++process)
    {
        if (!finish[process])
            result.deadlockedProcesses.push_back(static_cast<int>(process));
    }
    result.deadlockDetected = !result.deadlockedProcesses.empty();
    return result;
}
