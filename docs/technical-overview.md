# Nyx — Technical Overview

## 1. System Architecture

Nyx is a multi-tier web application with a C++23 backend and a Next.js frontend. It integrates
space-based photometric data from NASA's TESS mission with ground-based observations captured
using a DSLR or telescope camera in Mauritius.

```
Browser (Next.js 15, App Router)
        │  REST/JSON over HTTPS
        ▼
C++23 Backend (Drogon HTTP framework)
        │
        ├── PostgreSQL (primary store: users, targets, light curves, sessions, images)
        ├── NASA MAST API (remote: name resolution, TESS product discovery, FITS download)
        └── Local filesystem (uploaded raw image files, ./uploads/)
```

The backend follows a clean architecture layering:

```
presentation/   — HTTP controllers, middleware, request/response serialisation
application/    — use-case services, DTOs, interfaces for external dependencies
domain/         — entity definitions, repository interfaces (no framework types)
infrastructure/ — concrete implementations: PostgreSQL, LibRaw, CFITSIO, MAST HTTP client
core/           — error types, UUID generation, logging initialisation
```

All cross-layer dependencies point inward. No Drogon or database types appear in
`application/` or `domain/`.

---

## 2. Technology Stack

| Concern | Choice |
|---|---|
| Language | C++23 (GCC 13+ / Clang 17+) |
| HTTP framework | Drogon |
| Database | PostgreSQL |
| DB migrations | goose |
| Raw image decoding | LibRaw |
| EXIF metadata | Exiv2 |
| FITS parsing | CFITSIO |
| Password hashing | bcrypt (via backend hasher) |
| JWT | custom JWT service wrapping a JWT library |
| JSON serialisation | nlohmann/json |
| Frontend | Next.js 15, TypeScript, Tailwind CSS, shadcn/ui |
| Charts | Plotly.js (react-plotly.js, SSR disabled) |
| Build | CMake + vcpkg |

---

## 3. Database Schema

Migrations are managed by goose and stored in `backend/migrations/`. Each file is
timestamped and applied in order.

### `users`
```sql
id            UUID PRIMARY KEY
email         VARCHAR(255) UNIQUE NOT NULL
password_hash VARCHAR(255)           -- NULL for OAuth-only accounts
display_name  VARCHAR(100)           -- nullable
email_verified BOOLEAN DEFAULT FALSE
auth_provider VARCHAR(20)            -- 'local' | 'google'
google_id     VARCHAR(255)           -- NULL for local accounts
created_at    TIMESTAMPTZ
updated_at    TIMESTAMPTZ
```

### `refresh_tokens`
```sql
id           UUID PRIMARY KEY
user_id      UUID REFERENCES users ON DELETE CASCADE
token_hash   VARCHAR(255) UNIQUE    -- SHA-256 of the raw token
family_id    UUID                   -- token rotation family
is_revoked   BOOLEAN DEFAULT FALSE
expires_at   TIMESTAMPTZ
created_at   TIMESTAMPTZ
```
Indices on `user_id`, `family_id`, `token_hash`.

### `targets`
```sql
id              UUID PRIMARY KEY
canonical_name  VARCHAR(255) UNIQUE   -- MAST-resolved name e.g. "HD 209458"
target_type     VARCHAR(50)           -- e.g. "STAR"
right_ascension DOUBLE PRECISION      -- degrees
declination     DOUBLE PRECISION      -- degrees
created_at      TIMESTAMPTZ
```

### `tess_observations`
```sql
id               UUID PRIMARY KEY
target_id        UUID REFERENCES targets ON DELETE CASCADE
obsid            VARCHAR(20) UNIQUE   -- MAST observation ID
cadence_seconds  INTEGER              -- 120 (2-min) or 20 (20-sec)
start_time       DOUBLE PRECISION     -- BTJD (Barycentric TESS Julian Date)
end_time         DOUBLE PRECISION
data_uri         VARCHAR(500)         -- relative MAST path to _lc.fits file
created_at       TIMESTAMPTZ
updated_at       TIMESTAMPTZ
```

