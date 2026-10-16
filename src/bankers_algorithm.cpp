#include <iostream>
using namespace std;

int main() {
    int n, m;

    cout << "=== Banker's Algorithm ===\n";

    cout << "Enter number of processes: ";
    cin >> n;

    cout << "Enter number of resource types: ";
    cin >> m;

    if (n <= 0 || n > 100 || m <= 0 || m > 100) {
        cout << "Invalid input! Enter values between 1 and 100.\n";
        return 1;
    }

    int allocation[100][100];
    int maximum[100][100];
    int need[100][100];
    int available[100];
    bool finished[100] = {};

    int safeSequence[100];

    cout << "\nEnter Allocation Matrix (" << n << " x " << m << "):\n";
    for (int i = 0; i < n; i++) {
        cout << "Process P" << i << ": ";
        for (int j = 0; j < m; j++) {
            cin >> allocation[i][j];
            if (allocation[i][j] < 0) {
                cout << "Values cannot be negative.\n";
                return 1;
            }
        }
    }

    cout << "\nEnter Maximum Need Matrix (" << n << " x " << m << "):\n";
    for (int i = 0; i < n; i++) {
        cout << "Process P" << i << ": ";
        for (int j = 0; j < m; j++) {
            cin >> maximum[i][j];

            if (maximum[i][j] < allocation[i][j]) {
                cout << "Maximum cannot be less than allocation.\n";
                return 1;
            }
        }
    }

    cout << "\nEnter Available resources (" << m << " values):\n";
    for (int j = 0; j < m; j++) {
        cin >> available[j];
        if (available[j] < 0) {
            cout << "Available resources cannot be negative.\n";
            return 1;
        }
    }

    // Calculate Need = Maximum - Allocation
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }

    int work[100];
    for (int j = 0; j < m; j++) {
        work[j] = available[j];
    }

    int count = 0;

    while (count < n) {
        bool found = false;

        for (int i = 0; i < n; i++) {
            if (finished[i]) {
                continue;
            }

            int j;
            for (j = 0; j < m; j++) {
                if (need[i][j] > work[j]) {
                    break;
                }
            }

            if (j == m) {
                for (int k = 0; k < m; k++) {
                    work[k] += allocation[i][k];
                }

                safeSequence[count++] = i;
                finished[i] = true;
                found = true;
            }
        }

        if (!found) {
            cout << "\nSystem is NOT in a safe state.\n";
            return 0;
        }
    }

    cout << "\nSystem is in a SAFE state.\n";
    cout << "Safe sequence: ";

    for (int i = 0; i < n; i++) {
        cout << "P" << safeSequence[i];
        if (i < n - 1) {
            cout << " -> ";
        }
    }

    cout << '\n';
    return 0;
}
