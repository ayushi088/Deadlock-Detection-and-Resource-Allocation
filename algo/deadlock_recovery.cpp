
#include "deadlock_recovery.h"

#include <algorithm>
#include <iostream>
#include <numeric>

RecoveryResult recoverDeadlock(
    const vector<int>& available,
    const vector<vector<int>>& allocation,
    const vector<vector<int>>& request,
    RecoveryMethod method)
{
    RecoveryResult result;

    // Work on copies so original input is not changed.
    result.available = available;
    result.allocation = allocation;
    result.request = request;

    DeadlockReport currentReport =
        runDeadlockDetection(
            result.available,
            result.allocation,
            result.request);

    if (!currentReport.deadlockDetected)
    {
        result.recoverySuccessful = true;
        result.finalReport = currentReport;

        result.recoveryLog.push_back(
            "No deadlock exists. Recovery is not required.");

        return result;
    }

    int processCount = result.allocation.size();
    int resourceCount = result.available.size();

    result.recoveryLog.push_back("Deadlock detected.");
    result.recoveryLog.push_back("Starting recovery.");

    // Continue until deadlock is resolved.
    while (currentReport.deadlockDetected)
    {
        int victim = -1;
        int maximumAllocation = -1;

        // Step 1: Select the victim process.
        for (int process : currentReport.deadlockedProcesses)
        {
            int allocated = 0;

            for (int j = 0; j < resourceCount; j++)
                allocated += result.allocation[process][j];

            if (allocated > maximumAllocation)
            {
                maximumAllocation = allocated;
                victim = process;
            }
        }

        if (victim == -1)
            break;

        result.recoveryLog.push_back(
            "Selected victim: P" + to_string(victim));

        // Step 2A: Process termination.
        if (method == RecoveryMethod::Termination)
        {
            for (int j = 0; j < resourceCount; j++)
            {
                result.available[j] +=
                    result.allocation[victim][j];

                result.allocation[victim][j] = 0;

                result.request[victim][j] = 0;
            }

            result.recoveryLog.push_back(
                "Process P" + to_string(victim) + " terminated.");

            result.recoveryLog.push_back(
                "All resources held by P" +
                to_string(victim) + " released.");
        }

        // Step 2B: Resource preemption.
        else
        {
            int selectedResource = -1;

            for (int j = 0; j < resourceCount; j++)
            {
                if (result.allocation[victim][j] > 0)
                {
                    selectedResource = j;
                    break;
                }
            }

            // If victim holds no resource, termination is required.
            if (selectedResource == -1)
            {
                result.recoveryLog.push_back(
                    "Victim holds no resources. Terminating P" +
                    to_string(victim) + " instead.");

                for (int j = 0; j < resourceCount; j++)
                    result.request[victim][j] = 0;
            }
            else
            {
                result.allocation[victim][selectedResource]--;
                result.available[selectedResource]++;

                result.recoveryLog.push_back(
                    "Preempted one instance of R" +
                    to_string(selectedResource) +
                    " from P" + to_string(victim) + ".");
            }
        }

        // Step 3: Run detection again after the state changes.
        currentReport =
            runDeadlockDetection(
                result.available,
                result.allocation,
                result.request);
    }

    result.finalReport = currentReport;

    result.recoverySuccessful =
        !currentReport.deadlockDetected;

    if (result.recoverySuccessful)
    {
        result.recoveryLog.push_back(
            "Deadlock successfully resolved.");
    }
    else
    {
        result.recoveryLog.push_back(
            "Deadlock could not be resolved.");
    }

    return result;
}

void printRecoveryReport(const RecoveryResult& result)
{
    cout << "\n===== DEADLOCK RECOVERY REPORT =====\n";

    for (const string& message : result.recoveryLog)
        cout << message << '\n';

    cout << "\nRecovery Status: "
         << (result.recoverySuccessful ? "SUCCESS" : "FAILED")
         << '\n';

    cout << "\nUpdated Available Resources:\n";

    for (int j = 0; j < result.available.size(); j++)
        cout << "R" << j << ": " << result.available[j] << '\n';

    cout << "\nFinal Detection Result:\n";

    printDeadlockReport(result.finalReport);

    cout << "====================================\n";
}
