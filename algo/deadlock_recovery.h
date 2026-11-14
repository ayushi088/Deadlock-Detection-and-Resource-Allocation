
#ifndef DEADLOCK_RECOVERY_H
#define DEADLOCK_RECOVERY_H

#include "deadlock_detector.h"

#include <string>
#include <vector>

using namespace std;

enum class RecoveryMethod
{
    Termination,
    Preemption
};

struct RecoveryResult
{
    bool recoverySuccessful = false;

    vector<int> available;
    vector<vector<int>> allocation;
    vector<vector<int>> request;

    vector<string> recoveryLog;

    DeadlockReport finalReport;
};

RecoveryResult recoverDeadlock(
    const vector<int>& available,
    const vector<vector<int>>& allocation,
    const vector<vector<int>>& request,
    RecoveryMethod method);

void printRecoveryReport(const RecoveryResult& result);

#endif
