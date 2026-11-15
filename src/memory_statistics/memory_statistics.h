#ifndef MEMORY_STATISTICS_H
#define MEMORY_STATISTICS_H
#include "../memory_management/memory_management.h"
using namespace std;
class MemoryStatistics
{
private:
    const MemoryManager& memory;
public:
    MemoryStatistics(const MemoryManager& manager);
    int calculateTotalMemory() const;
    int calculateAllocatedMemory() const;
    int calculateFreeMemory() const;
    double calculateMemoryUtilization() const;
    int calculateInternalFragmentation() const;
    void displayStatistics() const;
};
#endif
