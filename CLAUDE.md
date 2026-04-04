# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Nyx is a data-driven astronomical observation platform that combines publicly available NASA space mission data (via open APIs) with ground-based optical observations captured using a telescope in Mauritius. The goal is to collect, validate, visualise, and manage astronomical time-series data, with emphasis on software architecture, data pipelines, and performance-aware system design.

## Key Design Goals

- Integrate NASA open astronomical data with locally captured telescope observations
- Cross-source data validation (comparing space-based vs ground-based measurements)
- Performant backend suitable for scientific workloads
- Modern, reactive frontend for data exploration
- Authenticated access to astronomical data

## Data Sources

- **Space-based**: NASA MAST (Mikulski Archive for Space Telescopes) and NASA Exoplanet Archive
- **Ground-based**: Optical observations from a telescope located in Mauritius
- These differ in scale, precision, cadence, and noise characteristics — integration is a core data engineering challenge
- NASA data is fetched and cached locally in PostgreSQL — no separate cache layer. Re-fetch on schedule or explicit request only

## Tech Stack

- **Backend**: Modern C++23 (GCC 13+ or Clang 17+)
- **Frontend**: Next.js (deferred — backend-first development)
- **Build system**: CMake
- **Package manager**: vcpkg
- **API style**: REST with versioned routes (`/api/v1/...`)
- **Authentication**: JWT
- **Configuration**: environment variables

## Backend Dependencies

- **HTTP framework**: Drogon — use `drogon_ctl` code generator for controllers and ORM model generation
- **ORM**: Drogon ORM for all database access. Keep the database layer behind abstractions so the underlying DB engine remains swappable
- **Database**: PostgreSQL (primary choice for time-series astronomical data)
- **Migrations**: goose — create via `goose -dir backend/migrations create <name> sql`, never manually number files
- **Logging**: spdlog
- **JSON**: nlohmann/json for serialization, simdjson for parsing high-volume NASA API responses
- **Formatting**: clang-format (Rust-style configuration)
- **Testing**: whichever framework integrates most easily with CMake (e.g. GoogleTest)

## Code Style & Conventions

- Clean code principles throughout
- **No `using namespace` — ever**
- No comments unless absolutely necessary — prefer long, descriptive variable/function/method/class names that make intent self-evident
- PascalCase for namespaces (e.g. `Nyx::Pipeline::`, `Nyx::Api::`), types/classes. snake_case for functions/variables
- Prefer `std::unique_ptr` / `std::shared_ptr` over raw pointers
- Use `this->member` to access class members — no trailing underscore convention
- Optimise aggressively: use move semantics, constexpr, compile-time evaluation, cache-friendly data structures, and threading where beneficial

### C++ Formatting (critical — do not rely on clang-format, it cannot achieve this style)
- 2-space indent, 80 character line limit, braces on same line (Attach)
- **Trailing return types everywhere**: `auto func() -> void` not `void func()`. No exceptions — includes overrides (`-> void override`)
- All content inside namespaces is indented (NamespaceIndentation: All)
- Access modifiers (public/private) are indented (IndentAccessModifiers: true)
- Long parameter lists: each param on its own line, closing paren on its own line before `-> ReturnType {`
- Constructor initializer lists: BeforeComma style

#### Line breaking rules (critical — do NOT break lines prematurely)
Only break a line when it actually exceeds 80 characters. If it fits, keep it on one line:
```cpp
// GOOD — fits on one line
callback(ResponseHelper::error(result.error(), correlation_id));
auto request_body = nlohmann::json::parse(request->body());

// BAD — unnecessary line breaks
callback(ResponseHelper::error(
  result.error(), correlation_id
));
auto request_body =
  nlohmann::json::parse(request->body());
```

#### Assignment with function call (when exceeding 80 chars)
Keep the function/constructor on the **same line** as `auto x =`, then break parameters below. Never move the entire function call to a new line:
```cpp
// GOOD — function stays on same line as assignment, params break below
auto validated = Nyx::Core::RequestValidator::validate(
  request_body, register_request_schema
);

auto result = this->auth_service->register_user(
  register_request, logger
);

auto register_request = Nyx::Application::Auth::RegisterRequest{
  .email = request_body["email"].get<std::string>(),
  .password = request_body["password"].get<std::string>(),
};

// BAD — function call moved to new line
auto validated =
  Nyx::Core::RequestValidator::validate(
    request_body, register_request_schema
  );

// BAD — designated initializer fields broken across multiple lines unnecessarily
auto register_request =
  Nyx::Application::Auth::RegisterRequest{
    .email =
      request_body["email"].get<std::string>(),
    .password =
      request_body["password"]
        .get<std::string>(),
  };
```

