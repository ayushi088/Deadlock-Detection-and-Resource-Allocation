#pragma once

#include <libpq-fe.h>

#include <string>
#include <vector>

struct DbProcess {
    int id;
    std::string name;
    std::string state;
};

struct DbResource {
    int id;
    std::string name;
    int total;
    int available;
};

struct DbAllocation {
    int processId;
    int resourceId;
    int allocated;
    int requested;
};

class PostgresStore {
public:
    PostgresStore();
    ~PostgresStore();

    bool connect(std::string& error);
    bool load(std::vector<DbProcess>& processes,
              std::vector<DbResource>& resources,
              std::vector<DbAllocation>& allocations,
              std::string& error);
    bool createProcess(const DbProcess& process, std::string& error);
    bool updateProcess(const DbProcess& process, std::string& error);
    bool deleteProcess(int processId, std::string& error);
    bool createResource(const DbResource& resource, std::string& error);
    bool updateResource(const DbResource& resource, std::string& error);
    bool deleteResource(int resourceId, std::string& error);
    bool requestResource(int processId, int resourceId, int quantity,
                         bool& granted, std::string& error);
    bool releaseResource(int processId, int resourceId, int quantity,
                         std::string& error);
    bool logDeadlock(const std::vector<int>& processIds,
                     const std::string& action, std::string& error);

private:
    PGconn* connection_;

    bool execute(const char* sql, int parameterCount,
                 const char* const* parameters, std::string& error);
    bool begin(std::string& error);
    bool rollback();
    bool commit(std::string& error);
};
