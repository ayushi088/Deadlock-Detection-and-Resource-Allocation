CREATE TABLE IF NOT EXISTS processes (
    process_id INTEGER PRIMARY KEY,
    process_name VARCHAR(100) NOT NULL,
    arrival_time INTEGER NOT NULL DEFAULT 0 CHECK (arrival_time >= 0),
    burst_time INTEGER NOT NULL DEFAULT 1 CHECK (burst_time > 0),
    priority INTEGER NOT NULL DEFAULT 0 CHECK (priority >= 0),
    state VARCHAR(20) NOT NULL DEFAULT 'NEW'
        CHECK (state IN ('NEW', 'READY', 'RUNNING', 'WAITING', 'BLOCKED', 'TERMINATED')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS resources (
    resource_id INTEGER PRIMARY KEY,
    resource_name VARCHAR(100) NOT NULL UNIQUE,
    total_instances INTEGER NOT NULL CHECK (total_instances >= 0),
    available_instances INTEGER NOT NULL CHECK (
        available_instances >= 0 AND available_instances <= total_instances
    )
);

CREATE TABLE IF NOT EXISTS resource_allocations (
    process_id INTEGER NOT NULL REFERENCES processes(process_id) ON DELETE CASCADE,
    resource_id INTEGER NOT NULL REFERENCES resources(resource_id) ON DELETE CASCADE,
    allocated_instances INTEGER NOT NULL DEFAULT 0 CHECK (allocated_instances >= 0),
    requested_instances INTEGER NOT NULL DEFAULT 0 CHECK (requested_instances >= 0),
    PRIMARY KEY (process_id, resource_id)
);

CREATE TABLE IF NOT EXISTS scheduling_runs (
    run_id BIGSERIAL PRIMARY KEY,
    algorithm VARCHAR(30) NOT NULL,
    time_quantum INTEGER NOT NULL DEFAULT 0 CHECK (time_quantum >= 0),
    started_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS scheduling_results (
    run_id BIGINT NOT NULL REFERENCES scheduling_runs(run_id) ON DELETE CASCADE,
    process_id INTEGER NOT NULL REFERENCES processes(process_id) ON DELETE CASCADE,
    completion_time INTEGER NOT NULL,
    turnaround_time INTEGER NOT NULL,
    waiting_time INTEGER NOT NULL,
    response_time INTEGER NOT NULL,
    PRIMARY KEY (run_id, process_id)
);

CREATE TABLE IF NOT EXISTS process_state_history (
    history_id BIGSERIAL PRIMARY KEY,
    run_id BIGINT NOT NULL REFERENCES scheduling_runs(run_id) ON DELETE CASCADE,
    process_id INTEGER NOT NULL REFERENCES processes(process_id) ON DELETE CASCADE,
    state VARCHAR(20) NOT NULL,
    state_time INTEGER NOT NULL,
    changed_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS deadlock_events (
    event_id BIGSERIAL PRIMARY KEY,
    detected_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    detection_method VARCHAR(30) NOT NULL,
    deadlock_detected BOOLEAN NOT NULL,
    event_status VARCHAR(20) NOT NULL DEFAULT 'NO_DEADLOCK',
    notes TEXT
);

CREATE TABLE IF NOT EXISTS deadlock_processes (
    event_id BIGINT NOT NULL REFERENCES deadlock_events(event_id) ON DELETE CASCADE,
    process_id INTEGER NOT NULL REFERENCES processes(process_id) ON DELETE CASCADE,
    is_deadlocked BOOLEAN NOT NULL DEFAULT TRUE,
    is_victim BOOLEAN NOT NULL DEFAULT FALSE,
    PRIMARY KEY (event_id, process_id)
);

CREATE TABLE IF NOT EXISTS resource_dependencies (
    dependency_id BIGSERIAL PRIMARY KEY,
    event_id BIGINT NOT NULL REFERENCES deadlock_events(event_id) ON DELETE CASCADE,
    from_node VARCHAR(100) NOT NULL,
    to_node VARCHAR(100) NOT NULL,
    edge_type VARCHAR(20) NOT NULL CHECK (edge_type IN ('REQUEST', 'ALLOCATION'))
);

CREATE TABLE IF NOT EXISTS recovery_history (
    recovery_id BIGSERIAL PRIMARY KEY,
    event_id BIGINT NOT NULL REFERENCES deadlock_events(event_id) ON DELETE CASCADE,
    victim_process_id INTEGER REFERENCES processes(process_id) ON DELETE SET NULL,
    recovery_method VARCHAR(30) NOT NULL,
    resources_released JSONB NOT NULL DEFAULT '[]'::jsonb,
    recovery_status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    recovery_details TEXT,
    recovered_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_process_state ON processes(state);
CREATE INDEX IF NOT EXISTS idx_deadlock_events_detected_at
    ON deadlock_events(detected_at DESC);
CREATE INDEX IF NOT EXISTS idx_scheduling_runs_started_at
    ON scheduling_runs(started_at DESC);

CREATE OR REPLACE VIEW scheduling_report AS
SELECT r.run_id, r.algorithm, r.time_quantum, r.started_at,
       p.process_id, p.arrival_time, p.burst_time, p.priority,
       sr.completion_time, sr.turnaround_time, sr.waiting_time,
       sr.response_time
FROM scheduling_runs r
JOIN scheduling_results sr ON sr.run_id = r.run_id
JOIN processes p ON p.process_id = sr.process_id;
