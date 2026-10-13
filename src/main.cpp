#include "scheduler.h"

#include <iostream>
#include <limits>

int main()
{
    std::vector<Process> processes;
    int choice = -1;
    do
    {
        std::cout << "\n===== Process Management + CPU Scheduling =====\n"
                     "1. Create Process\n"
                     "2. Display Processes\n"
                     "3. Terminate Process\n"
                     "4. FCFS Scheduling\n"
                     "5. SJF Scheduling (Non-preemptive)\n"
                     "6. Round Robin Scheduling\n"
                     "7. Priority Scheduling (Non-preemptive)\n"
                     "8. Export Latest Result for MySQL\n"
                     "0. Exit\nEnter choice: ";
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a number.\n";
            continue;
        }
        switch (choice)
        {
        case 1: createProcess(processes); break;
        case 2: displayProcesses(processes); break;
        case 3: terminateProcess(processes); break;
        case 4: scheduleFCFS(processes); break;
        case 5: scheduleSJF(processes); break;
        case 6:
        {
            int quantum;
            std::cout << "Enter time quantum: ";
            if (std::cin >> quantum)
                scheduleRoundRobin(processes, quantum);
            else
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid time quantum.\n";
            }
            break;
        }
        case 7: schedulePriority(processes); break;
        case 8:
            if (exportLatestRunToSql("latest_scheduling_run.sql"))
                std::cout << "Saved latest_scheduling_run.sql. Run it after schema.sql in MySQL.\n";
            else
                std::cout << "Run a scheduling algorithm before exporting.\n";
            break;
        case 0: std::cout << "Exiting.\n"; break;
        default: std::cout << "Invalid choice.\n";
        }
    } while (choice != 0);
    return 0;
}
