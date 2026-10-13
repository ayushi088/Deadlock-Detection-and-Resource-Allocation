-- Load process definitions before running the generated transaction:
INSERT INTO PROCESS (process_id, arrival_time, burst_time, priority)
VALUES
    (1, 0, 5, 2), (2, 1, 3, 1), (3, 2, 8, 3),
    (4, 3, 2, 2), (5, 4, 4, 1)
ON DUPLICATE KEY UPDATE arrival_time = VALUES(arrival_time),
    burst_time = VALUES(burst_time), priority = VALUES(priority);

-- Report for one scheduling run.
SELECT * FROM scheduling_report
WHERE run_id = ? ORDER BY process_id;

-- Average scheduling metrics by algorithm.
SELECT algorithm,
       ROUND(AVG(waiting_time), 2) AS average_waiting_time,
       ROUND(AVG(turnaround_time), 2) AS average_turnaround_time,
       ROUND(AVG(response_time), 2) AS average_response_time
FROM scheduling_report
GROUP BY algorithm
ORDER BY algorithm;

-- State timeline for a process in a run.
SELECT h.run_id, h.process_id, h.state, h.state_time, h.changed_at
FROM PROCESS_STATE_HISTORY h
WHERE h.run_id = ? AND h.process_id = ?
ORDER BY h.state_time, h.history_id;

-- Latest run summary.
SELECT run_id, algorithm, time_quantum, started_at
FROM SCHEDULING_RUN
ORDER BY started_at DESC, run_id DESC
LIMIT 1;