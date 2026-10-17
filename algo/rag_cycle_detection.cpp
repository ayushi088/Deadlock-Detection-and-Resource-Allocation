#include "rag_cycle_detection.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

RAGCycleDetection::RAGCycleDetection(int processes, int resources)
    : nodeCount(0), processCount(processes){
    if (processes < 0 || resources < 0)
        throw invalid_argument("Process and resource counts cannot be negative.");
    if (processes > numeric_limits<int>::max() - resources)
        throw length_error("Combined RAG node count is too large.");

    nodeCount = processes + resources;
    adjacencyList.resize(static_cast<size_t>(nodeCount));
    nodeName.resize(static_cast<size_t>(nodeCount));
}

void RAGCycleDetection::setNodeName(int index, string name){
    if (index < 0 || index >= nodeCount)
        throw out_of_range("RAG node index is out of range.");
    nodeName[index] = move(name);
}

void RAGCycleDetection::addEdge(int from, int to){
    if (from < 0 || from >= nodeCount || to < 0 || to >= nodeCount)
        throw out_of_range("RAG edge endpoint is out of range.");
    adjacencyList[from].push_back(to);
}

void RAGCycleDetection::dfs(int start, int node, vector<bool>& inPath, vector<int>& path,CycleDetectionResult& result) const
{
    for (int neighbor : adjacencyList[node]){
        if (neighbor == start){
            vector<int> cycle = path;
            cycle.push_back(start);

            vector<string> namedCycle;
            vector<int> processIds;
            vector<int> resourceIds;
            namedCycle.reserve(cycle.size());
            for (int vertex : cycle){
                namedCycle.push_back(nodeName[vertex]);
                if (vertex < processCount &&
                    std::find(processIds.begin(), processIds.end(), vertex) == processIds.end())
                    processIds.push_back(vertex);
                else if (vertex >= processCount &&
                         find(resourceIds.begin(), resourceIds.end(),
                              vertex - processCount) == resourceIds.end())
                    resourceIds.push_back(vertex - processCount);
            }

            result.cyclePaths.push_back(move(namedCycle));
            result.cycleProcessIds.push_back(move(processIds));
            result.cycleResourceIds.push_back(move(resourceIds));
        }
        else if (neighbor > start && !inPath[neighbor]){
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

    for (int start = 0; start < nodeCount; ++start){
        vector<bool> inPath(nodeCount, false);
        vector<int> path{start};
        inPath[start] = true;
        dfs(start, start, inPath, path, result);
    }

    result.hasCycle = !result.cyclePaths.empty();
    return result;
}