### `light_curve_points`
```sql
id                   BIGSERIAL PRIMARY KEY
tess_observation_id  UUID REFERENCES tess_observations ON DELETE CASCADE
time                 DOUBLE PRECISION    -- BTJD
pdcsap_flux          REAL                -- Pre-search Data Conditioning SAP flux (e⁻/s)
pdcsap_flux_err      REAL
sap_flux             REAL                -- Simple Aperture Photometry flux
quality              INTEGER             -- TESS quality bitmask
```
Indices on `(tess_observation_id)` and `(tess_observation_id, time)`.

### `observation_sessions`
```sql
id           UUID PRIMARY KEY
user_id      UUID REFERENCES users ON DELETE CASCADE
target_id    UUID REFERENCES targets
telescope_id UUID REFERENCES telescopes
camera_id    UUID REFERENCES cameras
mount_id     UUID REFERENCES mounts
location_id  UUID REFERENCES observing_locations
filter_id    UUID REFERENCES filters  -- nullable
notes        TEXT
created_at   TIMESTAMPTZ
updated_at   TIMESTAMPTZ
```

### `observation_images`
```sql
id                      UUID PRIMARY KEY
session_id              UUID REFERENCES observation_sessions ON DELETE CASCADE
filename                VARCHAR(255)       -- UUID-based stored name
original_filename       VARCHAR(255)       -- user's original filename
file_path               VARCHAR(1024)      -- absolute path on disk
file_size_bytes         BIGINT
mime_type               VARCHAR(50)
captured_at             TIMESTAMPTZ        -- from EXIF DateTimeOriginal
camera_model            VARCHAR(255)       -- from EXIF
exposure_time_seconds   DOUBLE PRECISION   -- from EXIF
iso_speed               INTEGER
gps_latitude            DOUBLE PRECISION
gps_longitude           DOUBLE PRECISION
image_width             INTEGER
image_height            INTEGER
target_x                INTEGER            -- pixel coord for photometry
target_y                INTEGER
raw_flux                DOUBLE PRECISION   -- background-subtracted aperture sum
raw_flux_error          DOUBLE PRECISION   -- Poisson error estimate
relative_flux           DOUBLE PRECISION   -- normalised against reference stars
relative_flux_error     DOUBLE PRECISION
photometry_status       VARCHAR(20)        -- 'processing' | 'completed' | 'failed'
photometry_error_message TEXT
created_at              TIMESTAMPTZ
```

Equipment tables (`telescopes`, `cameras`, `mounts`, `filters`, `observing_locations`) each
carry a `user_id` foreign key — equipment is per-user, not shared.

---

## 4. Request Lifecycle

Every inbound HTTP request passes through three Drogon filters before reaching a controller,
in this order:

### 4.1 CorrelationIdFilter

Generates a UUID v4 correlation ID for the request and attaches it to Drogon's attribute map.
A scoped `spdlog` logger is created that prepends this ID to every log line for the lifetime
of the request. All subsequent layers receive this logger via the attribute map, ensuring every
log line for a request shares the same correlation ID regardless of which thread handles it.

### 4.2 RateLimitFilter

Applied only to the auth endpoints (`POST /api/v1/auth/login`,
`POST /api/v1/auth/register`). Maintains a per-IP sliding window of timestamps using a
`std::deque<std::chrono::steady_clock::time_point>` behind a `std::mutex`. Requests older
than the window are pruned on each check. If `timestamps.size() >= max_requests`, the filter
returns HTTP 429 before the request reaches the controller.

### 4.3 JwtAuthFilter

Applied to all protected routes. Reads the `Authorization: Bearer <token>` header, verifies
the JWT signature and expiry via `ITokenService::verify_access_token`, and on success injects
`user_id` and `user_email` into the request attribute map for controllers to read. On failure
it short-circuits with HTTP 401.

### 4.4 CsrfFilter

Applied to all state-changing methods (POST, PUT, PATCH, DELETE). Reads the `X-CSRF-Token`
header and the `csrf_token` cookie; if they are absent or do not match, the request is
rejected with HTTP 403.

### 4.5 CORS

