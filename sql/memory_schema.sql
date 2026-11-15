CREATE TABLE IF NOT EXISTS memory_frames (
    frame_id INTEGER PRIMARY KEY,
    frame_size INTEGER NOT NULL CHECK (frame_size > 0),
    status VARCHAR(20) NOT NULL DEFAULT 'FREE'
        CHECK (status IN ('FREE', 'ALLOCATED')),
    process_id INTEGER REFERENCES processes(process_id) ON DELETE CASCADE,
    page_number INTEGER
);

CREATE TABLE IF NOT EXISTS page_table (
    process_id INTEGER NOT NULL REFERENCES processes(process_id) ON DELETE CASCADE,
    page_number INTEGER NOT NULL,
    frame_id INTEGER NOT NULL UNIQUE REFERENCES memory_frames(frame_id),
    PRIMARY KEY (process_id, page_number)
);

CREATE TABLE IF NOT EXISTS memory_history (
    history_id BIGSERIAL PRIMARY KEY,
    process_id INTEGER NOT NULL REFERENCES processes(process_id) ON DELETE CASCADE,
    operation VARCHAR(20) NOT NULL CHECK (operation IN ('ALLOCATE', 'DEALLOCATE')),
    requested_size INTEGER NOT NULL,
    result VARCHAR(20) NOT NULL CHECK (result IN ('SUCCESS', 'FAILED')),
    event_time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS system_metrics (
    metric_id BIGSERIAL PRIMARY KEY,
    total_memory INTEGER NOT NULL,
    allocated_memory INTEGER NOT NULL,
    free_memory INTEGER NOT NULL,
    internal_fragmentation INTEGER NOT NULL DEFAULT 0,
    utilization_percent NUMERIC(5, 2) NOT NULL,
    recorded_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS process_memory (
    process_id INTEGER PRIMARY KEY REFERENCES processes(process_id) ON DELETE CASCADE,
    requested_size INTEGER NOT NULL CHECK (requested_size > 0),
    allocated_pages INTEGER NOT NULL CHECK (allocated_pages > 0),
    allocation_time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);
