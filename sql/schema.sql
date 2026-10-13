CREATE DATABASE IF NOT EXISTS process_scheduling;
USE process_scheduling;

CREATE TABLE IF NOT EXISTS PROCESS (
    process_id INT PRIMARY KEY,
    arrival_time INT NOT NULL,
    burst_time INT NOT NULL,
    priority INT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (arrival_time >= 0),
    CHECK (burst_time > 0),
    CHECK (priority >= 0)
);

CREATE TABLE IF NOT EXISTS SCHEDULING_RUN (
    run_id BIGINT PRIMARY KEY,
    algorithm VARCHAR(30) NOT NULL,
    time_quantum INT NOT NULL DEFAULT 0,
    started_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS PROCESS_STATE_HISTORY (
    history_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    run_id BIGINT NOT NULL,
    process_id INT NOT NULL,
    state ENUM('NEW', 'READY', 'RUNNING', 'TERMINATED') NOT NULL,
    state_time INT NOT NULL,
    changed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (run_id) REFERENCES SCHEDULING_RUN(run_id),
    FOREIGN KEY (process_id) REFERENCES PROCESS(process_id)
);

CREATE TABLE IF NOT EXISTS SCHEDULING_RESULT (
    run_id BIGINT NOT NULL,
    process_id INT NOT NULL,
    completion_time INT NOT NULL,
    turnaround_time INT NOT NULL,
    waiting_time INT NOT NULL,
    response_time INT NOT NULL,
    PRIMARY KEY (run_id, process_id),
    FOREIGN KEY (run_id) REFERENCES SCHEDULING_RUN(run_id),
    FOREIGN KEY (process_id) REFERENCES PROCESS(process_id)
);

CREATE OR REPLACE VIEW scheduling_report AS
SELECT r.run_id, r.algorithm, r.time_quantum, r.started_at,
       p.process_id, p.arrival_time, p.burst_time, p.priority,
       sr.completion_time, sr.turnaround_time, sr.waiting_time,
       sr.response_time
FROM SCHEDULING_RUN r
JOIN SCHEDULING_RESULT sr ON sr.run_id = r.run_id
JOIN PROCESS p ON p.process_id = sr.process_id;