#include "rag_cycle_detection.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

RAGCycleDetection::RAGCycleDetection(int processes, int resources)
    : V(0), processCount(processes)
{
    if (processes < 0 || resources < 0)
        throw std::invalid_argument("Process and resource counts cannot be negative.");
    if (processes > std::numeric_limits<int>::max() - resources)
        throw std::length_error("Combined RAG node count is too large.");

    V = processes + resources;
    adj.resize(static_cast<std::size_t>(V));
    nodeName.resize(static_cast<std::size_t>(V));
}

void RAGCycleDetection::setNodeName(int index, std::string name)
{
    if (index < 0 || index >= V)
        throw std::out_of_range("RAG node index is out of range.");
    nodeName[index] = std::move(name);
}

void RAGCycleDetection::addEdge(int from, int to)
{
    if (from < 0 || from >= V || to < 0 || to >= V)
        throw std::out_of_range("RAG edge endpoint is out of range.");
    adj[from].push_back(to);
}

void RAGCycleDetection::dfs(int start, int node, std::vector<bool>& inPath,
                            std::vector<int>& path,
                            CycleDetectionResult& result) const
{
    for (int neighbor : adj[node])
    {
        if (neighbor == start)
        {
            std::vector<int> cycle = path;
            cycle.push_back(start);

            std::vector<std::string> namedCycle;
            std::vector<int> processIds;
            std::vector<int> resourceIds;
            namedCycle.reserve(cycle.size());
            for (int vertex : cycle)
            {
                namedCycle.push_back(nodeName[vertex]);
                if (vertex < processCount &&
                    std::find(processIds.begin(), processIds.end(), vertex) == processIds.end())
                    processIds.push_back(vertex);
                else if (vertex >= processCount &&
                         std::find(resourceIds.begin(), resourceIds.end(),
                                   vertex - processCount) == resourceIds.end())
                    resourceIds.push_back(vertex - processCount);
            }

            result.cyclePaths.push_back(std::move(namedCycle));
            result.cycleProcessIds.push_back(std::move(processIds));
            result.cycleResourceIds.push_back(std::move(resourceIds));
        }
        else if (neighbor > start && !inPath[neighbor])
        {
            inPath[neighbor] = true;
            path.push_back(neighbor);
            dfs(start, neighbor, inPath, path, result);
            path.pop_back();
            inPath[neighbor] = false;
        }
    }
}

CycleDetectionResult RAGCycleDetection::detectCycle() const
{
    CycleDetectionResult result{false, {}, {}, {}};

    for (int start = 0; start < V; ++start)
    {
        std::vector<bool> inPath(V, false);
        std::vector<int> path{start};
        inPath[start] = true;
        dfs(start, start, inPath, path, result);
    }

    result.hasCycle = !result.cyclePaths.empty();
    return result;
}
