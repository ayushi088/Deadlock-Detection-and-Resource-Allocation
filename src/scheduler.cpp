#include "scheduler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>

namespace
{
    SchedulingRun latestRun;
    bool hasLatestRun = false;
    long long nextRunId = 1;

    void addSegment(std::vector<GanttSegment>& chart, int pid, int start,
                    int end)
    {
        if (start >= end)
            return;
        if (!chart.empty() && chart.back().pid == pid &&
            chart.back().endTime == start)
        {
            chart.back().endTime = end;
            return;
        }
        chart.push_back({pid, start, end});
    }

    void changeState(Process& process, const std::string& state, int time,
                     std::vector<StateHistoryEntry>& history)
    {
        if (process.state != state)
        {
            process.state = state;
            history.push_back({process.pid, state, time});
        }
    }

    void resetProcesses(std::vector<Process>& processes,
                        std::vector<StateHistoryEntry>& history)
    {
        for (Process& process : processes)
        {
            if (process.terminated)
                continue;
            process.remainingTime = process.burstTime;
            process.completionTime = 0;
            process.turnaroundTime = 0;
            process.waitingTime = 0;
            process.responseTime = -1;
            changeState(process, "READY", 0, history);
        }
    }

    std::vector<int> activeOrder(const std::vector<Process>& processes)
    {
        std::vector<int> order;
        for (int i = 0; i < static_cast<int>(processes.size()); ++i)
            if (!processes[i].terminated)
                order.push_back(i);
        std::sort(order.begin(), order.end(),
                  [&processes](int a, int b)
                  {
                      if (processes[a].arrivalTime != processes[b].arrivalTime)
                          return processes[a].arrivalTime <
                                 processes[b].arrivalTime;
                      return processes[a].pid < processes[b].pid;
                  });
        return order;
    }

    void finish(Process& process, int completion)
    {
        process.completionTime = completion;
        process.turnaroundTime = completion - process.arrivalTime;
        process.waitingTime = process.turnaroundTime - process.burstTime;
        process.remainingTime = 0;
    }

    void saveRun(const std::vector<Process>& processes,
                 const std::vector<GanttSegment>& chart,
                 const std::vector<StateHistoryEntry>& history,
                 const std::string& algorithm, int quantum)
    {
        latestRun = {};
        latestRun.runId = nextRunId++;
        latestRun.algorithm = algorithm;
        latestRun.quantum = quantum;
        latestRun.processes = processes;
        latestRun.chart = chart;
        latestRun.stateHistory = history;
        for (const Process& process : processes)
        {
            if (!process.terminated && process.completionTime > 0)
                latestRun.results.push_back(
                    {process.pid, process.completionTime,
                     process.turnaroundTime, process.waitingTime,
                     process.responseTime});
        }
        hasLatestRun = true;
    }

    void printResults(const std::vector<Process>& processes,
                      const std::vector<GanttSegment>& chart,
                      const std::string& name)
    {
        std::cout << "\n--- " << name << " Scheduling ---\nGantt Chart:\n";
        if (chart.empty())
            std::cout << "No execution segments.\n";
        else
        {
            for (const GanttSegment& segment : chart)
                std::cout << "| "
                          << (segment.pid == 0 ? "Idle" : "P" +
                              std::to_string(segment.pid))
                          << " [" << segment.startTime << "-"
                          << segment.endTime << "] ";
            std::cout << "|\n";
        }

        double waiting = 0, turnaround = 0, response = 0;
        std::cout << "\nPID\tAT\tBT\tPriority\tCT\tTAT\tWT\tRT\n";
        for (const Process& p : processes)
        {
            if (p.terminated || p.completionTime == 0)
                continue;
            std::cout << p.pid << "\t" << p.arrivalTime << "\t"
                      << p.burstTime << "\t" << p.priority << "\t\t"
                      << p.completionTime << "\t" << p.turnaroundTime << "\t"
                      << p.waitingTime << "\t" << p.responseTime << "\n";
            waiting += p.waitingTime;
            turnaround += p.turnaroundTime;
            response += p.responseTime;
        }
        const std::size_t count = latestRun.results.size();
        if (count > 0)
        {
            std::cout << std::fixed << std::setprecision(2)
                      << "\nAverage Waiting Time: " << waiting / count
                      << "\nAverage Turnaround Time: " << turnaround / count
                      << "\nAverage Response Time: " << response / count
                      << "\n";
        }
    }

