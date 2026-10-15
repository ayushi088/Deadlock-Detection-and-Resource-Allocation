
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

class RAGCycleDetection {
private:
    int V;
    vector<vector<int>> adj;
    vector<string> nodeName;

    bool dfs(int node, vector<int>& visited, vector<int>& pathVisited, vector<int>& parent, int& cycleStart, int& cycleEnd){
        visited[node] = 1;
        pathVisited[node] = 1;

        for(int neighbor : adj[node]){
            if(!visited[neighbor]){
                parent[neighbor] = node;

                if(dfs(neighbor, visited, pathVisited, parent, cycleStart, cycleEnd)){
                    return true;
                }
            }
            else if(pathVisited[neighbor]){
                cycleStart = neighbor;
                cycleEnd = node;
                return true;
            }
        }

        pathVisited[node] = 0;
        return false;
    }

public:
    RAGCycleDetection(int vertices){
        V = vertices;
        adj.resize(V);
        nodeName.resize(V);
    }

    void setNodeName(int index, string name){
        nodeName[index] = name;
    }

    void addEdge(int from, int to){
        adj[from].push_back(to);
    }

    void detectCycle(){
        vector<int> visited(V, 0);
        vector<int> pathVisited(V, 0);
        vector<int> parent(V, -1);

        int cycleStart = -1;
        int cycleEnd = -1;

        for(int i = 0; i < V; i++){
            if(!visited[i]){
                if(dfs(i, visited, pathVisited, parent, cycleStart, cycleEnd)){
                    vector<int> cycle;
                    cycle.push_back(cycleStart);

                    for(int v = cycleEnd; v != cycleStart; v = parent[v]){
                        cycle.push_back(v);
                    }

                    cycle.push_back(cycleStart);
                    reverse(cycle.begin(), cycle.end());

                    cout << "\n========== RESULT ==========\n";
                    cout << "Cycle detected!\n";
                    cout << "Cycle path: ";

                    for(int i = 0; i < cycle.size(); i++){
                        cout << nodeName[cycle[i]];
                        if (i < cycle.size() - 1)
                            cout << " -> ";
                    }

                    cout << "\n============================\n";
                    return;
                }
            }
        }

        cout << "\nNo cycle detected in the graph.\n";
    }
};

int main() {
    int P, R;

    cout << "===== RAG CYCLE DETECTION =====\n";
    cout << "Enter number of processes: ";
    cin >> P;

    cout << "Enter number of resource types: ";
    cin >> R;

    if(P < 1 || R < 1){
        cout << "Invalid input. Counts must be positive.\n";
        return 1;
    }

    int V = P + R;
    RAGCycleDetection rag(V);

    // Processes get indices 0 to P-1.
    // Resources get indices P to P+R-1.

    for(int i = 0; i < P; i++){
        rag.setNodeName(i, "P" + to_string(i));
    }

    for(int i = 0; i < R; i++){
        rag.setNodeName(P + i, "R" + to_string(i));
    }

    cout << "\nProcesses: ";
    for(int i = 0; i < P; i++)
        cout << "P" << i << " ";

    cout << "\nResources: ";
    for(int i = 0; i < R; i++)
        cout << "R" << i << " ";

    cout << "\n\nEnter allocation edges (Resource -> Process).\n";
    int A;
    cout << "Number of allocation edges: ";
    cin >> A;

    for(int i = 0; i < A; i++){
        int resource, process;
        cout << "Allocation " << i + 1 << " (resource index, process index): ";
        cin >> resource >> process;

        if(resource < 0 || resource >= R || process < 0 || process >= P){
            cout << "Invalid indices. Try this edge again.\n";
            i--;
            continue;
        }

        rag.addEdge(P + resource, process);
    }

    cout << "\nEnter request edges (Process -> Resource).\n";
    int Q;
    cout << "Number of request edges: ";
    cin >> Q;

    for(int i = 0; i < Q; i++){
        int process, resource;
        cout << "Request " << i + 1 << " (process index, resource index): ";
        cin >> process >> resource;

        if(process < 0 || process >= P || resource < 0 || resource >= R){
            cout << "Invalid indices. Try this edge again.\n";
            i--;
            continue;
        }

        rag.addEdge(process, P + resource);
    }

    cout << "\nChecking graph for cycles...\n";
    rag.detectCycle();

    return 0;
}
