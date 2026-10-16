#include "deadlock_detector.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool readNonNegativeCount(const char* prompt, int& value)
{
    std::cout << prompt;
    if (!(std::cin >> value) || value < 0)
        return false;
    return true;
}
}

int main()
{
    int processCount = 0;
    int resourceCount = 0;

    std::cout << "===== INTEGRATED DEADLOCK DETECTOR =====\n";
    if (!readNonNegativeCount("Enter number of processes: ", processCount) ||
        !readNonNegativeCount("Enter number of resource types: ", resourceCount) ||
        processCount < 1 || resourceCount < 1)
    {
        std::cerr << "Process and resource counts must be positive integers.\n";
        return 1;
    }

    std::vector<int> available(static_cast<std::size_t>(resourceCount));
    std::vector<std::vector<int>> allocation(
        static_cast<std::size_t>(processCount),
        std::vector<int>(static_cast<std::size_t>(resourceCount)));
    std::vector<std::vector<int>> request(
        static_cast<std::size_t>(processCount),
        std::vector<int>(static_cast<std::size_t>(resourceCount)));

    std::cout << "\nEnter available instances for each resource type:\n";
    for (int resource = 0; resource < resourceCount; ++resource)
    {
        if (!readNonNegativeCount(("Available R" + std::to_string(resource) + ": ").c_str(),
                                  available[static_cast<std::size_t>(resource)]))
        {
            std::cerr << "Resource counts must be non-negative integers.\n";
            return 1;
        }
    }

    std::cout << "\nEnter allocation matrix (one row per process):\n";
    for (int process = 0; process < processCount; ++process)
    {
        std::cout << "P" << process << ": ";
        for (int resource = 0; resource < resourceCount; ++resource)
        {
            if (!(std::cin >> allocation[static_cast<std::size_t>(process)]
                                         [static_cast<std::size_t>(resource)]) ||
                allocation[static_cast<std::size_t>(process)]
                          [static_cast<std::size_t>(resource)] < 0)
            {
                std::cerr << "Allocation counts must be non-negative integers.\n";
                return 1;
            }
        }
    }

    std::cout << "\nEnter request matrix (one row per process):\n";
    for (int process = 0; process < processCount; ++process)
    {
        std::cout << "P" << process << ": ";
        for (int resource = 0; resource < resourceCount; ++resource)
        {
            if (!(std::cin >> request[static_cast<std::size_t>(process)]
                                      [static_cast<std::size_t>(resource)]) ||
                request[static_cast<std::size_t>(process)]
                       [static_cast<std::size_t>(resource)] < 0)
            {
                std::cerr << "Request counts must be non-negative integers.\n";
                return 1;
            }
        }
    }

    try
    {
        printDeadlockReport(runDeadlockDetection(available, allocation, request));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Detection failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
