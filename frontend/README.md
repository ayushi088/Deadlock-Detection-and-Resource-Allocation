# SafeState frontend

This is a framework-free frontend for the SafeState C++ project. The repository now includes a small HTTP adapter (`safestate_api`) for local development.

## Run

Serve the `frontend` directory with any static HTTP server (opening `index.html` directly can be blocked by browser module/CORS rules):

```powershell
cd frontend
python -m http.server 5500
```

Open `http://localhost:5500`.

## PostgreSQL schema

Create a database and apply the schema with PostgreSQL tools:

```powershell
createdb safestate
psql -d safestate -f ..\sql\schema.sql
psql -d safestate -f ..\sql\memory_schema.sql
```

The C++ API uses PostgreSQL through `libpq`. Copy the repository's
`.env.example` to `.env`, replace the password, and load it into the process
environment before starting the API:

```powershell
Copy-Item ..\.env.example ..\.env
Get-Content ..\.env | ForEach-Object {
    if ($_ -match '^([^#=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2]) }
}
```

## Start the backend

Build with CMake, then run the API executable:

```powershell
cmake -S . -B build -DPOSTGRESQL_ROOT="C:/Program Files/PostgreSQL/18"
cmake --build build --config Release
.\build\Release\safestate_api.exe
```

The API listens on `http://localhost:8082` by default and enables CORS for the static frontend.
Set `SAFESTATE_PORT=8081` if that port is available.

The API now loads and persists processes, resources, allocations, and deadlock
logs using the four existing tables: `processes`, `resources`,
`resource_allocation`, and `deadlock_logs`. The processes table supplies the
status field; the legacy scheduling-only arrival, burst, and priority values
remain API defaults because those columns are not present in the four tables.

## Connect another backend

Configure another HTTP adapter URL in **Settings**, or before loading the app:

```html
<script>window.SAFESTATE_API_BASE_URL = 'http://localhost:8082';</script>
```

The same value can be stored with `localStorage.setItem('safestate_api_url', 'http://localhost:8082')`. Endpoint paths and request methods are centralized in `assets/js/api.js`; update `ENDPOINTS` to match the routes exposed by your existing adapter.

Expected JSON endpoints are documented in the Settings screen. Scheduling is
available at `POST /api/scheduling/fcfs`, `/api/scheduling/sjf`,
`/api/scheduling/round-robin`, and `/api/scheduling/priority`. Responses may be
arrays or objects containing `data`, `items`, or the relevant named collection
(`processes`, `resources`, `allocations`, `history`, `nodes`, and `edges`).
