#include "memory_management.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

using namespace std;

MemoryManager::MemoryManager(int memorySize, int pageSize)
    : totalMemory(memorySize), frameSize(pageSize)
{
    if (memorySize <= 0 || pageSize <= 0 ||
        memorySize % pageSize != 0)
    {
        throw invalid_argument(
            "Memory size must be positive and divisible by frame size.");
    }

    int totalFrames = totalMemory / frameSize;

    for (int i = 0; i < totalFrames; i++)
    {
        frames.push_back({i, 0, -1});
    }
}

void MemoryManager::allocateMemory(int pid, int processSize)
{
    if (pid <= 0 || processSize <= 0)
    {
        cout << "Invalid PID or process size.\n";
        return;
    }

    for (const auto& entry : pageTable)
    {
        if (entry.pid == pid)
        {
            cout << "Memory already allocated to PID "
                 << pid << ".\n";
            return;
        }
    }

    int requiredPages = (processSize + frameSize - 1) / frameSize;

    int freeFrames = 0;

    for (const auto& frame : frames)
    {
        if (frame.pid == 0)
            freeFrames++;
    }

    if (requiredPages > freeFrames)
    {
        cout << "Not enough free memory frames.\n";
        return;
    }

    int page = 0;

    for (auto& frame : frames)
    {
        if (frame.pid == 0)
        {
            frame.pid = pid;
            frame.pageNumber = page;

            pageTable.push_back({pid, page, frame.frameId});
            page++;

            if (page == requiredPages)
                break;
        }
    }

    cout << "Memory allocated successfully to PID "
         << pid << ".\n";
}

void MemoryManager::deallocateMemory(int pid)
{
    bool found = false;

    for (auto& frame : frames)
    {
        if (frame.pid == pid)
        {
            frame.pid = 0;
            frame.pageNumber = -1;
            found = true;
        }
    }

    pageTable.erase(
        remove_if(
            pageTable.begin(), pageTable.end(),
            [pid](const PageTableEntry& entry)
            {
                return entry.pid == pid;
            }),
        pageTable.end());

    if (found)
        cout << "Memory released for PID " << pid << ".\n";
    else
        cout << "No memory allocation found for PID "
             << pid << ".\n";
}

void MemoryManager::displayMemory() const
{
    cout << "\nPhysical Memory Frames\n";
    cout << "Frame\tPID\tPage\n";

    for (const auto& frame : frames)
    {
        cout << frame.frameId << '\t'
             << (frame.pid == 0 ? -1 : frame.pid) << '\t'
             << frame.pageNumber << '\n';
    }
}

void MemoryManager::displayPageTable(int pid) const
{
    cout << "\nPage Table for PID " << pid << '\n';
    cout << "Page\tFrame\n";

    bool found = false;

    for (const auto& entry : pageTable)
    {
        if (entry.pid == pid)
        {
            cout << entry.pageNumber << '\t'
                 << entry.frameId << '\n';
            found = true;
        }
    }

    if (!found)
        cout << "No page table entries found.\n";
}

void MemoryManager::displayStatistics() const
{
    int allocatedFrames = 0;
    int internalFragmentation = 0;

    for (const auto& frame : frames)
    {
        if (frame.pid != 0)
            allocatedFrames++;
    }

    int allocatedMemory = allocatedFrames * frameSize;
    int freeMemory = totalMemory - allocatedMemory;

    for (int pid = 1; ; pid++)
    {
        // Internal fragmentation will be calculated
        // in the next refinement of the module.
        if (pid > 0)
            break;
    }

    double utilization =
        (static_cast<double>(allocatedMemory) / totalMemory) * 100.0;

    cout << "\nMemory Statistics\n";
    cout << "Total memory: " << totalMemory << '\n';
    cout << "Allocated memory: " << allocatedMemory << '\n';
    cout << "Free memory: " << freeMemory << '\n';
    cout << "Internal fragmentation: "
         << internalFragmentation << '\n';

    cout << fixed << setprecision(2);
    cout << "Memory utilization: "
         << utilization << "%\n";
}