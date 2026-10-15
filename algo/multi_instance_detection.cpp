#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

class MultipleInstanceDetection{
private:
    int P, R;
    vector<int> available;
    vector<vector<int>> allocation;
    vector<vector<int>> request;

public:
    MultipleInstanceDetection(int processes, int resources){
        P = processes;
        R = resources;

        available.resize(R);
        allocation.assign(P, vector<int>(R, 0));
        request.assign(P, vector<int>(R, 0));
    }

    void inputData(){
        cout << "\nEnter available instances for each resource:\n";
        for(int j = 0; j < R; j++){
            cout << "Available R" << j << ": ";
            cin >> available[j];
        }

        cout << "\nEnter allocation matrix:\n";
        cout << "(Instances of each resource held by each process)\n";

        for(int i = 0; i < P; i++){
            cout << "Process P" << i << ":\n";
            for(int j = 0; j < R; j++){
                cout << "  Allocation of R" << j << ": ";
                cin >> allocation[i][j];
            }
        }

        cout << "\nEnter request matrix:\n";
        cout << "(Additional instances each process currently needs)\n";

        for(int i = 0; i < P; i++){
            cout << "Process P" << i << ":\n";
            for(int j = 0; j < R; j++){
                cout << "  Request for R" << j << ": ";
                cin >> request[i][j];
            }
        }
    }

    void detectDeadlock() {
        vector<int> work = available;
        vector<bool> finish(P, false);

        // A process holding no resources can finish without
        // waiting for any resource to be released.
        for(int i = 0; i < P; i++){
            bool holdingResources = false;
            for(int j = 0; j < R; j++){
                if (allocation[i][j] > 0) {
                    holdingResources = true;
                    break;
                }
            }

            if(!holdingResources){
                finish[i] = true;
            }
        }

        bool progress = true;

        while(progress){
            progress = false;

            for(int i = 0; i < P; i++){
                if (finish[i]) {
                    continue;
                }

                bool canFinish = true;

                for(int j = 0; j < R; j++){
                    if(request[i][j] > work[j]){
                        canFinish = false;
                        break;
                    }
                }

                if(canFinish){
                    for(int j = 0; j < R; j++){
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    progress = true;
                }
            }
        }

        cout << "\n===== MULTIPLE-INSTANCE DETECTION =====\n";

        bool deadlockFound = false;

        for(int i = 0; i < P; i++){
            if(!finish[i]){
                if(!deadlockFound){
                    cout << "Deadlock detected!\n";
                    cout << "Deadlocked processes: ";
                }
                cout << "P" << i << " ";
                deadlockFound = true;
            }
        }

        if (!deadlockFound) {
            cout << "No deadlock detected.\n";
        }
        cout << "\n=======================================\n";
    }
};

int main(){
    int P, R;

    cout << "===== MULTIPLE-INSTANCE DEADLOCK DETECTION =====\n";
    cout << "Enter number of processes: ";
    cin >> P;

    cout << "Enter number of resource types: ";
    cin >> R;

    if(P < 1 || R < 1){
        cout << "Invalid input. Counts must be positive.\n";
        return 1;
    }

    MultipleInstanceDetection detector(P, R);
    detector.inputData();
    detector.detectDeadlock();

    return 0;
}
