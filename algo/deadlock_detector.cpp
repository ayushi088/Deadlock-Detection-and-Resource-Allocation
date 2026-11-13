#include "deadlock_detector.h"

#include "multi_instance_detection.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace std;

DeadlockReport runDeadlockDetection(const vector<int>& available, const vector<vector<int>>& allocation, const vector<vector<int>>& request){
    const size_t processCount = allocation.size();
    const size_t resourceCount = available.size();

    if (processCount > static_cast<size_t>(numeric_limits<int>::max()) ||
        resourceCount > static_cast<size_t>(numeric_limits<int>::max()) ||
        processCount + resourceCount > static_cast<size_t>(numeric_limits<int>::max()))
        throw length_error("Process and resource counts are too large.");

    if (request.size() != processCount)
        throw invalid_argument("Allocation and request must have the same process count.");
    for (int count : available)
        if (count < 0)
            throw invalid_argument("Available resource counts cannot be negative.");

    vector<long long> totalInstances(available.begin(), available.end());
    for (size_t process = 0; process < processCount; ++process){
        if (allocation[process].size() != resourceCount ||
            request[process].size() != resourceCount)
            throw invalid_argument("Every matrix row must match the resource count.");

        for (size_t resource = 0; resource < resourceCount; ++resource){
            if (allocation[process][resource] < 0 || request[process][resource] < 0)
                throw invalid_argument("Allocation and request counts cannot be negative.");
            totalInstances[resource] += allocation[process][resource];
        }
    }

    RAGCycleDetection rag(static_cast<int>(processCount), static_cast<int>(resourceCount));
    for (size_t process = 0; process < processCount; ++process)
        rag.setNodeName(static_cast<int>(process), "P" + to_string(process));

    for (size_t resource = 0; resource < resourceCount; ++resource)
        rag.setNodeName(static_cast<int>(processCount + resource), "R" + to_string(resource));

    for (size_t process = 0; process < processCount; ++process){
        for (size_t resource = 0; resource < resourceCount; ++resource){
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

    for (size_t index = 0; index < ragResult.cyclePaths.size(); ++index){
        CycleReport cycle;
        cycle.path = ragResult.cyclePaths[index];
        cycle.processIds = ragResult.cycleProcessIds[index];
        cycle.allResourcesSingleInstance = true;

        for (int resource : ragResult.cycleResourceIds[index]){
            if (resource < 0 || 
                static_cast<size_t>(resource) >= totalInstances.size() ||
                totalInstances[static_cast<size_t>(resource)] != 1)
                cycle.allResourcesSingleInstance = false;
        }
        report.cycles.push_back(move(cycle));
    }

    bool hasMultipleInstanceResource = false;
    for (long long count : totalInstances){
        if (count > 1){
            hasMultipleInstanceResource = true;
            break;
        }
    }

    if (hasMultipleInstanceResource){
        report.matrixDetectionUsed = true;
        const MultiInstanceResult matrixResult =
            MultipleInstanceDetection().detectDeadlock(available, allocation, request);
        report.deadlockedProcesses = matrixResult.deadlockedProcesses;
    }

    for (const CycleReport& cycle : report.cycles){
        if (cycle.allResourcesSingleInstance){
            report.deadlockedProcesses.insert(report.deadlockedProcesses.end(),
                                               cycle.processIds.begin(),
                                               cycle.processIds.end());
        }
    }

    sort(report.deadlockedProcesses.begin(), report.deadlockedProcesses.end());
    report.deadlockedProcesses.erase(
        unique(report.deadlockedProcesses.begin(),
        report.deadlockedProcesses.end()),
        report.deadlockedProcesses.end());

    report.deadlockDetected = !report.deadlockedProcesses.empty();
    return report;
}

void printDeadlockReport(const DeadlockReport& report){
    cout << "\n===== COMMON DEADLOCK DETECTOR =====\n";
    cout << "Deadlock detected: " << (report.deadlockDetected ? "YES" : "NO") << '\n';

    cout << "Deadlocked processes: ";
    if (report.deadlockedProcesses.empty()){
        cout << "None\n";
    }
    else{
        for (size_t index = 0; index < report.deadlockedProcesses.size(); ++index){
            if (index > 0)
                cout << ", ";
            cout << 'P' << report.deadlockedProcesses[index];
        }
        cout << '\n';
    }

    if (report.cycles.empty()){
        cout << "RAG cycles: None\n";
    }
    else{
        cout << "RAG cycles:\n";
        for (size_t index = 0; index < report.cycles.size(); ++index){
            const CycleReport& cycle = report.cycles[index];
            cout << "  Cycle " << index + 1 << ": ";
            for (size_t node = 0; node < cycle.path.size(); ++node){
                if (node > 0)
                    cout << " -> ";
                cout << cycle.path[node];
            }

            cout << "\n    Participating processes: ";
            if (cycle.processIds.empty()){
                cout << "None";
            }
            else{
                for (size_t process = 0; process < cycle.processIds.size(); ++process){
                    if (process > 0)
                        cout << ", ";
                    cout << 'P' << cycle.processIds[process];
                }
            }
            cout << '\n';
        }
    }

    if (!report.deadlockDetected && !report.cycles.empty())
        cout << "A RAG cycle exists, but deadlock was not confirmed.\n";
    if (report.matrixDetectionUsed)
        cout << "Multiple-instance matrix detection was used.\n";

    cout << "====================================\n";
}
