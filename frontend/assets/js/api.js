// Configure this URL to point at the SafeState C++ HTTP service.
export const API_BASE_URL = window.SAFESTATE_API_BASE_URL || localStorage.getItem('safestate_api_url') || 'http://localhost:8081';

export const ENDPOINTS = {
  dashboard: '/api/dashboard', processes: '/api/processes', resources: '/api/resources',
  allocations: '/api/allocations', request: '/api/allocations/request', release: '/api/allocations/release',
  scheduling: { fcfs: '/api/scheduling/fcfs', sjf: '/api/scheduling/sjf', roundRobin: '/api/scheduling/round-robin', priority: '/api/scheduling/priority' },
  banker: '/api/algorithms/banker', deadlock: '/api/algorithms/deadlock', rag: '/api/graph/rag',
  history: '/api/history'
};

function url(path) {
  return `${API_BASE_URL.replace(/\/$/, '')}${path}`;
}

async function request(path, options = {}) {
  if (!API_BASE_URL) throw new Error('Backend API URL is not configured. Set window.SAFESTATE_API_BASE_URL or safestate_api_url in localStorage.');
  let response;
  try {
    response = await fetch(url(path), {
      ...options,
      headers: { Accept: 'application/json', 'Content-Type': 'application/json', ...(options.headers || {}) }
    });
  } catch {
    throw new Error(`Cannot reach the SafeState backend at ${API_BASE_URL}. Start safestate_api on port 8081 or update the API URL in Settings.`);
  }
  let payload;
  try { payload = await response.json(); } catch { payload = null; }
  if (!response.ok) throw new Error(payload?.message || payload?.error || `Backend request failed (${response.status}).`);
  return payload;
}

const json = (method, body) => ({ method, body: JSON.stringify(body) });
export const api = {
  getDashboard: () => request(ENDPOINTS.dashboard),
  getProcesses: () => request(ENDPOINTS.processes),
  createProcess: (body) => request(ENDPOINTS.processes, json('POST', body)),
  updateProcess: (id, body) => request(`${ENDPOINTS.processes}/${encodeURIComponent(id)}`, json('PUT', body)),
  deleteProcess: (id) => request(`${ENDPOINTS.processes}/${encodeURIComponent(id)}`, { method: 'DELETE' }),
  getResources: () => request(ENDPOINTS.resources),
  createResource: (body) => request(ENDPOINTS.resources, json('POST', body)),
  updateResource: (id, body) => request(`${ENDPOINTS.resources}/${encodeURIComponent(id)}`, json('PUT', body)),
  deleteResource: (id) => request(`${ENDPOINTS.resources}/${encodeURIComponent(id)}`, { method: 'DELETE' }),
  getAllocations: () => request(ENDPOINTS.allocations),
  requestResource: (body) => request(ENDPOINTS.request, json('POST', body)),
  releaseResource: (body) => request(ENDPOINTS.release, json('POST', body)),
  runScheduling: (algorithm, body) => request(ENDPOINTS.scheduling[algorithm], json('POST', body)),
  runBanker: (body) => request(ENDPOINTS.banker, json('POST', body)),
  detectDeadlock: () => request(ENDPOINTS.deadlock, json('POST', {})),
  getRag: () => request(ENDPOINTS.rag),
  getHistory: () => request(ENDPOINTS.history)
};

export function list(payload, key) {
  if (Array.isArray(payload)) return payload;
  return payload?.[key] || payload?.data || payload?.items || [];
}
