#include "postgres_store.h"

#include <cstdlib>
#include <sstream>

namespace {

std::string envOrEmpty(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? "" : value;
}

std::string connectionString() {
    const std::string configured = envOrEmpty("SAFESTATE_DATABASE_URL");
    if (!configured.empty())
        return configured;
    const std::string host = envOrEmpty("PGHOST");
    const std::string port = envOrEmpty("PGPORT");
    const std::string database = envOrEmpty("PGDATABASE");
    const std::string user = envOrEmpty("PGUSER");
    const std::string password = envOrEmpty("PGPASSWORD");
    std::ostringstream result;
    if (!host.empty()) result << "host=" << host << ' ';
    if (!port.empty()) result << "port=" << port << ' ';
    if (!database.empty()) result << "dbname=" << database << ' ';
    if (!user.empty()) result << "user=" << user << ' ';
    if (!password.empty()) result << "password=" << password;
    return result.str();
}

std::string resultError(PGresult* result) {
    const char* message = PQresultErrorMessage(result);
    return message == nullptr ? "PostgreSQL operation failed." : message;
}

bool successful(PGresult* result) {
    const ExecStatusType status = PQresultStatus(result);
    return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
}

int value(PGresult* result, int row, int column) {
    return std::atoi(PQgetvalue(result, row, column));
}

} // namespace

PostgresStore::PostgresStore() : connection_(nullptr) {}

PostgresStore::~PostgresStore() {
    if (connection_ != nullptr)
        PQfinish(connection_);
}

bool PostgresStore::connect(std::string& error) {
    const std::string configured = connectionString();
    if (configured.empty()) {
        error = "SAFESTATE_DATABASE_URL or PostgreSQL PG* variables are required.";
        return false;
    }
    connection_ = PQconnectdb(configured.c_str());
    if (connection_ == nullptr || PQstatus(connection_) != CONNECTION_OK) {
        error = connection_ == nullptr ? "Could not allocate PostgreSQL connection."
                                       : PQerrorMessage(connection_);
        return false;
    }
    return true;
}

bool PostgresStore::execute(const char* sql, int parameterCount,
                            const char* const* parameters, std::string& error) {
    PGresult* result = PQexecParams(connection_, sql, parameterCount, nullptr,
                                    parameters, nullptr, nullptr, 0);
    if (result == nullptr || !successful(result)) {
        error = result == nullptr ? PQerrorMessage(connection_) : resultError(result);
        if (result != nullptr) PQclear(result);
        return false;
    }
    PQclear(result);
    return true;
}

bool PostgresStore::begin(std::string& error) {
    return execute("BEGIN", 0, nullptr, error);
}

bool PostgresStore::rollback() {
    PGresult* result = PQexec(connection_, "ROLLBACK");
    if (result == nullptr) return false;
    PQclear(result);
    return true;
}

bool PostgresStore::commit(std::string& error) {
    return execute("COMMIT", 0, nullptr, error);
}

bool PostgresStore::load(std::vector<DbProcess>& processes,
                         std::vector<DbResource>& resources,
                         std::vector<DbAllocation>& allocations,
                         std::string& error) {
    PGresult* result = PQexec(connection_,
        "SELECT process_id, process_name, status FROM processes ORDER BY process_id");
    if (result == nullptr || PQresultStatus(result) != PGRES_TUPLES_OK) {
        error = result == nullptr ? PQerrorMessage(connection_) : resultError(result);
        if (result != nullptr) PQclear(result);
        return false;
    }
    processes.clear();
    for (int row = 0; row < PQntuples(result); ++row)
        processes.push_back({value(result, row, 0), PQgetvalue(result, row, 1),
                             PQgetvalue(result, row, 2)});
    PQclear(result);

    result = PQexec(connection_,
        "SELECT resource_id, resource_name, total_instances, available_instances "
        "FROM resources ORDER BY resource_id");
    if (result == nullptr || PQresultStatus(result) != PGRES_TUPLES_OK) {
        error = result == nullptr ? PQerrorMessage(connection_) : resultError(result);
        if (result != nullptr) PQclear(result);
        return false;
    }
    resources.clear();
    for (int row = 0; row < PQntuples(result); ++row)
        resources.push_back({value(result, row, 0), PQgetvalue(result, row, 1),
                             value(result, row, 2), value(result, row, 3)});
    PQclear(result);

    result = PQexec(connection_,
        "SELECT process_id, resource_id, allocated_instances, requested_instances "
        "FROM resource_allocation ORDER BY process_id, resource_id");
    if (result == nullptr || PQresultStatus(result) != PGRES_TUPLES_OK) {
        error = result == nullptr ? PQerrorMessage(connection_) : resultError(result);
        if (result != nullptr) PQclear(result);
        return false;
    }
    allocations.clear();
    for (int row = 0; row < PQntuples(result); ++row)
        allocations.push_back({value(result, row, 0), value(result, row, 1),
                               value(result, row, 2), value(result, row, 3)});
    PQclear(result);
    return true;
}

