#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>

struct Process
{
    int pid = 0;
    int arrivalTime = 0;
    int burstTime = 0;
    int priority = 0;
    int remainingTime = 0;
    int completionTime = 0;
    int turnaroundTime = 0;
    int waitingTime = 0;
    int responseTime = -1;
    bool terminated = false;
    std::string state = "NEW";
};

struct GanttSegment
{
    int pid = 0; // 0 means that the CPU was idle.
    int startTime = 0;
    int endTime = 0;
};

struct StateHistoryEntry
{
    int pid = 0;
    std::string state;
    int atTime = 0;
};

struct SchedulingResult
{
    int pid = 0;
    int completionTime = 0;
    int turnaroundTime = 0;
    int waitingTime = 0;
    int responseTime = 0;
};

struct SchedulingRun
{
    long long runId = 0;
    std::string algorithm;
    int quantum = 0;
    std::vector<Process> processes;
    std::vector<GanttSegment> chart;
    std::vector<SchedulingResult> results;
    std::vector<StateHistoryEntry> stateHistory;
};

void createProcess(std::vector<Process>& processes);
void terminateProcess(std::vector<Process>& processes);
void displayProcesses(const std::vector<Process>& processes);

void scheduleFCFS(std::vector<Process>& processes);
void scheduleSJF(std::vector<Process>& processes);
void scheduleRoundRobin(std::vector<Process>& processes, int quantum);
void schedulePriority(std::vector<Process>& processes);

// Writes the latest run as a transaction accepted by MySQL.
bool exportLatestRunToSql(const std::string& filePath);

#endif