CORS is handled at the `registerPreRoutingAdvice` level in `main.cpp`. OPTIONS preflight
requests are handled immediately and return HTTP 204 with the appropriate
`Access-Control-Allow-*` headers. Non-preflight responses receive `Access-Control-Allow-Origin`
and `Access-Control-Allow-Credentials: true` via `registerPostHandlingAdvice`.

The allowed origin is read from the `FRONTEND_URL` environment variable at startup.

---

## 5. Authentication System

### 5.1 Registration

1. Client POSTs `{ email, password }` to `POST /api/v1/auth/register`.
2. `AuthService::register_user` hashes the password using bcrypt (cost ≥ 12).
3. A `User` record is created with `email_verified = false` and `auth_provider = 'local'`.
4. A UUID verification token is generated, hashed with SHA-256, and stored in
   `verification_tokens` with a 24-hour expiry.
5. The raw token is emailed to the user. In development, `ConsoleEmailSender` prints it to
   stdout; in production, `SmtpEmailSender` delivers it over TLS.
6. The response returns the user profile (HTTP 201). No tokens are issued until email
   verification completes.

### 5.2 Email Verification

1. Client POSTs `{ token }` to `POST /api/v1/auth/verify-email`.
2. The raw token is hashed; the hash is looked up in `verification_tokens`.
3. Token validity checks: not already used, not expired (lexicographic ISO-8601 comparison).
4. On success: `verification_tokens.used_at` is set; `users.email_verified` is set to `true`.

### 5.3 Login and Token Issuance

1. Client POSTs `{ email, password }` to `POST /api/v1/auth/login`.
2. `AuthService::login` checks: user exists, `auth_provider != 'google'` (prevents password
   login on OAuth accounts), password hash verifies, `email_verified == true`.
3. On success, `ITokenService::generate_token_pair` issues:
   - **Access token**: short-lived JWT (15 minutes), signed with HMAC-SHA256, contains
     `user_id`, `email`, `iat`, `exp`. Returned in the response body.
   - **Refresh token**: long-lived JWT (7 days), signed separately. Stored as a SHA-256 hash
     in `refresh_tokens` with a `family_id` UUID. Set as an HttpOnly, Secure, SameSite=Strict
     cookie. Also returned in the response body.
   - **CSRF token**: random UUID set as a readable (non-HttpOnly) cookie so the frontend
     JavaScript can read it and inject it as `X-CSRF-Token` on mutations.

### 5.4 Token Refresh and Rotation

1. Client POSTs to `POST /api/v1/auth/refresh` (the HttpOnly cookie is sent automatically).
2. The refresh token JWT signature and expiry are verified.
3. The token hash is looked up in `refresh_tokens`.
4. **Reuse detection**: if `is_revoked == true`, the entire token family is revoked
   (`UPDATE refresh_tokens SET is_revoked = true WHERE family_id = ?`). This invalidates all
   active sessions for that family, indicating a stolen token. The client receives HTTP 401.
5. On a valid unused token: the old record is marked revoked; a new token pair is issued with
   the same `family_id`. The chain of tokens within a family can be traced for audit.

### 5.5 Logout

The client POSTs to `POST /api/v1/auth/logout`. The entire refresh token family is revoked,
invalidating all sessions across devices that share that family.

### 5.6 Google OAuth

`POST /api/v1/auth/google` accepts an authorization code and redirect URI.
`GoogleAuthClient` exchanges the code for tokens at Google's token endpoint and retrieves
the user info. If a user with that `google_id` exists, they are logged in. If a user exists
with the same email but `auth_provider = 'local'`, a conflict error is returned to prevent
account takeover. Otherwise, a new user is created with `email_verified = true` and no
password hash.

---

## 6. Target Resolution and NASA Data Pipeline

### 6.1 Name Resolution via MAST

`POST /api/v1/targets/resolve` accepts `{ target_name: "HD 209458" }`.

`TargetService::resolve_target` calls `MastClient::resolve_target`, which sends:

```
POST https://mast.stsci.edu/api/v0/invoke
Content-Type: application/x-www-form-urlencoded

request={"service":"Mast.Name.Lookup","params":{"input":"HD 209458","format":"json"},...}
```

