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


--Resource types and their instance counts
CREATE TABLE IF NOT EXISTS RESOURCE_TYPE (
    resource_id INT PRIMARY KEY,
    resource_name VARCHAR(50) NOT NULL UNIQUE,
    total_instances INT NOT NULL,
    available_instances INT NOT NULL,
    CHECK (total_instances >= 0),
    CHECK (available_instances >= 0),
    CHECK (available_instances <= total_instances)
);

--Resource allocation matrix
CREATE TABLE IF NOT EXISTS RESOURCE_ALLOCATION (
    process_id INT NOT NULL,
    resource_id INT NOT NULL,
    allocated_instances INT NOT NULL DEFAULT 0,
    PRIMARY KEY (process_id, resource_id),
    FOREIGN KEY (process_id)
        REFERENCES PROCESS(process_id),
    FOREIGN KEY (resource_id)
        REFERENCES RESOURCE_TYPE(resource_id),
    CHECK (allocated_instances >= 0)
);

--Outstanding resource requests
CREATE TABLE IF NOT EXISTS RESOURCE_REQUEST (
    process_id INT NOT NULL,
    resource_id INT NOT NULL,
    requested_instances INT NOT NULL DEFAULT 0,
    PRIMARY KEY (process_id, resource_id),
    FOREIGN KEY (process_id)
        REFERENCES PROCESS(process_id),
    FOREIGN KEY (resource_id)
        REFERENCES RESOURCE_TYPE(resource_id),
    CHECK (requested_instances >= 0)
);

--Record every deadlock detection run
CREATE TABLE IF NOT EXISTS DEADLOCK_EVENT (
    event_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    run_id BIGINT NULL,
    detected_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    detection_method ENUM(
        'RAG',
        'MULTI_INSTANCE',
        'INTEGRATED'
    ) NOT NULL,
    deadlock_detected BOOLEAN NOT NULL,
    event_status ENUM(
        'NO_DEADLOCK',
        'DETECTED',
        'RESOLVED',
        'UNRESOLVED'
    ) NOT NULL DEFAULT 'NO_DEADLOCK',
    notes TEXT,
    FOREIGN KEY (run_id)
        REFERENCES SCHEDULING_RUN(run_id)
);

--Processes involved in each detection event
CREATE TABLE IF NOT EXISTS DEADLOCK_PROCESS (
    event_id BIGINT NOT NULL,
    process_id INT NOT NULL,
    is_deadlocked BOOLEAN NOT NULL DEFAULT TRUE,
    is_victim BOOLEAN NOT NULL DEFAULT FALSE,
    PRIMARY KEY (event_id, process_id),
    FOREIGN KEY (event_id)
        REFERENCES DEADLOCK_EVENT(event_id),
    FOREIGN KEY (process_id)
        REFERENCES PROCESS(process_id)
);

--RAG edges for graph visualization
CREATE TABLE IF NOT EXISTS RESOURCE_DEPENDENCY (
    dependency_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    event_id BIGINT NOT NULL,
    from_node VARCHAR(30) NOT NULL,
    to_node VARCHAR(30) NOT NULL,
    edge_type ENUM('REQUEST', 'ALLOCATION') NOT NULL,
    FOREIGN KEY (event_id)
        REFERENCES DEADLOCK_EVENT(event_id)
        ON DELETE CASCADE
);

--Recovery attempts and their outcomes
CREATE TABLE IF NOT EXISTS RECOVERY_LOG (
    recovery_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    event_id BIGINT NOT NULL,
    victim_process_id INT NULL,
    recovery_method ENUM(
        'PROCESS_TERMINATION',
        'RESOURCE_PREEMPTION',
        'ROLLBACK'
    ) NOT NULL,
    resources_released TEXT,
    recovery_status ENUM(
        'PENDING',
        'SUCCESS',
        'FAILED'
    ) NOT NULL DEFAULT 'PENDING',
    recovery_details TEXT,
    recovered_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (event_id)
        REFERENCES DEADLOCK_EVENT(event_id),
    FOREIGN KEY (victim_process_id)
        REFERENCES PROCESS(process_id)
);

--Indexes for reports and history
CREATE INDEX idx_deadlock_detected_at
    ON DEADLOCK_EVENT(detected_at);

CREATE INDEX idx_deadlock_status
    ON DEADLOCK_EVENT(event_status);

CREATE INDEX idx_recovery_event
    ON RECOVERY_LOG(event_id);
