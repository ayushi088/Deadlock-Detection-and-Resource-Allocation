#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H
#include <vector>
using namespace std;
struct MemoryFrame
{
    int frameId;
    int pid;
    int pageNumber;
};
struct PageTableEntry
{
    int pid;
    int pageNumber;
    int frameId;
};
struct ProcessMemoryInfo
{
    int pid;
    int requestedSize;
    int allocatedPages;
};
class MemoryManager
{
private:
    int totalMemory;
    int frameSize;
    vector<MemoryFrame> frames;
    vector<PageTableEntry> pageTable;
    vector<ProcessMemoryInfo> processInfo;

public:
    MemoryManager(int memorySize, int pageSize);
    void displayMemory() const;
    void displayPageTable(int pid) const;
    void allocateMemory(int pid, int processSize);
    void deallocateMemory(int pid);
    void displayStatistics() const;
    // Getters for the Memory Statistics module
    int getTotalMemory() const;
    int getFrameSize() const;
    const vector<MemoryFrame>& getFrames() const;
    const vector<ProcessMemoryInfo>& getProcessInfo() const;
};
#endif