MAST returns a canonical name and sky coordinates (RA/Dec in degrees).

The result is checked against the local `targets` table by `canonical_name`. If already
cached, the stored record and its associated `tess_observations` are returned immediately
without a second MAST call. This avoids redundant network requests for repeated lookups.

If not cached, a new `Target` row is created and a TESS time-series search is executed.

### 6.2 TESS Observation Discovery

`MastClient::search_tess_timeseries` queries the MAST filtered service:

```
POST https://mast.stsci.edu/api/v0/invoke
request={"service":"Mast.Caom.Filtered","params":{
  "filters":[
    {"paramName":"obs_collection","values":["TESS"]},
    {"paramName":"dataproduct_type","values":["timeseries"]},
    {"paramName":"s_ra","values":[{"min":ra-0.02,"max":ra+0.02}]},
    {"paramName":"s_dec","values":[{"min":dec-0.02,"max":dec+0.02}]}
  ],
  "columns":"obsid,t_min,t_max,em_min,em_max,obs_collection"
}}
```

The response is a columnar JSON array. Each row is parsed to extract `obsid`,
`t_min` (start time in Modified Julian Date), `t_max`, and cadence (inferred from the
`em_min`/`em_max` wavelength range: 120 s for 2-minute cadence, 20 s for 20-second cadence).

Multi-sector observations (time span > 30 days) are filtered out; only single-sector
observations are stored, as these correspond to individual TESS sectors and have
self-contained FITS files. Duplicate `obsid` values are detected via a bulk
`SELECT obsid FROM tess_observations WHERE obsid = ANY($1)` check before insertion.

### 6.3 Data Product Discovery

`POST /api/v1/tess-observations/:id/discover-products` fetches the MAST data products for
a given `obsid`:

```
POST https://mast.stsci.edu/api/v0/invoke
request={"service":"Mast.Caom.Products","params":{"obsid":"<obsid>"},...}
```

The product list is scanned for a file where `description == "Light curves"`,
`product_type == "SCIENCE"`, and `filename` ends with `_lc.fits`. The relative path
(`data_uri`) is stored in `tess_observations.data_uri`.

### 6.4 FITS Download and Parsing

`POST /api/v1/tess-observations/:id/fetch-light-curve` is idempotent: if
`light_curve_points` already exist for this observation, it returns the cached count and
time range without re-downloading.

Otherwise, `MastClient::download_fits` makes a GET request to
`https://mast.stsci.edu<data_uri>` and returns the raw FITS bytes as a `std::string`.

`FitsParser::parse_light_curve` (using CFITSIO):
1. Writes the FITS bytes to a `mkstemp` temporary file (`/tmp/nyx_fits_XXXXXX`). A RAII
   `FitsFileGuard` ensures deletion on scope exit.
2. Opens the file and moves to HDU 2 (the binary table extension containing the time series).
3. Reads column numbers for `TIME`, `PDCSAP_FLUX`, `PDCSAP_FLUX_ERR`, `SAP_FLUX`, `QUALITY`
   using case-insensitive column name lookup (`fits_get_colnum` with `CASEINSEN`).
4. Reads all rows into pre-allocated `std::vector<double/float/int>` buffers.
5. Filters out rows where `TIME` is NaN; maps NaN flux values to `std::nullopt`.
6. Returns a `std::vector<LightCurvePoint>` which is bulk-inserted into
   `light_curve_points` via a PostgreSQL `COPY`-style parameterised bulk insert.

### 6.5 Light Curve Retrieval

`GET /api/v1/tess-observations/:id/light-curve?quality_filter=true`

When `quality_filter=true`, the repository adds `AND quality = 0` to exclude cadences with
known anomalies (scattered light, cosmic rays, etc.) as flagged by the TESS pipeline
quality bitmask. The response is:

