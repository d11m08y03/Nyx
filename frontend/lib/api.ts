import type {
  Camera,
  Filter,
  LightCurveComparison,
  LightCurvePoint,
  Mount,
  ObservationSession,
  ObservingLocation,
  PhotometryStatus,
  Target,
  Telescope,
  TessObservation,
  TokenResponse,
  UserProfile,
} from "@/lib/types";

const API_BASE = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:8080";

export interface ApiErrorDetail {
  field: string;
  message: string;
}

export class NyxApiError extends Error {
  constructor(
    public readonly code: string,
    message: string,
    public readonly details: ApiErrorDetail[] = [],
    public readonly status: number = 0,
  ) {
    super(message);
    this.name = "NyxApiError";
  }
}

function getCsrfToken(): string | null {
  if (typeof document === "undefined") return null;
  const match = document.cookie.match(/(?:^|;\s*)csrf_token=([^;]+)/);
  return match ? decodeURIComponent(match[1]) : null;
}

let _accessToken: string | null = null;
let _onUnauthenticated: (() => void) | null = null;
let _refreshPromise: Promise<string | null> | null = null;

export function setAccessToken(token: string | null) {
  _accessToken = token;
}

export function getAccessToken(): string | null {
  return _accessToken;
}

export function setUnauthenticatedHandler(fn: () => void) {
  _onUnauthenticated = fn;
}

async function doRefresh(): Promise<string | null> {
  try {
    const res = await fetch(`${API_BASE}/api/v1/auth/refresh`, {
      method: "POST",
      credentials: "include",
      headers: { "Content-Type": "application/json" },
    });
    if (!res.ok) return null;
    const json = await res.json();
    const token: string = json.data.access_token;
    _accessToken = token;
    return token;
  } catch {
    return null;
  }
}

async function refreshOnce(): Promise<string | null> {
  if (_refreshPromise) return _refreshPromise;
  _refreshPromise = doRefresh().finally(() => {
    _refreshPromise = null;
  });
  return _refreshPromise;
}

type Method = "GET" | "POST" | "PUT" | "PATCH" | "DELETE";
const MUTATION_METHODS: Method[] = ["POST", "PUT", "PATCH", "DELETE"];

async function request<T>(
  method: Method,
  path: string,
  body?: unknown,
  retry = true,
): Promise<T> {
  const headers: Record<string, string> = {
    "Content-Type": "application/json",
  };
  if (_accessToken) headers["Authorization"] = `Bearer ${_accessToken}`;
  if (MUTATION_METHODS.includes(method)) {
    const csrf = getCsrfToken();
    if (csrf) headers["X-CSRF-Token"] = csrf;
  }

  const res = await fetch(`${API_BASE}${path}`, {
    method,
    credentials: "include",
    headers,
    body: body !== undefined ? JSON.stringify(body) : undefined,
  });

  if (res.status === 401 && retry) {
    const newToken = await refreshOnce();
    if (newToken) return request<T>(method, path, body, false);
    _accessToken = null;
    _onUnauthenticated?.();
    throw new NyxApiError("AUTHENTICATION_REQUIRED", "Session expired", [], 401);
  }

  if (res.status === 204) return undefined as T;

  const json = await res.json();

  if (!res.ok) {
    const err = json.error ?? { code: "INTERNAL_ERROR", message: "Unknown error", details: [] };
    throw new NyxApiError(err.code, err.message, err.details ?? [], res.status);
  }

  return json.data as T;
}

export const api = {
  get: <T>(path: string) => request<T>("GET", path),
  post: <T>(path: string, body?: unknown) => request<T>("POST", path, body),
  put: <T>(path: string, body?: unknown) => request<T>("PUT", path, body),
  patch: <T>(path: string, body?: unknown) => request<T>("PATCH", path, body),
  delete: <T>(path: string) => request<T>("DELETE", path),
};

// ─── Auth ────────────────────────────────────────────────────────────────────

