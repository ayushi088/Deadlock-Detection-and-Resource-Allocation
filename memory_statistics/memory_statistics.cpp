#include "memory_statistics.h"
#include <iostream>
#include <iomanip>
using namespace std;
// Constructor
MemoryStatistics::MemoryStatistics(const MemoryManager& manager)
    : memory(manager)
{
}
// Calculate total physical memory
int MemoryStatistics::calculateTotalMemory() const
{
    return memory.getTotalMemory();
}
// Calculate allocated memory
int MemoryStatistics::calculateAllocatedMemory() const
{
    int allocatedMemory = 0;
    const vector<MemoryFrame>& frames = memory.getFrames();
    for (const auto& frame : frames)
    {
        if (frame.pid != 0)
        {
            allocatedMemory += memory.getFrameSize();
        }
    }
    return allocatedMemory;
}
// Calculate free memory
int MemoryStatistics::calculateFreeMemory() const
{
    return calculateTotalMemory() - calculateAllocatedMemory();
}
// Calculate memory utilization
double MemoryStatistics::calculateMemoryUtilization() const
{
    int totalMemory = calculateTotalMemory();
    if (totalMemory == 0)
        return 0.0;
    return (static_cast<double>(calculateAllocatedMemory())
            / totalMemory) * 100.0;
}
// Calculate internal fragmentation
int MemoryStatistics::calculateInternalFragmentation() const
{
    int fragmentation = 0;
    const vector<ProcessMemoryInfo>& processes =
        memory.getProcessInfo();

    int frameSize = memory.getFrameSize();
    for (const auto& process : processes)
    {
        int allocatedMemory =
            process.allocatedPages * frameSize;
        fragmentation +=
            allocatedMemory - process.requestedSize;
    }
    return fragmentation;
}
// Display all memory statistics
void MemoryStatistics::displayStatistics() const
{
    cout << "\n========== MEMORY STATISTICS ==========\n";
    cout << "Total Memory: "
         << calculateTotalMemory() << " KB\n";
    cout << "Allocated Memory: "
         << calculateAllocatedMemory() << " KB\n";
    cout << "Free Memory: "
         << calculateFreeMemory() << " KB\n";
    cout << "Memory Utilization: "
         << fixed << setprecision(2)
         << calculateMemoryUtilization() << "%\n";
    cout << "Internal Fragmentation: "
         << calculateInternalFragmentation() << " KB\n";
    cout << "=======================================\n";
}
