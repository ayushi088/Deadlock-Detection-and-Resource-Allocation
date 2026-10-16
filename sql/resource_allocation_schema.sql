CREATE DATABASE IF NOT EXISTS deadlock_project;
USE deadlock_project;

CREATE TABLE IF NOT EXISTS processes (
    process_id INT PRIMARY KEY,
    process_name VARCHAR(20) NOT NULL
);

CREATE TABLE IF NOT EXISTS resources (
    resource_id INT PRIMARY KEY,
    resource_name VARCHAR(20) NOT NULL,
    total_units INT NOT NULL CHECK (total_units >= 0)
);

CREATE TABLE IF NOT EXISTS allocation (
    process_id INT NOT NULL,
    resource_id INT NOT NULL,
    allocated_units INT NOT NULL CHECK (allocated_units >= 0),
    PRIMARY KEY (process_id, resource_id),
    FOREIGN KEY (process_id) REFERENCES processes(process_id),
    FOREIGN KEY (resource_id) REFERENCES resources(resource_id)
);

CREATE TABLE IF NOT EXISTS maximum_needs (
    process_id INT NOT NULL,
    resource_id INT NOT NULL,
    max_units INT NOT NULL CHECK (max_units >= 0),
    PRIMARY KEY (process_id, resource_id),
    FOREIGN KEY (process_id) REFERENCES processes(process_id),
    FOREIGN KEY (resource_id) REFERENCES resources(resource_id)
);

CREATE TABLE IF NOT EXISTS available_resources (
    resource_id INT PRIMARY KEY,
    available_units INT NOT NULL CHECK (available_units >= 0),
    FOREIGN KEY (resource_id) REFERENCES resources(resource_id)
);

CREATE TABLE IF NOT EXISTS safety_results (
    result_id INT AUTO_INCREMENT PRIMARY KEY,
    system_status VARCHAR(20) NOT NULL,
    safe_sequence VARCHAR(255),
    checked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```
