#include "algo/deadlock_detector.h"
#include "scheduler.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
const Socket InvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
const Socket InvalidSocket = -1;
#endif

namespace {

struct Process {
    int id;
    std::string name;
    int arrival = 0;
    int burst = 1;
    int priority = 1;
    std::string state = "READY";
};

struct Resource {
    int id;
    std::string name;
    int total = 0;
    int available = 0;
};

struct Allocation {
    int processId;
    int resourceId;
    int units;
    int requested = 0;
};

std::vector<Process> processes;
std::vector<Resource> resources;
std::vector<Allocation> allocations;
std::vector<std::string> history;
std::mutex stateMutex;

std::string jsonEscape(const std::string& value) {
    std::string result;
    for (char c : value) {
        if (c == '"' || c == '\\')
            result += '\\';
        result += c;
    }
    return result;
}

int numberField(const std::string& body, const std::string& key, int fallback = 0) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t start = body.find(marker);
    if (start == std::string::npos)
        return fallback;
    const std::size_t colon = body.find(':', start + marker.size());
    if (colon == std::string::npos)
        return fallback;
    const char* value = body.c_str() + colon + 1;
    while (*value == ' ' || *value == '\t')
        ++value;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    return end == value ? fallback : static_cast<int>(parsed);
}

std::string stringField(const std::string& body, const std::string& key,
                        const std::string& fallback = "") {
    const std::string marker = "\"" + key + "\"";
    const std::size_t start = body.find(marker);
    if (start == std::string::npos)
        return fallback;
    const std::size_t colon = body.find(':', start + marker.size());
    const std::size_t quote = body.find('"', colon == std::string::npos ? start : colon);
    if (quote == std::string::npos)
        return fallback;
    const std::size_t end = body.find('"', quote + 1);
    return end == std::string::npos ? fallback : body.substr(quote + 1, end - quote - 1);
}

std::string processJson(const Process& process) {
    return "{\"processId\":" + std::to_string(process.id) +
           ",\"pid\":" + std::to_string(process.id) +
           ",\"processName\":\"" + jsonEscape(process.name) +
           "\",\"arrivalTime\":" + std::to_string(process.arrival) +
           ",\"burstTime\":" + std::to_string(process.burst) +
           ",\"priority\":" + std::to_string(process.priority) +
           ",\"state\":\"" + process.state + "\"}";
}

std::string resourceJson(const Resource& resource) {
    const int allocated = resource.total - resource.available;
    const int utilization = resource.total == 0 ? 0 : allocated * 100 / resource.total;
    return "{\"resourceId\":" + std::to_string(resource.id) +
           ",\"id\":" + std::to_string(resource.id) +
           ",\"resourceName\":\"" + jsonEscape(resource.name) +
           "\",\"totalInstances\":" + std::to_string(resource.total) +
           ",\"availableInstances\":" + std::to_string(resource.available) +
           ",\"allocatedInstances\":" + std::to_string(allocated) +
           ",\"utilization\":" + std::to_string(utilization) + "}";
}

std::string allocationsJson() {
    std::string result = "[";
    for (std::size_t i = 0; i < allocations.size(); ++i) {
        if (i)
            result += ',';
        const Allocation& allocation = allocations[i];
        result += "{\"processId\":" + std::to_string(allocation.processId) +
                  ",\"resourceId\":" + std::to_string(allocation.resourceId) +
                  ",\"allocated\":" + std::to_string(allocation.units) +
                  ",\"requested\":" + std::to_string(allocation.requested) + "}";
    }
    return result + "]";
}

std::string arrayOfProcesses() {
    std::string result = "[";
    for (std::size_t i = 0; i < processes.size(); ++i) {
        if (i)
            result += ',';
        result += processJson(processes[i]);
    }
    return result + "]";
}

std::string arrayOfResources() {
    std::string result = "[";
    for (std::size_t i = 0; i < resources.size(); ++i) {
        if (i)
            result += ',';
        result += resourceJson(resources[i]);
    }
    return result + "]";
}