export const authApi = {
  login: (email: string, password: string) =>
    api.post<TokenResponse>("/api/v1/auth/login", { email, password }),

  register: (email: string, password: string) =>
    api.post<UserProfile>("/api/v1/auth/register", { email, password }),

  refresh: () =>
    fetch(`${API_BASE}/api/v1/auth/refresh`, {
      method: "POST",
      credentials: "include",
      headers: { "Content-Type": "application/json" },
    }).then(async (res) => {
      if (!res.ok) throw new Error("refresh_failed");
      const json = await res.json();
      return json.data as TokenResponse;
    }),

  logout: () => api.post<void>("/api/v1/auth/logout"),

  verifyEmail: (token: string) =>
    api.post<{ message: string }>("/api/v1/auth/verify-email", { token }),

  resendVerification: (email: string) =>
    api.post<{ message: string }>("/api/v1/auth/resend-verification", { email }),
};

// ─── Profile ─────────────────────────────────────────────────────────────────

export const profileApi = {
  get: () => api.get<UserProfile>("/api/v1/users/me/profile"),

  // Returns only { display_name } — not full UserProfile
  updateDisplayName: (display_name: string) =>
    api.put<{ display_name: string }>("/api/v1/users/me/profile", {
      display_name,
    }),
};

// ─── Targets ─────────────────────────────────────────────────────────────────

export const targetApi = {
  resolve: (target_name: string) =>
    api.post<Target>("/api/v1/targets/resolve", { target_name }),

  getById: (id: string) => api.get<Target>(`/api/v1/targets/${id}`),

  listTessObservations: (id: string) =>
    api.get<TessObservation[]>(`/api/v1/targets/${id}/tess-observations`),

  getLightCurveComparison: (id: string) =>
    api.get<LightCurveComparison>(`/api/v1/targets/${id}/light-curve-comparison`),
};

// ─── TESS Observations ────────────────────────────────────────────────────────

interface LightCurveResponse {
  tess_observation_id: string;
  obsid: string;
  point_count: number;
  points: LightCurvePoint[];
}

export const tessApi = {
  discoverProducts: (id: string) =>
    api.post<TessObservation>(`/api/v1/tess-observations/${id}/discover-products`),

  fetchLightCurve: (id: string) =>
    api.post<{ tess_observation_id: string; obsid: string; point_count: number; time_min: number; time_max: number }>(
      `/api/v1/tess-observations/${id}/fetch-light-curve`,
    ),

  getLightCurve: (id: string) =>
    api
      .get<LightCurveResponse>(
        `/api/v1/tess-observations/${id}/light-curve?quality_filter=true`,
      )
      .then((r) => r.points),
};

// ─── Equipment ───────────────────────────────────────────────────────────────

export const telescopeApi = {
  list: () => api.get<Telescope[]>("/api/v1/telescopes"),
  getById: (id: string) => api.get<Telescope>(`/api/v1/telescopes/${id}`),
  create: (data: Omit<Telescope, "id">) => api.post<Telescope>("/api/v1/telescopes", data),
  update: (id: string, data: Omit<Telescope, "id">) =>
    api.put<Telescope>(`/api/v1/telescopes/${id}`, data),
  remove: (id: string) => api.delete<void>(`/api/v1/telescopes/${id}`),
};

export const cameraApi = {
  list: () => api.get<Camera[]>("/api/v1/cameras"),
  getById: (id: string) => api.get<Camera>(`/api/v1/cameras/${id}`),
  create: (data: Omit<Camera, "id">) => api.post<Camera>("/api/v1/cameras", data),
  update: (id: string, data: Omit<Camera, "id">) =>
    api.put<Camera>(`/api/v1/cameras/${id}`, data),
  remove: (id: string) => api.delete<void>(`/api/v1/cameras/${id}`),
};

