
#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <vector>

struct MemoryFrame
{
    int frameId;
    int pid;          // 0 means the frame is free
    int pageNumber;   // Page belonging to the process
};

struct PageTableEntry
{
    int pid;
    int pageNumber;
    int frameId;
};

class MemoryManager
{
private:
    int totalMemory;
    int frameSize;
    std::vector<MemoryFrame> frames;
    std::vector<PageTableEntry> pageTable;

public:
    MemoryManager(int memorySize, int pageSize);

    void displayMemory() const;
    void displayPageTable(int pid) const;

    void allocateMemory(int pid, int processSize);
    void deallocateMemory(int pid);

    void displayStatistics() const;
};

#endif