bool PostgresStore::createProcess(const DbProcess& process, std::string& error) {
    const std::string id = std::to_string(process.id);
    const char* parameters[] = {id.c_str(), process.name.c_str(), process.state.c_str()};
    return execute("INSERT INTO processes (process_id, process_name, status) "
                   "VALUES ($1, $2, $3)", 3, parameters, error);
}

bool PostgresStore::updateProcess(const DbProcess& process, std::string& error) {
    const std::string id = std::to_string(process.id);
    const char* parameters[] = {process.name.c_str(), process.state.c_str(), id.c_str()};
    return execute("UPDATE processes SET process_name = $1, status = $2 "
                   "WHERE process_id = $3", 3, parameters, error);
}

bool PostgresStore::deleteProcess(int processId, std::string& error) {
    const std::string id = std::to_string(processId);
    const char* parameters[] = {id.c_str()};
    return execute("DELETE FROM processes WHERE process_id = $1", 1, parameters, error);
}

bool PostgresStore::createResource(const DbResource& resource, std::string& error) {
    const std::string id = std::to_string(resource.id);
    const std::string total = std::to_string(resource.total);
    const std::string available = std::to_string(resource.available);
    const char* parameters[] = {id.c_str(), resource.name.c_str(),
                                total.c_str(), available.c_str()};
    return execute("INSERT INTO resources "
                   "(resource_id, resource_name, total_instances, available_instances) "
                   "VALUES ($1, $2, $3, $4)", 4, parameters, error);
}

bool PostgresStore::updateResource(const DbResource& resource, std::string& error) {
    const std::string id = std::to_string(resource.id);
    const std::string total = std::to_string(resource.total);
    const std::string available = std::to_string(resource.available);
    const char* parameters[] = {resource.name.c_str(), total.c_str(),
                                available.c_str(), id.c_str()};
    return execute("UPDATE resources SET resource_name = $1, total_instances = $2, "
                   "available_instances = $3 WHERE resource_id = $4",
                   4, parameters, error);
}

bool PostgresStore::deleteResource(int resourceId, std::string& error) {
    const std::string id = std::to_string(resourceId);
    const char* parameters[] = {id.c_str()};
    return execute("DELETE FROM resources WHERE resource_id = $1", 1, parameters, error);
}