    void runNonPreemptive(std::vector<Process>& processes, bool priority,
                          const std::string& name)
    {
        if (activeOrder(processes).empty())
        {
            std::cout << "No active processes available.\n";
            return;
        }
        std::vector<StateHistoryEntry> history;
        resetProcesses(processes, history);
        std::vector<GanttSegment> chart;
        std::vector<bool> completed(processes.size(), false);
        int completedCount = 0;
        int current = 0;

        while (completedCount < static_cast<int>(activeOrder(processes).size()))
        {
            int selected = -1;
            for (int i = 0; i < static_cast<int>(processes.size()); ++i)
            {
                if (processes[i].terminated || completed[i] ||
                    processes[i].arrivalTime > current)
                    continue;
                if (selected == -1)
                    selected = i;
                else
                {
                    const Process& a = processes[i];
                    const Process& b = processes[selected];
                    bool better = priority
                        ? (a.priority != b.priority ? a.priority < b.priority
                           : a.arrivalTime != b.arrivalTime
                           ? a.arrivalTime < b.arrivalTime : a.pid < b.pid)
                        : (a.burstTime != b.burstTime ? a.burstTime < b.burstTime
                           : a.arrivalTime != b.arrivalTime
                           ? a.arrivalTime < b.arrivalTime : a.pid < b.pid);
                    if (better)
                        selected = i;
                }
            }
            if (selected == -1)
            {
                int next = -1;
                for (int i = 0; i < static_cast<int>(processes.size()); ++i)
                    if (!processes[i].terminated && !completed[i] &&
                        (next == -1 ||
                         processes[i].arrivalTime < next))
                        next = processes[i].arrivalTime;
                addSegment(chart, 0, current, next);
                current = next;
                continue;
            }
            Process& p = processes[selected];
            changeState(p, "RUNNING", current, history);
            p.responseTime = current - p.arrivalTime;
            addSegment(chart, p.pid, current, current + p.burstTime);
            current += p.burstTime;
            finish(p, current);
            changeState(p, "TERMINATED", current, history);
            completed[selected] = true;
            ++completedCount;
        }
        saveRun(processes, chart, history, name, 0);
        printResults(processes, chart, name);
    }
}

void createProcess(std::vector<Process>& processes)
{
    Process p;
    int highestPid = 0;
    for (const Process& existing : processes)
        highestPid = std::max(highestPid, existing.pid);
    p.pid = highestPid + 1;
    std::cout << "Enter Arrival Time: ";
    if (!(std::cin >> p.arrivalTime))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input.\n";
        return;
    }
    std::cout << "Enter Burst Time: ";
    if (!(std::cin >> p.burstTime))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input.\n";
        return;
    }
    std::cout << "Enter Priority (smaller number = higher priority): ";
    if (!(std::cin >> p.priority))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input.\n";
        return;
    }
    if (p.arrivalTime < 0 || p.burstTime <= 0 || p.priority < 0)
    {
        std::cout << "Invalid values. Process not created.\n";
        return;
    }
    p.remainingTime = p.burstTime;
    processes.push_back(p);
    std::cout << "Process P" << p.pid << " created successfully.\n";
}

void terminateProcess(std::vector<Process>& processes)
{
    int pid;
    std::cout << "Enter PID to terminate: ";
    if (!(std::cin >> pid))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid PID.\n";
        return;
    }
    for (Process& p : processes)
        if (p.pid == pid && !p.terminated)
        {
            p.terminated = true;
            p.remainingTime = 0;
            p.state = "TERMINATED";
            std::cout << "Process P" << pid << " terminated.\n";
            return;
        }
    std::cout << "Active process not found.\n";
}

void displayProcesses(const std::vector<Process>& processes)
{
    if (processes.empty())
    {
        std::cout << "No processes available.\n";
        return;
    }
    std::cout << "\nPID\tAT\tBT\tPriority\tState\n";
    for (const Process& p : processes)
        std::cout << p.pid << "\t" << p.arrivalTime << "\t" << p.burstTime
                  << "\t" << p.priority << "\t\t" << p.state << "\n";
}