Resource* findResource(int id) {
    for (Resource& resource : resources)
        if (resource.id == id)
            return &resource;
    return nullptr;
}

Process* findProcess(int id) {
    for (Process& process : processes)
        if (process.id == id)
            return &process;
    return nullptr;
}

Allocation* findAllocation(int processId, int resourceId) {
    for (Allocation& allocation : allocations)
        if (allocation.processId == processId && allocation.resourceId == resourceId)
            return &allocation;
    return nullptr;
}

DeadlockReport currentDeadlockReport() {
    std::vector<int> available;
    std::vector<std::vector<int>> allocationMatrix(
        processes.size(), std::vector<int>(resources.size(), 0));
    std::vector<std::vector<int>> requestMatrix(
        processes.size(), std::vector<int>(resources.size(), 0));

    for (const Resource& resource : resources)
        available.push_back(resource.available);
    for (const Allocation& item : allocations) {
        for (std::size_t p = 0; p < processes.size(); ++p) {
            if (processes[p].id != item.processId)
                continue;
            for (std::size_t r = 0; r < resources.size(); ++r) {
                if (resources[r].id == item.resourceId) {
                    allocationMatrix[p][r] = item.units;
                    requestMatrix[p][r] = item.requested;
                }
            }
        }
    }
    return runDeadlockDetection(available, allocationMatrix, requestMatrix);
}

std::string errorJson(const std::string& message) {
    return "{\"error\":\"" + jsonEscape(message) + "\"}";
}

std::string matrixJson(const std::vector<std::vector<int>>& matrix) {
    std::string result = "[";
    for (std::size_t row = 0; row < matrix.size(); ++row) {
        if (row)
            result += ',';
        result += '[';
        for (std::size_t column = 0; column < matrix[row].size(); ++column) {
            if (column)
                result += ',';
            result += std::to_string(matrix[row][column]);
        }
        result += ']';
    }
    return result + ']';
}

std::string schedulingJson(const SchedulingRun& run) {
    std::string results = "[";
    for (std::size_t i = 0; i < run.results.size(); ++i) {
        if (i)
            results += ',';
        const SchedulingResult& result = run.results[i];
        results += "{\"processId\":" + std::to_string(result.pid) +
                   ",\"completionTime\":" + std::to_string(result.completionTime) +
                   ",\"turnaroundTime\":" + std::to_string(result.turnaroundTime) +
                   ",\"waitingTime\":" + std::to_string(result.waitingTime) +
                   ",\"responseTime\":" + std::to_string(result.responseTime) + "}";
    }
    results += ']';
    std::string chart = "[";
    for (std::size_t i = 0; i < run.chart.size(); ++i) {
        if (i)
            chart += ',';
        const GanttSegment& segment = run.chart[i];
        chart += "{\"processId\":" + std::to_string(segment.pid) +
                 ",\"startTime\":" + std::to_string(segment.startTime) +
                 ",\"endTime\":" + std::to_string(segment.endTime) + "}";
    }
    chart += ']';
    return "{\"runId\":" + std::to_string(run.runId) +
           ",\"algorithm\":\"" + jsonEscape(run.algorithm) +
           "\",\"quantum\":" + std::to_string(run.quantum) +
           ",\"results\":" + results + ",\"gantt\":" + chart + "}";
}

