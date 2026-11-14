#include "memory_management.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
using namespace std;
// Constructor
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
// Allocate memory to a process using paging
void MemoryManager::allocateMemory(int pid, int processSize)
{
    if (pid <= 0 || processSize <= 0)
    {
        cout << "Invalid PID or process size.\n";
        return;
    }
    // Check whether memory is already allocated to this process
    for (const auto& entry : pageTable)
    {
        if (entry.pid == pid)
        {
            cout << "Memory already allocated to PID "
                 << pid << ".\n";
            return;
        }
    }
    // Calculate required pages
    int requiredPages = (processSize + frameSize - 1) / frameSize;
    // Count available frames
    int freeFrames = 0;
    for (const auto& frame : frames)
    {
        if (frame.pid == 0)
            freeFrames++;
    }
    // Check whether sufficient memory is available
    if (requiredPages > freeFrames)
    {
        cout << "Not enough free memory frames.\n";
        return;
    }
    int page = 0;
    // Allocate free frames to the process
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
    // Store original requested size and allocated pages
    // Required for calculating internal fragmentation
    processInfo.push_back({pid, processSize, requiredPages});
    cout << "Memory allocated successfully to PID "
         << pid << ".\n";
}
// Deallocate memory from a process
void MemoryManager::deallocateMemory(int pid)
{
    bool found = false;
    // Release allocated frames
    for (auto& frame : frames)
    {
        if (frame.pid == pid)
        {
            frame.pid = 0;
            frame.pageNumber = -1;
            found = true;
        }
    }
    // Remove page table entries
    for (int i = 0; i < pageTable.size(); )
    {
        if (pageTable[i].pid == pid)
            pageTable.erase(pageTable.begin() + i);
        else
            i++;
    }
    // Remove process memory information
    for (int i = 0; i < processInfo.size(); )
    {
        if (processInfo[i].pid == pid)
            processInfo.erase(processInfo.begin() + i);
        else
            i++;
    }
    if (found)
        cout << "Memory released for PID " << pid << ".\n";
    else
        cout << "No memory allocation found for PID "
             << pid << ".\n";
}
// Display physical memory frames
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
// Display page table of a process
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
// Get total memory
int MemoryManager::getTotalMemory() const
{
    return totalMemory;
}
// Get frame size
int MemoryManager::getFrameSize() const
{
    return frameSize;
}
// Get physical memory frames
const vector<MemoryFrame>& MemoryManager::getFrames() const
{
    return frames;
}
// Get process memory information
const vector<ProcessMemoryInfo>& MemoryManager::getProcessInfo() const
{
    return processInfo;
}