void scheduleFCFS(std::vector<Process>& processes)
{
    const std::vector<int> order = activeOrder(processes);
    if (order.empty())
    {
        std::cout << "No active processes available.\n";
        return;
    }
    std::vector<StateHistoryEntry> history;
    resetProcesses(processes, history);
    std::vector<GanttSegment> chart;
    int current = 0;
    for (int index : order)
    {
        Process& p = processes[index];
        if (current < p.arrivalTime)
        {
            addSegment(chart, 0, current, p.arrivalTime);
            current = p.arrivalTime;
        }
        changeState(p, "RUNNING", current, history);
        p.responseTime = current - p.arrivalTime;
        addSegment(chart, p.pid, current, current + p.burstTime);
        current += p.burstTime;
        finish(p, current);
        changeState(p, "TERMINATED", current, history);
    }
    saveRun(processes, chart, history, "FCFS", 0);
    printResults(processes, chart, "FCFS");
}

void scheduleSJF(std::vector<Process>& processes)
{
    runNonPreemptive(processes, false, "SJF");
}

void schedulePriority(std::vector<Process>& processes)
{
    runNonPreemptive(processes, true, "Priority");
}

void scheduleRoundRobin(std::vector<Process>& processes, int quantum)
{
    const std::vector<int> order = activeOrder(processes);
    if (order.empty())
    {
        std::cout << "No active processes available.\n";
        return;
    }
    if (quantum <= 0)
    {
        std::cout << "Time quantum must be greater than zero.\n";
        return;
    }
    std::vector<StateHistoryEntry> history;
    resetProcesses(processes, history);
    std::queue<int> ready;
    std::vector<GanttSegment> chart;
    int current = 0, next = 0, completed = 0;
    while (completed < static_cast<int>(order.size()))
    {
        while (next < static_cast<int>(order.size()) &&
               processes[order[next]].arrivalTime <= current)
            ready.push(order[next++]);
        if (ready.empty())
        {
            const int arrival = processes[order[next]].arrivalTime;
            addSegment(chart, 0, current, arrival);
            current = arrival;
            continue;
        }
        const int index = ready.front();
        ready.pop();
        Process& p = processes[index];
        if (p.responseTime < 0)
            p.responseTime = current - p.arrivalTime;
        changeState(p, "RUNNING", current, history);
        const int runTime = std::min(quantum, p.remainingTime);
        addSegment(chart, p.pid, current, current + runTime);
        current += runTime;
        p.remainingTime -= runTime;
        while (next < static_cast<int>(order.size()) &&
               processes[order[next]].arrivalTime <= current)
            ready.push(order[next++]);
        if (p.remainingTime == 0)
        {
            finish(p, current);
            changeState(p, "TERMINATED", current, history);
            ++completed;
        }
        else
        {
            changeState(p, "READY", current, history);
            ready.push(index);
        }
    }
    saveRun(processes, chart, history, "ROUND_ROBIN", quantum);
    printResults(processes, chart, "Round Robin");
}

bool exportLatestRunToSql(const std::string& filePath)
{
    if (!hasLatestRun)
        return false;
    std::ofstream output(filePath);
    if (!output)
        return false;
    output << "START TRANSACTION;\n";
    for (const Process& p : latestRun.processes)
        output << "INSERT INTO PROCESS "
                  "(process_id, arrival_time, burst_time, priority) VALUES ("
               << p.pid << ", " << p.arrivalTime << ", " << p.burstTime
               << ", " << p.priority
               << ") ON DUPLICATE KEY UPDATE arrival_time = VALUES(arrival_time), "
                  "burst_time = VALUES(burst_time), priority = VALUES(priority);\n";
    output << "INSERT INTO SCHEDULING_RUN "
              "(run_id, algorithm, time_quantum) VALUES ("
           << latestRun.runId << ", '" << latestRun.algorithm << "', "
           << latestRun.quantum << ");\n";
    for (const StateHistoryEntry& h : latestRun.stateHistory)
        output << "INSERT INTO PROCESS_STATE_HISTORY "
                  "(run_id, process_id, state, state_time) VALUES ("
               << latestRun.runId << ", " << h.pid << ", '" << h.state
               << "', " << h.atTime << ");\n";
    for (const SchedulingResult& r : latestRun.results)
        output << "INSERT INTO SCHEDULING_RESULT "
                  "(run_id, process_id, completion_time, turnaround_time, "
                  "waiting_time, response_time) VALUES ("
               << latestRun.runId << ", " << r.pid << ", "
               << r.completionTime << ", " << r.turnaroundTime << ", "
               << r.waitingTime << ", " << r.responseTime << ");\n";
    output << "COMMIT;\n";
    return true;
}