std::string bankerJson() {
    std::vector<int> available;
    std::vector<std::vector<int>> allocationMatrix(
        processes.size(), std::vector<int>(resources.size(), 0));
    std::vector<std::vector<int>> maximumMatrix(
        processes.size(), std::vector<int>(resources.size(), 0));
    for (const Resource& resource : resources)
        available.push_back(resource.available);
    for (const Allocation& item : allocations) {
        for (std::size_t p = 0; p < processes.size(); ++p)
            if (processes[p].id == item.processId)
                for (std::size_t r = 0; r < resources.size(); ++r)
                    if (resources[r].id == item.resourceId) {
                        allocationMatrix[p][r] = item.units;
                        maximumMatrix[p][r] = item.units + item.requested;
                    }
    }

    std::vector<std::vector<int>> need = maximumMatrix;
    for (std::size_t p = 0; p < allocationMatrix.size(); ++p)
        for (std::size_t r = 0; r < allocationMatrix[p].size(); ++r)
            need[p][r] -= allocationMatrix[p][r];

    std::vector<int> work = available;
    std::vector<bool> finished(processes.size(), false);
    std::vector<int> sequence;
    bool progress = true;
    while (progress) {
        progress = false;
        for (std::size_t p = 0; p < processes.size(); ++p) {
            if (finished[p])
                continue;
            bool canFinish = true;
            for (std::size_t r = 0; r < resources.size(); ++r)
                if (need[p][r] > work[r])
                    canFinish = false;
            if (!canFinish)
                continue;
            for (std::size_t r = 0; r < resources.size(); ++r)
                work[r] += allocationMatrix[p][r];
            finished[p] = true;
            sequence.push_back(processes[p].id);
            progress = true;
        }
    }
    std::string safeSequence = "[";
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        if (i)
            safeSequence += ',';
        safeSequence += "\"P" + std::to_string(sequence[i]) + "\"";
    }
    safeSequence += ']';
    const bool safe = sequence.size() == processes.size();
    return std::string("{\"safe\":") + (safe ? "true" : "false") +
           ",\"message\":\"" + (safe ? "System is in a safe state." :
                                  "System is not in a safe state.") +
           "\",\"safeSequence\":" + safeSequence +
           ",\"available\":" + matrixJson({available}) +
           ",\"allocationMatrix\":" + matrixJson(allocationMatrix) +
           ",\"maximumMatrix\":" + matrixJson(maximumMatrix) +
           ",\"needMatrix\":" + matrixJson(need) + "}";
}