```json
{
  "data": {
    "tess_observation_id": "...",
    "obsid": "...",
    "point_count": 18420,
    "points": [
      { "time": 1325.3, "pdcsap_flux": 284532.0, "pdcsap_flux_err": 12.4,
        "sap_flux": 290100.0, "quality": 0 },
      ...
    ]
  },
  "meta": { "request_id": "...", "timestamp": "..." }
}
```

The frontend divides each `pdcsap_flux` value by the median of all non-null flux values in
the series to produce a normalised relative flux (1.0 = median brightness). This is necessary
because raw PDCSAP flux is in electrons per second and varies by several orders of magnitude
between targets.

---

## 7. Observation Session Management

A session ties together a target, a set of equipment, a location, and a collection of images.

### 7.1 Equipment Ownership Validation

On `POST /api/v1/observation-sessions`, `ObservationService::verify_equipment_ownership`
verifies that the telescope, camera, mount, location, and (optionally) filter referenced in
the request all exist and belong to the authenticated user. Any mismatch returns HTTP 403.

### 7.2 Image Upload

`POST /api/v1/observation-sessions/:id/images` accepts multipart/form-data.

1. Files are accepted with MIME types: `image/jpeg`, `image/png`, `image/tiff`,
   `image/x-adobe-dng`, `image/x-nikon-nef`, `image/x-canon-cr2`, `image/x-sony-arw`,
   `image/x-fuji-raf`, `image/x-olympus-orf`, `image/x-panasonic-rw2`,
   `image/x-pentax-pef`. The MIME type is inferred from the file extension.
2. Maximum file size: 50 MB (enforced both in application code and via Drogon's
   `setClientMaxBodySize(52428800)`).
3. The file is saved to `./uploads/<user_id>/<session_id>/<uuid><ext>`.
4. `Exiv2ExifParser::parse` extracts metadata: `DateTimeOriginal` (normalised from EXIF
   colon-separated format `2023:10:22 16:08:01` to PostgreSQL-compatible
   `2023-10-22 16:08:01`), `Image.Model`, `ExposureTime`, `ISOSpeedRatings`, and GPS
   coordinates converted from DMS to decimal degrees.
5. An `ObservationImage` record is created. Photometry fields are all `NULL` at this stage.

### 7.3 Local Photometry Pipeline

`POST /api/v1/observation-sessions/:id/photometry` accepts `{ target_x, target_y }` (pixel
coordinates of the target star in the images).

The controller returns HTTP 202 immediately with `{ status: "processing" }` while a
`std::thread` runs the pipeline in the background. All images in the session are marked
`photometry_status = 'processing'` before the thread is detached.

**Aperture size calculation**

Aperture radius is derived from the plate scale of the optical system:

```
plate_scale (arcsec/px) = (pixel_size_μm / focal_length_mm) × 206.265
aperture_radius (px)    = max(5.0, 2.0 / plate_scale)
annulus_inner           = 2 × aperture_radius
annulus_outer           = 3 × aperture_radius
```

If `pixel_size_um` or `focal_length_mm` is zero (equipment specs not filled in), a fallback
aperture of 8 pixels is used.

**Per-image processing (AperturePhotometer)**

For each image in the session:

1. `LibrawDngDecoder::decode` opens the file using LibRaw's `open_file` / `unpack` /
   `dcraw_process` pipeline. Parameters are set for linear output (gamma = 1.0),
   16-bit output, no auto-brightening, and camera white balance. The resulting RGB buffer
   is stored as `std::vector<uint16_t>`.

2. Pixel luminance is computed per-pixel as BT.709 weighted sum for colour images:
   `L = 0.2126R + 0.7152G + 0.0722B`, or directly for single-channel images.

3. **Target aperture measurement** (`measure_aperture`):
   - All pixels within Euclidean distance `aperture_radius` of `(target_x, target_y)` are
     summed: `aperture_sum`.
   - Pixels in the annulus (`annulus_inner ≤ r ≤ annulus_outer`) form the sky background
     sample: `sky_per_pixel = annulus_sum / annulus_count`.
   - Background-subtracted flux:
     `raw_flux = aperture_sum - sky_per_pixel × aperture_count`
   - Poisson noise estimate:
     `raw_flux_error = √(aperture_sum + aperture_count × sky_per_pixel)`

