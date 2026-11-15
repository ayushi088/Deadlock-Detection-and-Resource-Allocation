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

The current local HTTP adapter does not read a database connection string yet;
these scripts are the PostgreSQL foundation for the pending persistence layer.

## Start the backend

Build with CMake, then run the API executable:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\safestate_api.exe
```

The API listens on `http://localhost:8081` and enables CORS for the static frontend.

The current adapter keeps its working state in memory for local development. The
PostgreSQL DDL is provided in `sql/schema.sql` and `sql/memory_schema.sql`, but
the adapter does not yet open a PostgreSQL connection or persist API mutations.
Do not use this adapter as a production database-backed service until that
repository layer is implemented.

## Connect another backend

Configure another HTTP adapter URL in **Settings**, or before loading the app:

```html
<script>window.SAFESTATE_API_BASE_URL = 'http://localhost:8081';</script>
```

The same value can be stored with `localStorage.setItem('safestate_api_url', 'http://localhost:8081')`. Endpoint paths and request methods are centralized in `assets/js/api.js`; update `ENDPOINTS` to match the routes exposed by your existing adapter.

Expected JSON endpoints are documented in the Settings screen. Scheduling is
available at `POST /api/scheduling/fcfs`, `/api/scheduling/sjf`,
`/api/scheduling/round-robin`, and `/api/scheduling/priority`. Responses may be
arrays or objects containing `data`, `items`, or the relevant named collection
(`processes`, `resources`, `allocations`, `history`, `nodes`, and `edges`).