std::string handleRequest(const std::string& method, const std::string& path,
                          const std::string& body, int& status) {
    std::lock_guard<std::mutex> lock(stateMutex);
    status = 200;

    if (method == "OPTIONS")
        return "{}";

    if (method == "GET" && path == "/api/processes")
        return "{\"processes\":" + arrayOfProcesses() + "}";
    if (method == "GET" && path == "/api/resources")
        return "{\"resources\":" + arrayOfResources() + "}";
    if (method == "GET" && path == "/api/allocations")
        return "{\"processes\":" + arrayOfProcesses() + ",\"resources\":" +
               arrayOfResources() + ",\"allocations\":" + allocationsJson() + "}";
    if (method == "GET" && path == "/api/dashboard") {
        int active = 0;
        int available = 0;
        const DeadlockReport report = currentDeadlockReport();
        for (const Resource& resource : resources) {
            available += resource.available;
            for (const Allocation& allocation : allocations)
                if (allocation.resourceId == resource.id)
                    active += allocation.units;
        }
        return "{\"totalProcesses\":" + std::to_string(processes.size()) +
               ",\"totalResources\":" + std::to_string(resources.size()) +
               ",\"activeAllocations\":" + std::to_string(active) +
               ",\"availableResources\":" + std::to_string(available) +
               ",\"safe\":" + (report.deadlockDetected ? "false" : "true") +
               ",\"detectedDeadlocks\":" +
               std::to_string(report.deadlockedProcesses.size()) +
               ",\"processes\":" +
               arrayOfProcesses() + ",\"resources\":" + arrayOfResources() +
               ",\"utilizationLabels\":[],\"utilizationData\":[],\"stateData\":[]}";
    }
    if (method == "GET" && path == "/api/history") {
        std::string result = "{\"history\":[";
        for (std::size_t i = 0; i < history.size(); ++i) {
            if (i)
                result += ',';
            result += "{\"eventType\":\"system\",\"details\":\"" +
                      jsonEscape(history[i]) + "\"}";
        }
        return result + "]}";
    }
    if (method == "GET" && path == "/api/graph/rag") {
        std::string nodes = "[";
        for (const Process& process : processes) {
            if (nodes.size() > 1)
                nodes += ',';
            nodes += "{\"id\":\"P" + std::to_string(process.id) +
                     "\",\"label\":\"P" + std::to_string(process.id) +
                     "\",\"type\":\"process\"}";
        }
        for (const Resource& resource : resources) {
            if (nodes.size() > 1)
                nodes += ',';
            nodes += "{\"id\":\"R" + std::to_string(resource.id) +
                     "\",\"label\":\"R" + std::to_string(resource.id) +
                     "\",\"type\":\"resource\"}";
        }
        nodes += "]";
        std::string edges = "[";
        bool first = true;
        for (const Allocation& allocation : allocations) {
            if (allocation.units > 0) {
                if (!first)
                    edges += ',';
                first = false;
                edges += "{\"source\":\"R" + std::to_string(allocation.resourceId) +
                         "\",\"target\":\"P" + std::to_string(allocation.processId) +
                         "\",\"type\":\"ALLOCATION\"}";
            }
            if (allocation.requested > 0) {
                if (!first)
                    edges += ',';
                first = false;
                edges += "{\"source\":\"P" + std::to_string(allocation.processId) +
                         "\",\"target\":\"R" + std::to_string(allocation.resourceId) +
                         "\",\"type\":\"REQUEST\"}";
            }
        }
        return "{\"nodes\":" + nodes + ",\"edges\":" + edges + "]}";
    }

    if (method == "POST" && path == "/api/processes") {
        const int id = numberField(body, "processId");
        if (id <= 0 || findProcess(id) != nullptr) {
            status = 400;
            return errorJson("A unique positive processId is required.");
        }
        Process process{id, stringField(body, "processName", "Process " + std::to_string(id))};
        process.arrival = numberField(body, "arrivalTime");
        process.burst = numberField(body, "burstTime", 1);
        process.priority = numberField(body, "priority", 1);
        if (process.arrival < 0 || process.burst <= 0 || process.priority < 0) {
            status = 400;
            return errorJson("Process values are invalid.");
        }
        processes.push_back(process);
        history.push_back("Process P" + std::to_string(id) + " created");
        return "{\"message\":\"Process created\",\"process\":" + processJson(process) + "}";
    }
    if ((method == "PUT" || method == "DELETE") &&
        path.find("/api/processes/") == 0) {
        const int id = std::atoi(path.substr(15).c_str());
        Process* process = findProcess(id);
        if (!process) {
            status = 404;
            return errorJson("Process not found.");
        }
        if (method == "DELETE") {
            processes.erase(std::remove_if(processes.begin(), processes.end(),
                                            [id](const Process& item) { return item.id == id; }),
                            processes.end());
            allocations.erase(std::remove_if(allocations.begin(), allocations.end(),
                                             [id](const Allocation& item) {
                                                 return item.processId == id;
                                             }),
                             allocations.end());
            history.push_back("Process P" + std::to_string(id) + " deleted");
            return "{\"message\":\"Process deleted\"}";
        }
        process->name = stringField(body, "processName", process->name);
        process->arrival = numberField(body, "arrivalTime", process->arrival);
        process->burst = numberField(body, "burstTime", process->burst);
        process->priority = numberField(body, "priority", process->priority);
        if (process->arrival < 0 || process->burst <= 0 || process->priority < 0) {
            status = 400;
            return errorJson("Process values are invalid.");
        }
        return "{\"message\":\"Process updated\",\"process\":" + processJson(*process) + "}";
    }
    if (method == "POST" && path == "/api/resources") {
        const int id = numberField(body, "resourceId");
        if (id <= 0 || findResource(id) != nullptr) {
            status = 400;
            return errorJson("A unique positive resourceId is required.");
        }
        const int total = numberField(body, "totalInstances");
        if (total < 0) {
            status = 400;
            return errorJson("totalInstances cannot be negative.");
        }
        Resource resource{id, stringField(body, "resourceName", "Resource " + std::to_string(id)), total, total};
        resources.push_back(resource);
        history.push_back("Resource R" + std::to_string(id) + " created");
        return "{\"message\":\"Resource created\",\"resource\":" + resourceJson(resource) + "}";
    }
    if ((method == "PUT" || method == "DELETE") &&
        path.find("/api/resources/") == 0) {
        const int id = std::atoi(path.substr(15).c_str());
        Resource* resource = findResource(id);
        if (!resource) {
            status = 404;
            return errorJson("Resource not found.");
        }
        if (method == "DELETE") {
            resources.erase(std::remove_if(resources.begin(), resources.end(),
                                            [id](const Resource& item) { return item.id == id; }),
                            resources.end());
            allocations.erase(std::remove_if(allocations.begin(), allocations.end(),
                                             [id](const Allocation& item) {
                                                 return item.resourceId == id;
                                             }),
                             allocations.end());
            history.push_back("Resource R" + std::to_string(id) + " deleted");
            return "{\"message\":\"Resource deleted\"}";
        }
        const int allocated = resource->total - resource->available;
        const int total = numberField(body, "totalInstances", resource->total);
        if (total < allocated) {
            status = 409;
            return errorJson("Total instances cannot be below the allocated quantity.");
        }
        resource->name = stringField(body, "resourceName", resource->name);
        resource->total = total;
        resource->available = total - allocated;
        return "{\"message\":\"Resource updated\",\"resource\":" + resourceJson(*resource) + "}";
    }
    if (method == "POST" && (path == "/api/scheduling/fcfs" ||
                             path == "/api/scheduling/sjf" ||
                             path == "/api/scheduling/round-robin" ||
                             path == "/api/scheduling/priority")) {
        std::string algorithm = path == "/api/scheduling/fcfs" ? "FCFS" :
                                path == "/api/scheduling/sjf" ? "SJF" :
                                path == "/api/scheduling/priority" ? "PRIORITY" :
                                "ROUND_ROBIN";
        const int quantum = numberField(body, "quantum");
        if (algorithm == "ROUND_ROBIN" && quantum <= 0) {
            status = 400;
            return errorJson("A positive quantum is required for round-robin scheduling.");
        }
        std::vector<Process> scheduled;
        for (const Process& process : processes) {
            if (process.state != "TERMINATED")
                scheduled.push_back({process.id, process.name, process.arrival,
                                     process.burst, process.priority, process.state});
        }
        std::vector<::Process> algorithmProcesses;
        for (const Process& process : scheduled)
            algorithmProcesses.push_back({process.id, process.arrival, process.burst,
                                           process.priority});
        if (!runScheduling(algorithmProcesses, algorithm, quantum)) {
            status = 400;
            return errorJson("Unsupported scheduling algorithm.");
        }
        const SchedulingRun* run = latestSchedulingRun();
        if (!run) {
            status = 500;
            return errorJson("Scheduling did not produce a result.");
        }
        history.push_back(algorithm + " scheduling run completed");
        return schedulingJson(*run);
    }
    if (method == "POST" && (path == "/api/allocations/request" ||
                             path == "/api/allocations/release")) {
        const int processId = numberField(body, "processId");
        const int resourceId = numberField(body, "resourceId");
        const int quantity = numberField(body, "quantity");
        Resource* resource = findResource(resourceId);
        Process* process = findProcess(processId);
        if (!process || !resource || quantity <= 0) {
            status = 400;
            return errorJson("Process, resource, and a positive quantity are required.");
        }
        Allocation* allocation = findAllocation(processId, resourceId);
        if (path.find("/request") != std::string::npos) {
            if (resource->available < quantity) {
                if (!allocation)
                    allocations.push_back({processId, resourceId, 0, quantity});
                else
                    allocation->requested += quantity;
                history.push_back("P" + std::to_string(processId) + " queued R" +
                                  std::to_string(resourceId) + " request");
                status = 409;
                return errorJson("The requested resource quantity is unavailable; request queued.");
            }
            if (!allocation) {
                allocations.push_back({processId, resourceId, quantity});
            } else {
                allocation->units += quantity;
            }
            if (allocation)
                allocation->requested = 0;
            resource->available -= quantity;
            history.push_back("P" + std::to_string(processId) + " requested R" +
                              std::to_string(resourceId));
            return "{\"message\":\"Resource request granted\"}";
        }
        if (!allocation || allocation->units < quantity) {
            status = 409;
            return errorJson("The process does not hold that many instances.");
        }
        allocation->units -= quantity;
        resource->available += quantity;
        history.push_back("P" + std::to_string(processId) + " released R" +
                          std::to_string(resourceId));
        return "{\"message\":\"Resources released\"}";
    }
    if (method == "POST" && path == "/api/algorithms/banker") {
        return bankerJson();
    }
    if (method == "POST" && path == "/api/algorithms/deadlock") {
        std::vector<int> available;
        std::vector<std::vector<int>> allocationMatrix(processes.size(),
                                                         std::vector<int>(resources.size()));
        std::vector<std::vector<int>> requestMatrix(processes.size(),
                                                      std::vector<int>(resources.size()));
        for (const Resource& resource : resources)
            available.push_back(resource.available);
        for (const Allocation& allocation : allocations) {
            for (std::size_t p = 0; p < processes.size(); ++p)
                for (std::size_t r = 0; r < resources.size(); ++r)
                    if (processes[p].id == allocation.processId &&
                        resources[r].id == allocation.resourceId) {
                        allocationMatrix[p][r] = allocation.units;
                        requestMatrix[p][r] = allocation.requested;
                    }
        }
        const DeadlockReport report =
            runDeadlockDetection(available, allocationMatrix, requestMatrix);
        std::string deadlocked = "[";
        for (std::size_t i = 0; i < report.deadlockedProcesses.size(); ++i) {
            if (i) deadlocked += ',';
            deadlocked += "{\"processId\":" +
                          std::to_string(report.deadlockedProcesses[i]) + "}";
        }
        return "{\"deadlockDetected\":" +
               std::string(report.deadlockDetected ? "true" : "false") +
               ",\"message\":\"Detection completed by backend.\",\"deadlockedProcesses\":" +
               deadlocked + "]}";
    }

    status = 404;
    return errorJson("Route not found.");
}

