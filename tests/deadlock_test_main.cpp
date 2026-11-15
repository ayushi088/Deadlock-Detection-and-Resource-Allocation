#include "../src/algo/deadlock_detector.h"
#include "../src/algo/deadlock_recovery.h"

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
    DeadlockReport report = runDeadlockDetection(
        available, allocation, request);

    printDeadlockReport(report);

    if (report.deadlockDetected){
        int choice;

        cout << "\n===== DEADLOCK RECOVERY =====\n";
        cout << "1. Process Termination\n";
        cout << "2. Resource Preemption\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice) || (choice != 1 && choice != 2)){
            cerr << "Invalid choice.\n";
            return 1;
        }

        RecoveryMethod method;

        if (choice == 1)
            method = RecoveryMethod::Termination;
        else
            method = RecoveryMethod::Preemption;

        RecoveryResult recovery = recoverDeadlock(
            available, allocation, request, method);

        printRecoveryReport(recovery);
    }
    else{
        cout << "\nNo deadlock detected. Recovery is not required.\n";
    }
}
catch (const exception& error){
    cerr << "Program failed: " << error.what() << '\n';
    return 1;
}

    return 0;
}
