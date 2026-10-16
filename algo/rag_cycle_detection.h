#ifndef RAG_CYCLE_DETECTION_H
#define RAG_CYCLE_DETECTION_H

#include <string>
#include <vector>

struct CycleDetectionResult
{
    bool hasCycle;
    std::vector<std::vector<std::string>> cyclePaths;
    std::vector<std::vector<int>> cycleProcessIds;
    std::vector<std::vector<int>> cycleResourceIds;
};

class RAGCycleDetection
{
private:
    int V;
    int processCount;
    std::vector<std::vector<int>> adj;
    std::vector<std::string> nodeName;

    void dfs(int start, int node, std::vector<bool>& inPath,
             std::vector<int>& path, CycleDetectionResult& result) const;

public:
    RAGCycleDetection(int processes, int resources);

    void setNodeName(int index, std::string name);
    void addEdge(int from, int to);
    CycleDetectionResult detectCycle() const;
};

#endif