4. **Reference star detection** (`detect_sources`):
   - Image-wide mean and standard deviation of luminance are computed.
   - Local maxima above a 5σ threshold are found in a 11×11 sliding window
     (half-window = 5 pixels), excluding a 20-pixel edge margin.
   - Sources are sorted by peak brightness.

5. **Reference flux measurement**:
   - Up to 5 reference stars are selected from the detected sources, excluding any source
     within `2 × aperture_radius` of the target.
   - Each reference star's flux is measured using the same aperture/annulus parameters.
   - `mean_ref = mean(reference_fluxes)`

6. **Relative flux normalisation**:
   - `relative_flux = target_raw_flux / mean_ref`
   - `relative_flux_error = relative_flux × (target_raw_flux_error / target_raw_flux)`

7. Results are written back to `observation_images` via `update_photometry`. If decoding
   or measurement fails, `photometry_status` is set to `'failed'` with a descriptive
   `photometry_error_message`.

The frontend polls `GET /api/v1/observation-sessions/:id` every 3 seconds while any image
has `photometry_status == 'processing'`, stopping once all images reach a terminal status.

---

## 8. Frontend Architecture

### 8.1 Auth Layer

`AuthProvider` (`lib/auth-context.tsx`) wraps the entire application. On mount it calls
`POST /api/v1/auth/refresh` — if the HttpOnly refresh token cookie is present and valid,
the new access token is stored in React state (never in `localStorage` or a readable cookie).
If refresh fails, the user is unauthenticated.

The access token is held in a module-level variable in `lib/api.ts` and injected as
`Authorization: Bearer <token>` on every request. On any 401 response, `api.ts` attempts
one silent refresh then retries the original request. If the retry also fails, the
unauthenticated handler is called and the user is redirected to `/login`.

CSRF tokens are read from the `csrf_token` cookie via `document.cookie` and sent as
`X-CSRF-Token` on all POST/PUT/PATCH/DELETE requests.

### 8.2 Route Protection

Next.js middleware (`middleware.ts`) checks for the presence of the `csrf_token` readable
cookie as a proxy signal for authentication. Protected routes redirect to `/login` if the
cookie is absent. This is a UX-only check; actual authentication is enforced server-side
by `JwtAuthFilter`.

### 8.3 API Layer

`lib/api.ts` exports typed wrappers:

```
authApi     — login, register, refresh, logout, verifyEmail, resendVerification
profileApi  — get, updateDisplayName
targetApi   — resolve, getById, listTessObservations, getLightCurveComparison
tessApi     — discoverProducts, fetchLightCurve, getLightCurve
telescopeApi / cameraApi / mountApi / filterApi — CRUD
locationApi — CRUD
sessionApi  — list, getById, create, update, remove, runPhotometry, uploadImages, deleteImage
```

All calls go through a common `request<T>()` function that handles CSRF injection,
401 retry-with-refresh, and JSON envelope unwrapping (extracts `json.data`).

Image upload is a special case: `sessionApi.uploadImages` uses raw `fetch` with
`FormData` (no `Content-Type: application/json`) and implements the same 401-retry logic
manually.

### 8.4 Light Curve Visualisation

Target detail pages load TESS light curve data via `tessApi.getLightCurve`, which includes
`?quality_filter=true`. The raw `pdcsap_flux` values (electrons/second) are normalised
client-side:

```typescript
const nonNull = points.filter(p => p.pdcsap_flux != null).map(p => p.pdcsap_flux!);
const sorted = [...nonNull].sort((a, b) => a - b);
const scale = sorted[Math.floor(sorted.length / 2)];  // median
const y = points.map(p => p.pdcsap_flux != null ? p.pdcsap_flux / scale : null);
```

The chart is rendered by `react-plotly.js` with SSR disabled (dynamic import with
`{ ssr: false }`) to avoid hydration issues with Plotly's DOM manipulation.

---

## 9. Error Handling

All application code uses `std::expected<T, AppError>` (`Nyx::Core::Result<T>`) for
recoverable errors. `AppError` carries an `ErrorCode` enum, a human-readable message, and
an optional `std::vector<FieldError>` for validation failures.

