# SafeState frontend

This is a framework-free frontend for the existing SafeState C++/MySQL project. It does not create, modify, or replace the backend.

## Run

Serve the `frontend` directory with any static HTTP server (opening `index.html` directly can be blocked by browser module/CORS rules):

```powershell
cd frontend
python -m http.server 5500
```

Open `http://localhost:5500`.

## Connect the existing backend

The repository currently contains command-line C++ programs and SQL schemas, not an HTTP API. The frontend therefore does not invent a backend implementation or fake successful data. Configure the HTTP adapter URL in **Settings**, or before loading the app:

```html
<script>window.SAFESTATE_API_BASE_URL = 'http://localhost:8080';</script>
```

The same value can be stored with `localStorage.setItem('safestate_api_url', 'http://localhost:8080')`. Endpoint paths and request methods are centralized in `assets/js/api.js`; update `ENDPOINTS` to match the routes exposed by your existing adapter.

Expected JSON endpoints are documented in the Settings screen. Responses may be arrays or objects containing `data`, `items`, or the relevant named collection (`processes`, `resources`, `allocations`, `history`, `nodes`, and `edges`).