void closeSocket(Socket socket) {
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

void serveClient(Socket client) {
    char buffer[8192];
    const int received = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        closeSocket(client);
        return;
    }
    buffer[received] = '\0';
    const std::string request(buffer, received);
    const std::size_t lineEnd = request.find("\r\n");
    const std::string requestLine = request.substr(0, lineEnd);
    std::istringstream line(requestLine);
    std::string method;
    std::string path;
    line >> method >> path;
    const std::size_t bodyStart = request.find("\r\n\r\n");
    const std::string body = bodyStart == std::string::npos ? "" : request.substr(bodyStart + 4);
    int status = 200;
    const std::string payload = handleRequest(method, path, body, status);
    const std::string response = "HTTP/1.1 " + std::to_string(status) +
        (status == 200 ? " OK" : status == 404 ? " Not Found" : " Bad Request") +
        "\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\nContent-Length: " +
        std::to_string(payload.size()) + "\r\nConnection: close\r\n\r\n" + payload;
    send(client, response.c_str(), static_cast<int>(response.size()), 0);
    closeSocket(client);
}

} // namespace

int main() {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        std::cerr << "Could not start Winsock.\n";
        return 1;
    }
#endif
    const Socket server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == InvalidSocket) {
        std::cerr << "Could not create HTTP server socket.\n";
        return 1;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8081);
    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
        listen(server, 16) < 0) {
        std::cerr << "Could not listen on http://localhost:8081.\n";
        closeSocket(server);
        return 1;
    }
    std::cout << "SafeState API listening on http://localhost:8081\n";
    while (true) {
        Socket client = accept(server, nullptr, nullptr);
        if (client != InvalidSocket)
            serveClient(client);
    }
}