Error codes map to HTTP status codes in `ResponseHelper::error`:

| ErrorCode | HTTP status |
|---|---|
| ValidationError | 400 |
| Unauthorized | 401 |
| EmailNotVerified | 403 |
| Forbidden | 403 |
| NotFound | 404 |
| Conflict | 409 |
| InvalidToken | 401 |
| RateLimitExceeded | 429 |
| InternalError | 500 |
| ExternalServiceError | 502 |

All 5xx responses return a generic message to the client; full details are logged at
`error` level with the correlation ID. Library exceptions (Drogon, PostgreSQL, LibRaw,
Exiv2) are caught at service boundaries and converted to `AppError`.

All error responses use the standard envelope:

```json
{
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Human-readable description",
    "details": [{ "field": "email", "message": "must be valid email" }]
  },
  "meta": { "request_id": "<correlation-id>", "timestamp": "<ISO-8601>" }
}
```

---

## 10. API Reference

All routes are prefixed `/api/v1/`. Protected routes require `Authorization: Bearer <token>`.
Mutation routes require `X-CSRF-Token`.

### Auth
| Method | Path | Auth | Description |
|---|---|---|---|
| POST | `/auth/register` | No | Register new user |
| POST | `/auth/login` | No | Login, receive token pair |
| POST | `/auth/refresh` | Cookie | Rotate refresh token |
| POST | `/auth/logout` | Cookie | Revoke token family |
| POST | `/auth/verify-email` | No | Verify email with token |
| POST | `/auth/resend-verification` | No | Resend verification email |
| POST | `/auth/google` | No | Google OAuth login/register |

### Profile
| Method | Path | Auth | Description |
|---|---|---|---|
| GET | `/users/me/profile` | JWT | Get own profile |
| PUT | `/users/me/profile` | JWT | Update display name |

### Targets and TESS
| Method | Path | Auth | Description |
|---|---|---|---|
| POST | `/targets/resolve` | JWT | Resolve name via MAST, cache result |
| GET | `/targets/:id` | JWT | Get cached target |
| GET | `/targets/:id/tess-observations` | JWT | List TESS observations for target |
| GET | `/targets/:id/light-curve-comparison` | JWT | Compare NASA vs local (placeholder) |
| POST | `/tess-observations/:id/discover-products` | JWT | Fetch FITS URI from MAST |
| POST | `/tess-observations/:id/fetch-light-curve` | JWT | Download and parse FITS into DB |
| GET | `/tess-observations/:id/light-curve` | JWT | Retrieve stored light curve points |

### Equipment
| Method | Path | Auth | Description |
|---|---|---|---|
| GET/POST | `/telescopes` | JWT | List / create |
| GET/PUT/DELETE | `/telescopes/:id` | JWT | Get / update / delete |
| GET/POST | `/cameras` | JWT | List / create |
| GET/PUT/DELETE | `/cameras/:id` | JWT | Get / update / delete |
| GET/POST | `/mounts` | JWT | List / create |
| GET/PUT/DELETE | `/mounts/:id` | JWT | Get / update / delete |
| GET/POST | `/filters` | JWT | List / create |
| GET/PUT/DELETE | `/filters/:id` | JWT | Get / update / delete |

### Observing Locations
| Method | Path | Auth | Description |
|---|---|---|---|
| GET/POST | `/observing-locations` | JWT | List / create |
| GET/PUT/DELETE | `/observing-locations/:id` | JWT | Get / update / delete |

### Observation Sessions and Images
| Method | Path | Auth | Description |
|---|---|---|---|
| GET/POST | `/observation-sessions` | JWT | List / create session |
| GET/PUT/DELETE | `/observation-sessions/:id` | JWT | Get (with images) / update / delete |
| POST | `/observation-sessions/:id/images` | JWT | Upload images (multipart) |
| DELETE | `/observation-sessions/:id/images/:img_id` | JWT | Delete image and file |
| POST | `/observation-sessions/:id/photometry` | JWT | Start photometry job |
