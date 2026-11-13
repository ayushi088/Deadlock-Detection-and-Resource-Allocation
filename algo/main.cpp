#include "deadlock_detector.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace{
bool readNonNegativeCount(const char* prompt, int& value){
    cout << prompt;
    if (!(cin >> value) || value < 0)
        return false;
    return true;
}
}

int main(){
    int processCount = 0;
    int resourceCount = 0;

    cout << "===== INTEGRATED DEADLOCK DETECTOR =====\n";
    if (!readNonNegativeCount("Enter number of processes: ", processCount) ||
        !readNonNegativeCount("Enter number of resource types: ", resourceCount) ||
        processCount < 1 || resourceCount < 1){
        cerr << "Process and resource counts must be positive integers.\n";
        return 1;
    }

    vector<int> available(resourceCount);
    vector<vector<int>> allocation(processCount, vector<int>(resourceCount));
    vector<vector<int>> request(processCount, vector<int>(resourceCount));

    cout << "\nEnter available instances for each resource type:\n";
    for (int resource = 0; resource < resourceCount; ++resource){
        if (!readNonNegativeCount(("Available R" + to_string(resource) + ": ").c_str(),
                                  available[resource])){
            cerr << "Resource counts must be non-negative integers.\n";
            return 1;
        }
    }

    cout << "\nEnter allocation matrix (one row per process):\n";
    for (int process = 0; process < processCount; ++process){
        cout << "P" << process << ": ";
        for (int resource = 0; resource < resourceCount; ++resource){
            if (!(cin >> allocation[process][resource]) || allocation[process][resource] < 0){
                cerr << "Allocation counts must be non-negative integers.\n";
                return 1;
            }
        }
    }

    cout << "\nEnter request matrix (one row per process):\n";
    for (int process = 0; process < processCount; ++process){
        cout << "P" << process << ": ";
        for (int resource = 0; resource < resourceCount; ++resource){
            if (!(cin >> request[process][resource]) || request[process][resource] < 0){
                cerr << "Request counts must be non-negative integers.\n";
                return 1;
            }
        }
    }

    try{
        printDeadlockReport(runDeadlockDetection(available, allocation, request));
    }
    catch (const exception& error){
        cerr << "Detection failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
