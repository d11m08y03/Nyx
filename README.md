# Nyx

Astronomical observation platform. Integrates NASA space mission data (MAST, Exoplanet Archive) with ground-based telescope observations from Mauritius. C++23 backend, Next.js frontend (deferred).

## Requirements

- GCC 13+ or Clang 17+
- CMake 3.20+
- vcpkg (`VCPKG_ROOT` env var set)
- PostgreSQL
- goose (migrations)

## Build

```sh
cd backend

# Debug
cmake --preset default
cmake --build build

# Release
cmake --preset release
cmake --build build --preset release

# Run
./build/nyx-backend
```

## Tests

```sh
cd backend
cmake --preset test
cmake --build build --preset test
ctest --test-dir build --output-on-failure
```

## Configuration

Copy `.env.example` to `.env` in the `backend/` directory (or set env vars directly).

| Variable | Required | Default | Description |
|---|---|---|---|
| `PGHOST` | yes | — | PostgreSQL host |
| `PGPORT` | no | `5432` | PostgreSQL port |
| `PGDATABASE` | yes | — | Database name |
| `PGUSER` | yes | — | Database user |
| `PGPASSWORD` | yes | — | Database password |
| `PGSSLMODE` | no | `prefer` | SSL mode |
| `JWT_SECRET` | yes | — | JWT signing secret |
| `JWT_ACCESS_TOKEN_EXPIRY_SECONDS` | no | `900` | Access token lifetime (15 min) |
| `JWT_REFRESH_TOKEN_EXPIRY_SECONDS` | no | `604800` | Refresh token lifetime (7 days) |
| `SERVER_PORT` | no | `8080` | HTTP listen port |
| `SERVER_THREADS` | no | `4` | Drogon thread count |
| `LOG_LEVEL` | no | `debug` | spdlog level |
| `COOKIE_SECURE` | no | `true` | Secure flag on cookies |
| `FRONTEND_URL` | no | `http://localhost:5173` | CORS / redirect base URL |
| `NASA_MAST_BASE_URL` | no | `/api/v0` | MAST API base |
| `NASA_EXOPLANET_ARCHIVE_BASE_URL` | no | `https://exoplanetarchive.ipac.caltech.edu` | Exoplanet Archive base |
| `SMTP_HOST` | no | `""` | SMTP host (blank → console sender) |
| `SMTP_PORT` | no | `587` | SMTP port |
| `SMTP_USERNAME` | no | `""` | SMTP username |
| `SMTP_PASSWORD` | no | `""` | SMTP password |
| `SMTP_FROM_EMAIL` | no | `""` | From address |
| `SMTP_USE_TLS` | no | `true` | TLS for SMTP |
| `GOOGLE_CLIENT_ID` | no | `""` | Google OAuth client ID |
| `GOOGLE_CLIENT_SECRET` | no | `""` | Google OAuth client secret |

## Migrations

```sh
# From backend/
goose -dir migrations postgres "$DATABASE_URL" up
goose -dir migrations postgres "$DATABASE_URL" down

# Create new migration
goose -dir migrations create <name> sql
```

## API

Base path: `/api/v1`

All responses use:
```json
{ "data": {}, "meta": { "request_id": "...", "timestamp": "ISO8601" } }
```

Errors:
```json
{ "error": { "code": "...", "message": "...", "details": [] }, "meta": { ... } }
```

Legend: 🔓 public · 🔑 JWT required · 🛡️ JWT + CSRF required · ⚡ rate-limited

---

