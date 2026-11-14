USE process_scheduling;

CREATE TABLE IF NOT EXISTS MEMORY_FRAME (
    frame_id INT PRIMARY KEY,
    frame_size INT NOT NULL,
    status ENUM('FREE', 'ALLOCATED') NOT NULL DEFAULT 'FREE',
    process_id INT NULL,
    page_number INT NULL,
    CHECK (frame_size > 0),
    FOREIGN KEY (process_id) REFERENCES PROCESS(process_id)
);

CREATE TABLE IF NOT EXISTS PAGE_TABLE (
    process_id INT NOT NULL,
    page_number INT NOT NULL,
    frame_id INT NOT NULL,
    PRIMARY KEY (process_id, page_number),
    UNIQUE (frame_id),
    FOREIGN KEY (process_id) REFERENCES PROCESS(process_id),
    FOREIGN KEY (frame_id) REFERENCES MEMORY_FRAME(frame_id)
);

CREATE TABLE IF NOT EXISTS MEMORY_HISTORY (
    history_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    process_id INT NOT NULL,
    operation ENUM('ALLOCATE', 'DEALLOCATE') NOT NULL,
    requested_size INT NOT NULL,
    result ENUM('SUCCESS', 'FAILED') NOT NULL,
    event_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (process_id) REFERENCES PROCESS(process_id)
);

CREATE TABLE IF NOT EXISTS SYSTEM_METRICS (
    metric_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    total_memory INT NOT NULL,
    allocated_memory INT NOT NULL,
    free_memory INT NOT NULL,
    internal_fragmentation INT NOT NULL DEFAULT 0,
    utilization_percent DECIMAL(5,2) NOT NULL,
    recorded_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);