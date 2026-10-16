#include "deadlock_detector.h"

#include "multi_instance_detection.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

DeadlockReport runDeadlockDetection(
    const std::vector<int>& available,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request)
{
    const std::size_t processCount = allocation.size();
    const std::size_t resourceCount = available.size();

    if (processCount > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        resourceCount > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        processCount + resourceCount >
            static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("Process and resource counts are too large.");

    if (request.size() != processCount)
        throw std::invalid_argument("Allocation and request must have the same process count.");
    for (int count : available)
        if (count < 0)
            throw std::invalid_argument("Available resource counts cannot be negative.");

    std::vector<long long> totalInstances(available.begin(), available.end());
    for (std::size_t process = 0; process < processCount; ++process)
    {
        if (allocation[process].size() != resourceCount ||
            request[process].size() != resourceCount)
            throw std::invalid_argument("Every matrix row must match the resource count.");
        for (std::size_t resource = 0; resource < resourceCount; ++resource)
        {
            if (allocation[process][resource] < 0 || request[process][resource] < 0)
                throw std::invalid_argument("Allocation and request counts cannot be negative.");
            totalInstances[resource] += allocation[process][resource];
        }
    }

    RAGCycleDetection rag(static_cast<int>(processCount),
                          static_cast<int>(resourceCount));
    for (std::size_t process = 0; process < processCount; ++process)
        rag.setNodeName(static_cast<int>(process), "P" + std::to_string(process));
    for (std::size_t resource = 0; resource < resourceCount; ++resource)
        rag.setNodeName(static_cast<int>(processCount + resource),
                        "R" + std::to_string(resource));

    for (std::size_t process = 0; process < processCount; ++process)
    {
        for (std::size_t resource = 0; resource < resourceCount; ++resource)
        {
            const int resourceNode = static_cast<int>(processCount + resource);
            if (allocation[process][resource] > 0)
                rag.addEdge(resourceNode, static_cast<int>(process));
            if (request[process][resource] > 0)
                rag.addEdge(static_cast<int>(process), resourceNode);
        }
    }

    const CycleDetectionResult ragResult = rag.detectCycle();
    DeadlockReport report;
    report.cycles.reserve(ragResult.cyclePaths.size());

    for (std::size_t index = 0; index < ragResult.cyclePaths.size(); ++index)
    {
        CycleReport cycle;
        cycle.path = ragResult.cyclePaths[index];
        cycle.processIds = ragResult.cycleProcessIds[index];
        cycle.allResourcesSingleInstance = true;

        for (int resource : ragResult.cycleResourceIds[index])
        {
            if (resource < 0 || static_cast<std::size_t>(resource) >= totalInstances.size() ||
                totalInstances[static_cast<std::size_t>(resource)] != 1)
                cycle.allResourcesSingleInstance = false;
        }
        report.cycles.push_back(std::move(cycle));
    }

    const bool hasMultipleInstanceResource =
        std::any_of(totalInstances.begin(), totalInstances.end(),
                    [](long long count) { return count > 1; });

    if (hasMultipleInstanceResource)
    {
        report.matrixDetectionUsed = true;
        const MultiInstanceResult matrixResult =
            MultipleInstanceDetection().detectDeadlock(available, allocation, request);
        report.deadlockedProcesses = matrixResult.deadlockedProcesses;
    }

    for (const CycleReport& cycle : report.cycles)
    {
        if (cycle.allResourcesSingleInstance)
        {
            report.deadlockedProcesses.insert(report.deadlockedProcesses.end(),
                                               cycle.processIds.begin(),
                                               cycle.processIds.end());
        }
    }

    std::sort(report.deadlockedProcesses.begin(), report.deadlockedProcesses.end());
    report.deadlockedProcesses.erase(
        std::unique(report.deadlockedProcesses.begin(), report.deadlockedProcesses.end()),
        report.deadlockedProcesses.end());
    report.deadlockDetected = !report.deadlockedProcesses.empty();
    return report;
}

void printDeadlockReport(const DeadlockReport& report)
{
    std::cout << "\n===== COMMON DEADLOCK DETECTOR =====\n";
    std::cout << "Deadlock detected: " << (report.deadlockDetected ? "YES" : "NO") << '\n';

    std::cout << "Deadlocked processes: ";
    if (report.deadlockedProcesses.empty())
    {
        std::cout << "None\n";
    }
    else
    {
        for (std::size_t index = 0; index < report.deadlockedProcesses.size(); ++index)
        {
            if (index > 0)
                std::cout << ", ";
            std::cout << 'P' << report.deadlockedProcesses[index];
        }
        std::cout << '\n';
    }

    if (report.cycles.empty())
    {
        std::cout << "RAG cycles: None\n";
    }
    else
    {
        std::cout << "RAG cycles:\n";
        for (std::size_t index = 0; index < report.cycles.size(); ++index)
        {
            const CycleReport& cycle = report.cycles[index];
            std::cout << "  Cycle " << index + 1 << ": ";
            for (std::size_t node = 0; node < cycle.path.size(); ++node)
            {
                if (node > 0)
                    std::cout << " -> ";
                std::cout << cycle.path[node];
            }

            std::cout << "\n    Participating processes: ";
            if (cycle.processIds.empty())
            {
                std::cout << "None";
            }
            else
            {
                for (std::size_t process = 0; process < cycle.processIds.size(); ++process)
                {
                    if (process > 0)
                        std::cout << ", ";
                    std::cout << 'P' << cycle.processIds[process];
                }
            }
            std::cout << '\n';
        }
    }

    if (!report.deadlockDetected && !report.cycles.empty())
        std::cout << "A RAG cycle exists, but deadlock was not confirmed.\n";
    if (report.matrixDetectionUsed)
        std::cout << "Multiple-instance matrix detection was used.\n";

    std::cout << "====================================\n";
}