export const mountApi = {
  list: () => api.get<Mount[]>("/api/v1/mounts"),
  getById: (id: string) => api.get<Mount>(`/api/v1/mounts/${id}`),
  create: (data: Omit<Mount, "id">) => api.post<Mount>("/api/v1/mounts", data),
  update: (id: string, data: Omit<Mount, "id">) =>
    api.put<Mount>(`/api/v1/mounts/${id}`, data),
  remove: (id: string) => api.delete<void>(`/api/v1/mounts/${id}`),
};

export const filterApi = {
  list: () => api.get<Filter[]>("/api/v1/filters"),
  getById: (id: string) => api.get<Filter>(`/api/v1/filters/${id}`),
  create: (data: Omit<Filter, "id">) => api.post<Filter>("/api/v1/filters", data),
  update: (id: string, data: Omit<Filter, "id">) =>
    api.put<Filter>(`/api/v1/filters/${id}`, data),
  remove: (id: string) => api.delete<void>(`/api/v1/filters/${id}`),
};

// ─── Locations ───────────────────────────────────────────────────────────────

export const locationApi = {
  list: () => api.get<ObservingLocation[]>("/api/v1/observing-locations"),
  getById: (id: string) =>
    api.get<ObservingLocation>(`/api/v1/observing-locations/${id}`),
  create: (data: Omit<ObservingLocation, "id">) =>
    api.post<ObservingLocation>("/api/v1/observing-locations", data),
  update: (id: string, data: Omit<ObservingLocation, "id">) =>
    api.put<ObservingLocation>(`/api/v1/observing-locations/${id}`, data),
  remove: (id: string) => api.delete<void>(`/api/v1/observing-locations/${id}`),
};

// ─── Observation Sessions ────────────────────────────────────────────────────

export const sessionApi = {
  list: () => api.get<ObservationSession[]>("/api/v1/observation-sessions"),
  getById: (id: string) =>
    api.get<ObservationSession>(`/api/v1/observation-sessions/${id}`),
  create: (data: {
    target_id: string;
    telescope_id: string;
    camera_id: string;
    mount_id: string;
    location_id: string;
    filter_id?: string;
    notes?: string;
  }) => api.post<ObservationSession>("/api/v1/observation-sessions", data),
  update: (id: string, data: { filter_id?: string; notes?: string }) =>
    api.put<ObservationSession>(`/api/v1/observation-sessions/${id}`, data),
  remove: (id: string) => api.delete<void>(`/api/v1/observation-sessions/${id}`),
  // Returns { session_id, status, message } — images in the session carry per-image flux
  runPhotometry: (id: string, target_x: number, target_y: number) =>
    api.post<PhotometryStatus>(
      `/api/v1/observation-sessions/${id}/photometry`,
      { target_x, target_y },
    ),
  // Multipart POST — files sent as FormData, returns array of created images
  uploadImages: async (session_id: string, files: File[]) => {
    const form = new FormData();
    for (const file of files) form.append("file", file);
    const headers: Record<string, string> = {};
    if (_accessToken) headers["Authorization"] = `Bearer ${_accessToken}`;
    const csrf = getCsrfToken();
    if (csrf) headers["X-CSRF-Token"] = csrf;
    const res = await fetch(
      `${API_BASE}/api/v1/observation-sessions/${session_id}/images`,
      { method: "POST", credentials: "include", headers, body: form },
    );
    if (res.status === 401) {
      const newToken = await refreshOnce();
      if (newToken) return sessionApi.uploadImages(session_id, files);
      _accessToken = null;
      _onUnauthenticated?.();
      throw new NyxApiError("AUTHENTICATION_REQUIRED", "Session expired", [], 401);
    }
    const json = await res.json();
    if (!res.ok) {
      const err = json.error ?? { code: "INTERNAL_ERROR", message: "Upload failed", details: [] };
      throw new NyxApiError(err.code, err.message, err.details ?? [], res.status);
    }
    return json.data as import("@/lib/types").ObservationImage[];
  },
  // No GET /images endpoint — images are embedded in session.images from getById
  deleteImage: (session_id: string, image_id: string) =>
    api.delete<void>(
      `/api/v1/observation-sessions/${session_id}/images/${image_id}`,
    ),
};