### Auth

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/auth/register` | 🔓 ⚡ | Register with email + password |
| `POST` | `/auth/login` | 🔓 ⚡ | Login; sets `refresh_token` cookie |
| `POST` | `/auth/refresh` | 🔓 ⚡ | Rotate refresh token; returns new access token |
| `POST` | `/auth/logout` | 🔓 ⚡ | Revoke entire token family; clears cookies |
| `POST` | `/auth/verify-email` | 🔓 ⚡ | Verify email with token from email link |
| `POST` | `/auth/resend-verification` | 🔓 ⚡ | Resend verification email |
| `POST` | `/auth/google` | 🔓 ⚡ | Google OAuth (exchange code for tokens) |

---

### Profile

| Method | Path | Auth | Description |
|---|---|---|---|
| `PUT` | `/users/me/profile` | 🛡️ | Update display name / onboarding |

---

### Equipment

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/telescopes` | 🛡️ | Create telescope |
| `GET` | `/telescopes` | 🔑 | List telescopes |
| `GET` | `/telescopes/{id}` | 🔑 | Get telescope |
| `PUT` | `/telescopes/{id}` | 🛡️ | Update telescope |
| `DELETE` | `/telescopes/{id}` | 🛡️ | Delete telescope |
| `POST` | `/cameras` | 🛡️ | Create camera |
| `GET` | `/cameras` | 🔑 | List cameras |
| `GET` | `/cameras/{id}` | 🔑 | Get camera |
| `PUT` | `/cameras/{id}` | 🛡️ | Update camera |
| `DELETE` | `/cameras/{id}` | 🛡️ | Delete camera |
| `POST` | `/mounts` | 🛡️ | Create mount |
| `GET` | `/mounts` | 🔑 | List mounts |
| `GET` | `/mounts/{id}` | 🔑 | Get mount |
| `PUT` | `/mounts/{id}` | 🛡️ | Update mount |
| `DELETE` | `/mounts/{id}` | 🛡️ | Delete mount |
| `POST` | `/filters` | 🛡️ | Create filter |
| `GET` | `/filters` | 🔑 | List filters |
| `GET` | `/filters/{id}` | 🔑 | Get filter |
| `PUT` | `/filters/{id}` | 🛡️ | Update filter |
| `DELETE` | `/filters/{id}` | 🛡️ | Delete filter |

---

### Observing Locations

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/observing-locations` | 🛡️ | Create location |
| `GET` | `/observing-locations` | 🔑 | List locations |
| `GET` | `/observing-locations/{id}` | 🔑 | Get location |
| `PUT` | `/observing-locations/{id}` | 🛡️ | Update location |
| `DELETE` | `/observing-locations/{id}` | 🛡️ | Delete location |

---

### Targets (NASA)

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/targets/resolve` | 🔓 | Resolve target name via MAST; caches locally |
| `GET` | `/targets/{id}` | 🔓 | Get target by ID |
| `GET` | `/targets/{id}/tess-observations` | 🔓 | List TESS observations for target |
| `GET` | `/targets/{id}/light-curve-comparison` | 🔑 | Compare local observations vs NASA light curve |

---

### TESS Observations

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/tess-observations/{id}/discover-products` | 🔓 | Fetch available MAST data products |
| `POST` | `/tess-observations/{id}/fetch-light-curve` | 🔓 | Download and parse FITS light curve from MAST |
| `GET` | `/tess-observations/{id}/light-curve` | 🔓 | Get cached light curve points |

---

### Observation Sessions

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/observation-sessions` | 🛡️ | Create session |
| `GET` | `/observation-sessions` | 🔑 | List sessions |
| `GET` | `/observation-sessions/{id}` | 🔑 | Get session |
| `PUT` | `/observation-sessions/{id}` | 🛡️ | Update session |
| `DELETE` | `/observation-sessions/{id}` | 🛡️ | Delete session |
| `POST` | `/observation-sessions/{id}/photometry` | 🛡️ | Run aperture photometry on session images |

---

### Observation Images

| Method | Path | Auth | Description |
|---|---|---|---|
| `POST` | `/observation-sessions/{session_id}/images` | 🛡️ | Upload image (FITS / DNG / JPEG / PNG); extracts EXIF |
| `DELETE` | `/observation-sessions/{session_id}/images/{image_id}` | 🛡️ | Delete image |