#### make_shared / constructor calls
```cpp
// GOOD
auto user_repository = std::make_shared<
  Nyx::Infrastructure::Persistence::PostgresUserRepository
>();

auto token_service = std::make_shared<
  Nyx::Infrastructure::Security::JwtTokenService
>(config);
```

## Error Handling & Logging

- **Errors must ALWAYS be handled — NEVER silently ignored**
- Use `std::expected` for recoverable errors in application code
- Library code (Drogon, etc.) may throw exceptions — always catch and handle these at boundaries
- All errors must be logged with context
- Use extensive debug logging — prefer too many debug logs over too few
- Log at appropriate levels (debug, info, warn, error) via spdlog
- **Request-scoped tracing**: every inbound HTTP request must be assigned a unique correlation ID at the Drogon middleware layer. This ID must appear in every subsequent log line for that request across all layers (controller, service, data access). When work crosses async/coroutine boundaries, explicitly pass the correlation ID — do not rely on thread-locals alone

## API Conventions

### Response Format
All API responses use a consistent JSON envelope:
```json
{
  "data": { ... },
  "meta": { "request_id": "correlation-id", "timestamp": "ISO8601" }
}
```

### Error Response Format
```json
{
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Human-readable description",
    "details": [{ "field": "email", "message": "must be valid email" }]
  },
  "meta": { "request_id": "correlation-id", "timestamp": "ISO8601" }
}
```
Server errors (5xx) must never expose internal details to clients — log the full error internally, return a generic message.

### REST Resource Naming
- Plural nouns for collections (`/observations`, `/targets`)
- Lowercase with hyphens (`/observation-sessions`, not `/observationSessions`)
- No verbs in URLs — use HTTP methods (`POST /observations`, not `POST /create-observation`)
- Actions that don't fit CRUD: `POST /observations/{id}/compare`

### Pagination
Use cursor-based pagination for list endpoints (more reliable than offset for real-time observation data):
```json
{
  "data": [...],
  "pagination": { "limit": 20, "next_cursor": "...", "has_more": true }
}
```

## Security

### Authentication
- JWT with short-lived access tokens (15 min) and longer refresh tokens (7 days)
- Refresh token rotation — issue a new refresh token on each use, revoke the family on reuse detection
- Password hashing with argon2 or bcrypt (cost >= 12)
- Rate limit login and password reset endpoints aggressively

### SSRF Protection
Since the backend fetches data from NASA APIs based on user-specified targets, validate and sanitise all outbound URLs. Only allow requests to known NASA API hostnames — never fetch arbitrary user-supplied URLs.

### Security Event Logging
Log all security-relevant events (login success/failure, permission denied, token revocation, rate limit hits) with user ID, IP, user-agent, and timestamp. These logs must never contain passwords, tokens, or API keys — redact sensitive fields.

### Input Validation
Validate all user input at the API boundary. Use parameterised queries exclusively — never interpolate user input into SQL strings (Drogon ORM handles this, but be vigilant with raw queries).

## Application Design

### Observation Sessions
- Users upload observation images (FITS for serious amateurs, JPEG/PNG with EXIF for casual users)
- Timestamps and metadata extracted automatically from image headers — no manual entry required
- User specifies the target (star/exoplanet name) when creating a session

### NASA Data Integration
- User-specified target is used to query MAST and Exoplanet Archive for matching data
- Fetched NASA data is cached in PostgreSQL — no plate-solving or auto-matching from images (out of scope)

### Users
- Multi-user platform with registration and JWT authentication
- Each user owns their observation sessions

### Processing
- Core feature: overlay local observations against NASA light curves for comparison/validation
- Start with viewing, plotting, and basic statistics
- Processing layer designed to be extensible for future additions (photometry, period folding, detrending)

### Frontend Features (Next.js — deferred until backend is complete)

**Core:**
- Sky Catalog Browser — searchable/filterable catalog of targets from MAST and Exoplanet Archive (type, magnitude, constellation)
- Target Detail Pages — NASA properties, light curves, and user observations overlaid on a single page per target
- Observation Planner — shows targets visible from user's location (default: Mauritius), altitude/timing, and upcoming exoplanet transits
- Interactive Light Curve Viewer — zoom, pan, toggle NASA vs local data layers, highlight transit windows
- Observation Journal — timeline of sessions with metadata, images, notes, and links to NASA comparison

**Nice to have:**
- Dashboard — personal stats (sessions, targets, hours), recent activity, upcoming transits
- Community Feed — public gallery of recent observations from other users
- Data Export — download merged local + NASA datasets in CSV/FITS

## Current State

The repository is in early stages. No source code has been committed yet. The `docs/` directory contains the project poster (poster.pdf).
