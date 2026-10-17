#ifndef RAG_CYCLE_DETECTION_H
#define RAG_CYCLE_DETECTION_H

#include <string>
#include <vector>

using namespace std;

struct CycleDetectionResult{
    bool hasCycle;
    vector<vector<string>> cyclePaths;
    vector<vector<int>> cycleProcessIds;
    vector<vector<int>> cycleResourceIds;
};

class RAGCycleDetection{
private:
    int nodeCount;
    int processCount;
    vector<vector<int>> adjacencyList;
    vector<string> nodeName;

    void dfs(int start, int node, vector<bool>& inPath,
             vector<int>& path, CycleDetectionResult& result) const;

public:
    RAGCycleDetection(int processes, int resources);

    void setNodeName(int index, string name);
    void addEdge(int from, int to);
    CycleDetectionResult detectCycle() const;
};

#endif