bool PostgresStore::requestResource(int processId, int resourceId, int quantity,
                                    bool& granted, std::string& error) {
    granted = false;
    if (!begin(error)) return false;
    const std::string process = std::to_string(processId);
    const std::string resource = std::to_string(resourceId);
    const std::string amount = std::to_string(quantity);
    const char* checkParameters[] = {resource.c_str()};
    PGresult* check = PQexecParams(connection_,
        "SELECT available_instances FROM resources WHERE resource_id = $1 FOR UPDATE",
        1, nullptr, checkParameters, nullptr, nullptr, 0);
    if (check == nullptr || PQresultStatus(check) != PGRES_TUPLES_OK ||
        PQntuples(check) != 1) {
        error = check == nullptr ? PQerrorMessage(connection_) : resultError(check);
        if (check != nullptr) PQclear(check);
        rollback();
        return false;
    }
    const int available = value(check, 0, 0);
    PQclear(check);
    const char* allocationParameters[] = {process.c_str(), resource.c_str()};
    PGresult* existing = PQexecParams(connection_,
        "SELECT id, allocated_instances, requested_instances FROM resource_allocation "
        "WHERE process_id = $1 AND resource_id = $2 FOR UPDATE",
        2, nullptr, allocationParameters, nullptr, nullptr, 0);
    if (existing == nullptr || PQresultStatus(existing) != PGRES_TUPLES_OK) {
        error = existing == nullptr ? PQerrorMessage(connection_) : resultError(existing);
        if (existing != nullptr) PQclear(existing);
        rollback();
        return false;
    }
    const bool hasAllocation = PQntuples(existing) > 0;
    const std::string rowId = hasAllocation ? PQgetvalue(existing, 0, 0) : "";
    const int allocated = hasAllocation ? value(existing, 0, 1) : 0;
    const int requested = hasAllocation ? value(existing, 0, 2) : 0;
    PQclear(existing);
    if (available < quantity) {
        const std::string nextRequested = std::to_string(requested + quantity);
        if (hasAllocation) {
            const char* parameters[] = {nextRequested.c_str(), rowId.c_str()};
            if (!execute("UPDATE resource_allocation SET requested_instances = $1 "
                         "WHERE id = $2", 2, parameters, error)) {
                rollback();
                return false;
            }
        } else {
            const char* parameters[] = {process.c_str(), resource.c_str(),
                                        nextRequested.c_str()};
            if (!execute("INSERT INTO resource_allocation "
                         "(process_id, resource_id, allocated_instances, requested_instances) "
                         "VALUES ($1, $2, 0, $3)", 3, parameters, error)) {
                rollback();
                return false;
            }
        }
    } else {
        const std::string nextAllocated = std::to_string(allocated + quantity);
        const char* updateParameters[] = {nextAllocated.c_str(), rowId.c_str()};
        if (hasAllocation) {
            if (!execute("UPDATE resource_allocation SET allocated_instances = $1 "
                         "WHERE id = $2", 2, updateParameters, error)) {
                rollback();
                return false;
            }
        } else {
            const char* parameters[] = {process.c_str(), resource.c_str(), amount.c_str()};
            if (!execute("INSERT INTO resource_allocation "
                         "(process_id, resource_id, allocated_instances, requested_instances) "
                         "VALUES ($1, $2, $3, 0)", 3, parameters, error)) {
                rollback();
                return false;
            }
        }
        const char* resourceParameters[] = {amount.c_str(), resource.c_str()};
        if (!execute("UPDATE resources SET available_instances = "
                     "available_instances - $1 WHERE resource_id = $2",
                     2, resourceParameters, error)) {
            rollback();
            return false;
        }
        granted = true;
    }
    return commit(error);
}

bool PostgresStore::releaseResource(int processId, int resourceId, int quantity,
                                    std::string& error) {
    if (!begin(error)) return false;
    const std::string process = std::to_string(processId);
    const std::string resource = std::to_string(resourceId);
    const std::string amount = std::to_string(quantity);
    const char* parameters[] = {process.c_str(), resource.c_str()};
    const char* releaseParameters[] = {amount.c_str(), process.c_str(), resource.c_str()};
    PGresult* result = PQexecParams(connection_,
        "UPDATE resource_allocation SET allocated_instances = allocated_instances - $1 "
        "WHERE process_id = $2 AND resource_id = $3 AND allocated_instances >= $1",
        3, nullptr, releaseParameters,
        nullptr, nullptr, 0);
    if (result == nullptr || PQresultStatus(result) != PGRES_COMMAND_OK ||
        std::atoi(PQcmdTuples(result)) != 1) {
        error = result == nullptr ? PQerrorMessage(connection_) :
                resultError(result);
        if (result != nullptr) PQclear(result);
        rollback();
        return false;
    }
    PQclear(result);
    const char* resourceParameters[] = {amount.c_str(), resource.c_str()};
    if (!execute("UPDATE resources SET available_instances = available_instances + $1 "
                 "WHERE resource_id = $2", 2, resourceParameters, error)) {
        rollback();
        return false;
    }
    return commit(error);
}

bool PostgresStore::logDeadlock(const std::vector<int>& processIds,
                                const std::string& action, std::string& error) {
    std::ostringstream ids;
    for (std::size_t i = 0; i < processIds.size(); ++i) {
        if (i) ids << ',';
        ids << processIds[i];
    }
    const char* parameters[] = {ids.str().c_str(), action.c_str()};
    return execute("INSERT INTO deadlock_logs (involved_processes, resolution_action) "
                   "VALUES ($1, $2)", 2, parameters, error);
}
