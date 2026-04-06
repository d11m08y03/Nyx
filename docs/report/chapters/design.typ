= System Design <design>

This chapter presents the complete system design of the Nyx platform. It covers the layered backend architecture and the rationale behind its decomposition, the relational database schema and its indexing strategy, the REST API surface, the multi-stage TESS data ingestion pipeline, the FITS binary table parsing subsystem, the image processing and photometry pipeline for ground-based observations, the astronomical time system conversions, the authentication and authorisation architecture, the HTTP middleware pipeline, the frontend architecture, and the trade-offs that shaped each decision.

== System Architecture <architecture>

The backend follows a clean architecture pattern @martin2017 organised into four concentric layers. Each layer has a single, well-defined responsibility, and dependencies point strictly inward: outer layers may depend on inner layers, but inner layers never reference outer ones. This constraint is the Dependency Inversion Principle @martin2000 applied at the package level, and it yields three practical benefits. First, the domain and application layers are entirely free of framework coupling --- they can be tested with in-memory fakes without starting a database or HTTP server. Second, swapping an infrastructure component (e.g. replacing PostgreSQL with another relational engine, or replacing CFITSIO with a different FITS library) requires changes only in the infrastructure layer; the application and domain layers remain untouched. Third, the compilation firewall created by the interface boundary reduces incremental rebuild times, since modifying an infrastructure implementation does not invalidate the application layer object files.

@architecture_diagram illustrates the four layers and their dependency directions.

#figure(
  rect(width: 100%, height: 220pt, stroke: 0.5pt)[
    #align(center + horizon)[_Architecture diagram --- concentric layers: Domain (innermost) #sym.arrow Application #sym.arrow Infrastructure #sym.arrow Presentation (outermost). Arrows show dependency direction inward._]
  ],
  caption: [Clean architecture layers of the Nyx backend. Dependencies point strictly inward.],
) <architecture_diagram>

=== Domain Layer

The domain layer is the innermost ring. It contains two categories of artefact: entity structs and repository interfaces.

Entity structs are plain C++ aggregates that model the core concepts of the system. They carry no behaviour beyond what the language provides for aggregate types (copy, move, aggregate initialisation). The entities defined in this layer are:

- `Target` --- a resolved astronomical object with a canonical name, sky coordinates (right ascension and declination), and a target type (e.g. "Star", "Exoplanet Host").
- `TessObservation` --- a single TESS observation sector record with an observation identifier (`obsid`), cadence in seconds, start and end times (Modified Julian Date), and an optional `data_uri` pointing to the FITS light curve file.
- `LightCurvePoint` --- a single time-series data point with a timestamp (Barycentric TESS Julian Date), optional PDCSAP and SAP flux values, optional PDCSAP flux error, and an integer quality flag.
- `User` --- an account record with email, optional password hash (nullable for OAuth users), display name, authentication provider, optional Google ID, and email verification status.
- `RefreshToken` --- a token record with a SHA-256 hash, a family identifier for rotation tracking, a revocation flag, and an expiry timestamp.
- `VerificationToken` --- an email verification token with a SHA-256 hash, expiry, and optional `used_at` timestamp.
- `ObservationSession` --- a ground-based observation session linking a user, a target, and the equipment used (telescope, camera, mount, filter, observing location).
- `ObservationImage` --- an uploaded image with file metadata (path, size, MIME type), EXIF-extracted fields (capture time, camera model, exposure, ISO, GPS coordinates, dimensions), and optional photometry results (target pixel coordinates, raw flux, relative flux, status).
- `Telescope`, `Camera`, `Mount`, `Filter` --- equipment records owned by a user, each with type-specific attributes (aperture, focal length, sensor type, band, etc.).
- `ObservingLocation` --- a geographic location with latitude, longitude, and Bortle class.

Repository interfaces define the data access contracts that the application layer relies on. Each interface is a pure abstract class with a virtual destructor and one or more pure virtual methods. For example, `ITargetRepository` declares methods `create`, `find_by_id`, `find_by_canonical_name`, and `find_all`. `ITessObservationRepository` declares `bulk_create`, `find_by_target_id`, `find_by_id`, `find_existing_obsids`, and `update_data_uri`. `ILightCurvePointRepository` declares `bulk_create`, `find_by_observation_id`, `find_by_observation_id_as_json`, `count_by_observation_id`, and `delete_by_observation_id`. There are 13 repository interfaces in total, one for each aggregate root or entity that requires persistence.

The domain layer has zero dependencies on external libraries. It uses only the C++ standard library (`std::string`, `std::optional`, `std::vector`, `std::expected`). This makes it the most stable layer in the system --- it changes only when the core data model changes.

=== Application Layer

The application layer sits immediately outside the domain layer. It contains service classes that implement the system's use cases. Each service orchestrates domain entities and repository interfaces to fulfil a single business capability. The services defined in this layer are:

- `TargetService` --- resolves target names via the MAST API, searches for TESS observations, discovers FITS data products, downloads and parses light curves, and retrieves stored light curve data. This is the most complex service, implementing the four-stage data ingestion pipeline described in @data_pipeline.
- `AuthService` --- handles user registration, login (email/password and Google OAuth2), token refresh with rotation, logout (token revocation), email verification, and verification email resend.
- `EquipmentService` --- provides CRUD operations for telescopes, cameras, mounts, and filters, all scoped to the authenticated user.
- `LocationService` --- provides CRUD operations for observing locations, scoped to the authenticated user.
- `ObservationService` --- manages observation sessions and image uploads, including EXIF metadata extraction, DNG raw image decoding, and aperture photometry.
- `ProfileService` --- handles user profile completion (setting a display name after OAuth registration).
- `LightCurveComparisonService` --- retrieves TESS light curve data alongside the user's ground-based photometry results for the same target, converting user observation timestamps to the TESS time system for overlay comparison.

Services depend on interfaces, never on concrete implementations. For example, `TargetService` depends on `IMastClient` (an application-layer interface for the MAST API), `IFitsParser` (an application-layer interface for FITS parsing), `ITargetRepository`, `ITessObservationRepository`, `ILightCurvePointRepository`, and `IUuidGenerator`. All of these are abstract classes. The concrete implementations (`MastClient`, `FitsParser`, `PostgresTargetRepository`, etc.) are provided at application startup via constructor injection.

The application layer also defines Data Transfer Objects (DTOs) --- request structs (e.g. `ResolveTargetRequest`, `RegisterRequest`, `LoginRequest`) and response structs (e.g. `TargetResponse`, `TessObservationResponse`, `LightCurveResponse`). These DTOs decouple the API contract from the domain model. A `TargetResponse`, for example, includes a nested `std::vector<TessObservationResponse>` that the domain `Target` entity does not carry, since the entity models only the target itself while the response aggregates related data for the client.

=== Infrastructure Layer

The infrastructure layer provides all concrete implementations of the interfaces declared in the domain and application layers. It is the widest ring in terms of external dependencies and the most likely to change when third-party libraries or services evolve. The implementations grouped by concern are:

*Persistence* --- thirteen PostgreSQL repository classes, one per repository interface. Each uses the Drogon ORM @drogon2024 for database access. Parameterised queries are used exclusively; no user input is ever interpolated into SQL strings. Bulk insert operations (e.g. for light curve points) use dynamically constructed multi-row `INSERT` statements with positional parameters (`$1`, `$2`, ...) to minimise round-trips. A single bulk insert can handle up to 5,000 rows per batch within a database transaction.

*NASA API client* --- `MastClient` implements `IMastClient`. It communicates with the MAST Portal API @stsdci2024 using Drogon's synchronous HTTP client. All MAST API calls use the form-encoded POST convention required by the MAST `/api/v0/invoke` endpoint: the JSON request payload is URL-encoded and sent as the value of a `request` form field with content type `application/x-www-form-urlencoded`. The response is standard JSON.

*FITS parsing* --- `FitsParser` implements `IFitsParser`. It uses the CFITSIO library to read binary FITS files, extracting TIME, PDCSAP_FLUX, PDCSAP_FLUX_ERR, SAP_FLUX, and QUALITY columns from the second HDU (Header Data Unit). Because CFITSIO requires file-based access and cannot parse in-memory buffers, the parser writes the binary data to a temporary file (via `mkstemp`), reads the columns, and cleans up the file with an RAII guard class.

*Security* --- `JwtTokenService` implements `ITokenService` using the jwt-cpp library for HS256 JWT generation and verification. `ArgonPasswordHasher` implements `IPasswordHasher` using libsodium's Argon2id implementation with `MODERATE` parameters @biryukov2016.

*Authentication* --- `GoogleAuthClient` implements `IGoogleAuthClient` for the OAuth2 Authorization Code exchange with Google's token endpoint @hardt2012.

*Email* --- `SmtpEmailSender` and `ConsoleEmailSender` both implement `IEmailSender`. The console variant logs emails to standard output for development environments.

*Imaging* --- `Exiv2ExifParser` implements `IExifParser` using the Exiv2 library for EXIF metadata extraction. `LibrawDngDecoder` implements `IDngDecoder` using the LibRaw library for DNG raw image decoding. `AperturePhotometer` implements `IPhotometryProcessor` for aperture photometry calculations.

*Configuration* --- `EnvironmentConfig` reads all configuration from environment variables (database connection string, JWT secret, server port, thread count, Google OAuth credentials, SMTP settings, log level). No configuration files are used.

*Storage* --- `LocalFileStorage` implements `IFileStorage` for writing uploaded images to the local filesystem.

=== Presentation Layer

The presentation layer is the outermost ring. It handles HTTP request parsing, response formatting, and cross-cutting concerns (authentication, rate limiting, CSRF protection, request tracing). It contains two sub-layers: controllers and middleware.

Controllers are Drogon `HttpController` subclasses. Each controller method performs three operations: (1) parse the request body and path parameters, (2) call the appropriate application service method, and (3) format the result into a JSON response using the `ResponseHelper` utility class. Controllers never contain business logic. If a service method returns an error (via `std::unexpected`), the controller passes it directly to `ResponseHelper::error()`, which maps the `ErrorCode` enum to the appropriate HTTP status code and JSON error envelope.

The `ResponseHelper` class enforces the response envelope contract. Every successful response is wrapped in a `{"data": ..., "meta": {"request_id": "...", "timestamp": "..."}}` structure. Every error response is wrapped in a `{"error": {"code": "...", "message": "...", "details": [...]}, "meta": {...}}` structure. The `meta.request_id` field carries the correlation ID assigned by the `CorrelationIdFilter`, providing end-to-end request traceability.

=== Constructor Injection Pattern

All dependencies are wired at application startup using `std::shared_ptr` constructor injection. The `main.cpp` entry point creates the `EnvironmentConfig`, initialises the database connection pool via `DatabaseManager::initialize()`, and starts the Drogon HTTP server. Drogon's controller registration is declarative (via `METHOD_LIST_BEGIN` / `ADD_METHOD_TO` macros), and each controller's constructor resolves its service dependencies by constructing the full dependency graph.

For example, `TargetController`'s constructor creates a `TargetService`, which in turn receives a `MastClient`, a `FitsParser`, a `PostgresTargetRepository`, a `PostgresTessObservationRepository`, a `PostgresLightCurvePointRepository`, and a `DrogonUuidGenerator` --- all as `std::shared_ptr` to their respective interface types. This wiring is explicit and compile-time-checked. There is no service locator or runtime dependency injection container.

The use of `std::shared_ptr` rather than `std::unique_ptr` is a deliberate choice. Several services share repository instances (e.g. `TargetService` and `LightCurveComparisonService` both need `ITargetRepository` and `ITessObservationRepository`), and shared ownership simplifies the wiring without introducing lifetime ambiguity.

== Directory Structure <directory_structure>

The source tree mirrors the architecture layers described above:

```
src/
+-- core/
|   +-- error/              AppError, ErrorCode, Result<T>
|   +-- logging/            Logger (spdlog initialisation)
|   +-- util/               TimeConversion, UuidGenerator
|   +-- validation/         RequestValidator (JSON schema)
+-- domain/
|   +-- entities/           Target, User, TessObservation,
|   |                       LightCurvePoint, ObservationSession,
|   |                       ObservationImage, Telescope, Camera,
|   |                       Mount, Filter, ObservingLocation,
|   |                       RefreshToken, VerificationToken
|   +-- repositories/       ITargetRepository, IUserRepository,
|                           ITessObservationRepository,
|                           ILightCurvePointRepository,
|                           IObservationSessionRepository,
|                           IObservationImageRepository,
|                           ITelescopeRepository, ICameraRepository,
|                           IMountRepository, IFilterRepository,
|                           IObservingLocationRepository,
|                           IRefreshTokenRepository,
|                           IVerificationTokenRepository
+-- application/
|   +-- auth/               AuthService, IPasswordHasher,
|   |                       ITokenService, IEmailSender,
|   |                       IGoogleAuthClient, Dtos
|   +-- target/             TargetService, IMastClient,
|   |                       IFitsParser, LightCurveComparisonService,
|   |                       Dtos
|   +-- equipment/          EquipmentService, Dtos
|   +-- location/           LocationService, Dtos
|   +-- observation/        ObservationService, IExifParser,
|   |                       IDngDecoder, IPhotometryProcessor,
|   |                       IFileStorage, Dtos
|   +-- profile/            ProfileService, Dtos
+-- infrastructure/
|   +-- nasa/               MastClient, FitsParser
|   +-- persistence/        PostgresTargetRepository,
|   |                       PostgresUserRepository,
|   |                       PostgresTessObservationRepository,
|   |                       PostgresLightCurvePointRepository,
|   |                       PostgresObservationSessionRepository,
|   |                       PostgresObservationImageRepository,
|   |                       PostgresTelescopeRepository,
|   |                       PostgresCameraRepository,
|   |                       PostgresMountRepository,
|   |                       PostgresFilterRepository,
|   |                       PostgresObservingLocationRepository,
|   |                       PostgresRefreshTokenRepository,
|   |                       PostgresVerificationTokenRepository
|   +-- security/           JwtTokenService, ArgonPasswordHasher
|   +-- auth/               GoogleAuthClient
|   +-- email/              SmtpEmailSender, ConsoleEmailSender
|   +-- config/             EnvironmentConfig
|   +-- imaging/            Exiv2ExifParser, LibrawDngDecoder,
|   |                       AperturePhotometer
|   +-- storage/            LocalFileStorage
|   +-- database/           DatabaseManager
|   +-- util/               DrogonUuidGenerator
+-- presentation/
    +-- http/
    |   +-- auth/           AuthController, AuthSchemas
    |   +-- target/         TargetController, TessObservationController,
    |   |                   LightCurveComparisonController,
    |   |                   TargetSchemas
    |   +-- equipment/      TelescopeController, CameraController,
    |   |                   MountController, FilterController,
    |   |                   EquipmentSchemas
    |   +-- location/       LocationController, LocationSchemas
    |   +-- observation/    ObservationSessionController,
    |   |                   ObservationImageController,
    |   |                   ObservationSchemas
    |   +-- profile/        ProfileController, ProfileSchemas
    |   +-- ResponseHelper
    +-- middleware/
        +-- CorrelationIdFilter
        +-- RateLimitFilter
        +-- JwtAuthFilter
        +-- CsrfFilter
```

The `core/` directory is separate from `domain/` because it contains cross-cutting utilities (error types, logging, validation) that are used by all layers. The `core/error/AppError.hpp` header defines the `Result<T>` type alias (`std::expected<T, AppError>` @cppreference2024), the `ErrorCode` enumeration (13 error codes mapping to HTTP status codes 400--500), and factory methods (`AppError::validation()`, `AppError::not_found()`, `AppError::internal()`, etc.) that standardise error construction across the codebase.

== Database Schema Design <database_schema>

The database consists of 18 tables managed by 18 goose migration files @postgresql2024. Tables are grouped into four domains: authentication, targets, equipment, and observations. @erd shows the entity-relationship diagram.

#figure(
  rect(width: 100%, height: 300pt, stroke: 0.5pt)[
    #align(center + horizon)[_Entity-relationship diagram --- 18 tables across four domains. users #sym.arrow.l refresh\_tokens, verification\_tokens. targets #sym.arrow.l tess\_observations #sym.arrow.l light\_curve\_points. users #sym.arrow.l telescopes, cameras, mounts, filters, observing\_locations. users #sym.arrow.l observation\_sessions #sym.arrow.l observation\_images. observation\_sessions references targets and all equipment tables._]
  ],
  caption: [Entity-relationship diagram of the Nyx database schema.],
) <erd>

=== Authentication Domain

The authentication domain manages user accounts, refresh tokens for session management, and email verification tokens.

*`users` table:*

```sql
CREATE TABLE users (
  id UUID PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  password_hash VARCHAR(255),
  display_name VARCHAR(100),
  email_verified BOOLEAN NOT NULL DEFAULT FALSE,
  auth_provider VARCHAR(20) NOT NULL DEFAULT 'local',
  google_id VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_users_email ON users(email);
CREATE UNIQUE INDEX idx_users_google_id
  ON users(google_id) WHERE google_id IS NOT NULL;
```

The `password_hash` column is nullable to support OAuth-only accounts where no password is set. The `auth_provider` column distinguishes between `'local'` (email/password) and `'google'` (OAuth) accounts. The partial unique index on `google_id` ensures that each Google account maps to at most one Nyx user, while allowing multiple rows with `NULL` Google IDs (the default for local accounts). This is more space-efficient than a standard unique constraint, which would require a sentinel value for non-Google users.

*`refresh_tokens` table:*

```sql
CREATE TABLE refresh_tokens (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  token_hash VARCHAR(255) NOT NULL UNIQUE,
  family_id UUID NOT NULL,
  is_revoked BOOLEAN NOT NULL DEFAULT FALSE,
  expires_at TIMESTAMPTZ NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_refresh_tokens_user_id ON refresh_tokens(user_id);
CREATE INDEX idx_refresh_tokens_family_id ON refresh_tokens(family_id);
CREATE INDEX idx_refresh_tokens_token_hash ON refresh_tokens(token_hash);
```

The `token_hash` column stores the SHA-256 hash of the raw refresh token --- the raw token is never persisted. The `family_id` groups all tokens in a rotation chain. When a token is refreshed, a new token is created with the same `family_id`, and the old token's `is_revoked` flag is set to `TRUE`. If a previously revoked token is presented (indicating token theft), all tokens sharing the `family_id` are revoked. The `token_hash` index enables O(1) lookup during the refresh flow. The `family_id` index enables efficient bulk revocation of an entire token family.

*`verification_tokens` table:*

```sql
CREATE TABLE verification_tokens (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  token_hash VARCHAR(255) NOT NULL UNIQUE,
  expires_at TIMESTAMPTZ NOT NULL,
  used_at TIMESTAMPTZ,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_verification_tokens_user_id
  ON verification_tokens(user_id);
CREATE INDEX idx_verification_tokens_token_hash
  ON verification_tokens(token_hash);
```

Like refresh tokens, verification tokens are stored as SHA-256 hashes. The `used_at` column is set when the token is consumed, preventing reuse. Expired or used tokens are retained for auditing rather than deleted.

=== Target Domain

The target domain stores resolved astronomical targets, their TESS observation metadata, and the parsed light curve time-series data.

*`targets` table:*

```sql
CREATE TABLE targets (
  id UUID PRIMARY KEY,
  canonical_name VARCHAR(255) NOT NULL UNIQUE,
  target_type VARCHAR(50),
  right_ascension DOUBLE PRECISION,
  declination DOUBLE PRECISION,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
```

The `canonical_name` is the MAST-resolved name (e.g. "HAT-P-7" rather than the user's input "hatp7" or "HAT-P-7b"). The unique constraint ensures that repeated resolution of the same target returns the cached record rather than creating duplicates. Right ascension and declination are stored as DOUBLE PRECISION in degrees, matching the MAST API coordinate system. Both are nullable because name resolution occasionally succeeds without returning coordinates (rare, but the schema must tolerate it).

*`tess_observations` table:*

```sql
CREATE TABLE tess_observations (
  id UUID PRIMARY KEY,
  target_id UUID NOT NULL REFERENCES targets(id)
    ON DELETE CASCADE,
  obsid VARCHAR(20) NOT NULL UNIQUE,
  cadence_seconds INTEGER NOT NULL,
  start_time DOUBLE PRECISION NOT NULL,
  end_time DOUBLE PRECISION NOT NULL,
  data_uri VARCHAR(500),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_tess_observations_target_id
  ON tess_observations(target_id);
```

The `obsid` column carries a `UNIQUE` constraint. This is the MAST-assigned observation identifier and serves as the natural key for idempotent ingestion: when the pipeline re-runs for a target that already has observations in the database, the service queries `find_existing_obsids()` with the list of obsids returned by MAST and skips any that already exist. This approach avoids `ON CONFLICT` clauses and gives the application full control over what constitutes a "new" observation.

The `start_time` and `end_time` columns store Modified Julian Dates (MJD) as DOUBLE PRECISION values. MJD was chosen over TIMESTAMPTZ because the MAST API returns observation time bounds as MJD floats, and storing them in the same format avoids lossy conversion. The `data_uri` column is nullable because it is populated in a separate pipeline stage (product discovery) after the observation record is initially created during the observation search stage.

*`light_curve_points` table:*

```sql
CREATE TABLE light_curve_points (
  id BIGSERIAL PRIMARY KEY,
  tess_observation_id UUID NOT NULL
    REFERENCES tess_observations(id) ON DELETE CASCADE,
  time DOUBLE PRECISION NOT NULL,
  pdcsap_flux REAL,
  pdcsap_flux_err REAL,
  sap_flux REAL,
  quality INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX idx_lcp_observation_id
  ON light_curve_points(tess_observation_id);
CREATE INDEX idx_lcp_observation_time
  ON light_curve_points(tess_observation_id, time);
```

This table stores the parsed FITS light curve data. A single TESS observation sector contains approximately 18,000 to 20,000 data points at 2-minute cadence, or approximately 600 to 700 points at 30-minute cadence. For a target observed across 10 sectors, this amounts to roughly 180,000 to 200,000 rows. At scale (hundreds of targets), the table could reach tens of millions of rows.

The primary key uses `BIGSERIAL` rather than `UUID` for two reasons. First, the auto-incrementing integer avoids the 16-byte storage overhead of UUID per row, which matters when the table contains millions of rows with narrow columns (the entire row is approximately 40 bytes excluding the UUID overhead). Second, `BIGSERIAL` produces naturally ordered keys that align with B-tree index leaf page ordering, reducing page splits during sequential inserts @winand2012.

The `time` column stores Barycentric TESS Julian Date (BTJD) as DOUBLE PRECISION. The `pdcsap_flux`, `pdcsap_flux_err`, and `sap_flux` columns use REAL (4-byte single-precision float) rather than DOUBLE PRECISION because the source FITS data stores these columns as 32-bit floats, and there is no information gain from widening to 64 bits. The `quality` column is an integer bitmask defined by the TESS SPOC pipeline @jenkins2016; a value of zero means no known data quality issues.

The composite index `idx_lcp_observation_time` on `(tess_observation_id, time)` is the most performance-critical index in the schema. It supports the primary query pattern: retrieve all light curve points for a given observation, ordered by time. Because `tess_observation_id` is the leading column, PostgreSQL can use an index-only scan (if the query selects only indexed columns) or an index scan with a single seek followed by a sequential read of the leaf pages. The `ORDER BY time ASC` clause in the query aligns with the index sort order, so no in-memory sort is required @winand2012.

=== Equipment Domain

The equipment domain stores user-owned astronomical equipment.

*`telescopes` table:*

```sql
CREATE TABLE telescopes (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id)
    ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  aperture_mm INTEGER NOT NULL,
  focal_length_mm INTEGER NOT NULL,
  optical_design VARCHAR(50) NOT NULL,
  brand VARCHAR(255),
  model VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_telescopes_user_id ON telescopes(user_id);
```

*`cameras` table:*

```sql
CREATE TABLE cameras (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id)
    ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  sensor_type VARCHAR(50) NOT NULL,
  brand VARCHAR(255),
  model VARCHAR(255),
  pixel_size_um DOUBLE PRECISION,
  resolution VARCHAR(50),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_cameras_user_id ON cameras(user_id);
```

*`mounts` table:*

```sql
CREATE TABLE mounts (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id)
    ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  mount_type VARCHAR(50) NOT NULL,
  is_tracked BOOLEAN NOT NULL DEFAULT FALSE,
  has_goto BOOLEAN NOT NULL DEFAULT FALSE,
  brand VARCHAR(255),
  model VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_mounts_user_id ON mounts(user_id);
```

*`filters` table:*

```sql
CREATE TABLE filters (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id)
    ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  band VARCHAR(50) NOT NULL,
  bandwidth_nm DOUBLE PRECISION,
  brand VARCHAR(255),
  model VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_filters_user_id ON filters(user_id);
```

All equipment tables share the same structural pattern: UUID primary key, foreign key to `users(id)` with `ON DELETE CASCADE`, a user-assigned name, type-specific attributes, and timestamps. A unique index on `(user_id, name)` is applied to observing locations (and could be extended to equipment tables) to prevent a user from creating two items with the same name.

=== Observation Domain

The observation domain links ground-based observations to targets and equipment.

*`observing_locations` table:*

```sql
CREATE TABLE observing_locations (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id)
    ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  latitude DOUBLE PRECISION NOT NULL,
  longitude DOUBLE PRECISION NOT NULL,
  bortle_class SMALLINT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_observing_locations_user_id
  ON observing_locations(user_id);
CREATE UNIQUE INDEX idx_observing_locations_user_id_name
  ON observing_locations(user_id, name);
```

The `bortle_class` column uses SMALLINT (1--9 scale) to record the sky brightness at the location. The unique index on `(user_id, name)` prevents duplicate location names per user, returning a 409 Conflict error at the database level rather than requiring an application-level check.

*`observation_sessions` table:*

```sql
CREATE TABLE observation_sessions (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id)
    ON DELETE CASCADE,
  target_id UUID NOT NULL REFERENCES targets(id),
  telescope_id UUID NOT NULL REFERENCES telescopes(id),
  camera_id UUID NOT NULL REFERENCES cameras(id),
  mount_id UUID NOT NULL REFERENCES mounts(id),
  location_id UUID NOT NULL
    REFERENCES observing_locations(id),
  filter_id UUID REFERENCES filters(id),
  notes TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_observation_sessions_user_id
  ON observation_sessions(user_id);
CREATE INDEX idx_observation_sessions_target_id
  ON observation_sessions(target_id);
```

The session table links a user, a target, and the complete equipment configuration used for the observation. The `filter_id` column is the only nullable equipment reference, because unfiltered observations are common in amateur astronomy. All other equipment references are `NOT NULL` because a meaningful ground-based observation requires at minimum a telescope, camera, mount, and location.

*`observation_images` table:*

```sql
CREATE TABLE observation_images (
  id UUID PRIMARY KEY,
  session_id UUID NOT NULL
    REFERENCES observation_sessions(id) ON DELETE CASCADE,
  filename VARCHAR(255) NOT NULL,
  original_filename VARCHAR(255) NOT NULL,
  file_path VARCHAR(1024) NOT NULL,
  file_size_bytes BIGINT NOT NULL,
  mime_type VARCHAR(50) NOT NULL,
  captured_at TIMESTAMPTZ,
  camera_model VARCHAR(255),
  exposure_time_seconds DOUBLE PRECISION,
  iso_speed INTEGER,
  gps_latitude DOUBLE PRECISION,
  gps_longitude DOUBLE PRECISION,
  image_width INTEGER,
  image_height INTEGER,
  target_x INTEGER,
  target_y INTEGER,
  raw_flux DOUBLE PRECISION,
  raw_flux_error DOUBLE PRECISION,
  relative_flux DOUBLE PRECISION,
  relative_flux_error DOUBLE PRECISION,
  photometry_status VARCHAR(20),
  photometry_error_message TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_observation_images_session_id
  ON observation_images(session_id);
```

The image table stores both the file metadata and the EXIF-extracted metadata (first group of nullable columns: `captured_at` through `image_height`) and the photometry results (second group: `target_x` through `photometry_error_message`). The `filename` column stores a UUID-based name generated on upload to avoid filesystem collisions; `original_filename` preserves the name the user uploaded. The `photometry_status` column can be `'pending'`, `'completed'`, or `'failed'`, tracking the state of the asynchronous photometry pipeline. When photometry fails, `photometry_error_message` records the reason.

=== Why PostgreSQL <why_postgresql>

PostgreSQL @postgresql2024 was chosen over specialised time-series databases (TimescaleDB @timescale2024, InfluxDB) for three reasons. First, the data volume --- tens of millions of light curve points at most --- is well within PostgreSQL's capability when properly indexed. TimescaleDB's hypertable partitioning provides significant benefit only at billions of rows or when time-range deletions (chunk dropping) are frequent; neither condition applies here. Second, the system requires relational modelling beyond time-series: users, sessions, equipment, tokens, and images all have complex relationships with foreign keys and join requirements that relational databases handle natively but time-series databases handle poorly or not at all. Third, using a single database engine simplifies deployment, backup, monitoring, and developer onboarding. The composite index `(tess_observation_id, time)` provides the range-scan performance that a hypertable would otherwise offer, as confirmed by query plan analysis in @testing.

== API Design <api_design>

All endpoints are versioned under `/api/v1`, following Fielding's REST architectural style @fielding2000. Resource names use plural nouns with hyphens for multi-word names (e.g. `/observation-sessions`, `/tess-observations`). HTTP methods map to operations: `GET` for retrieval, `POST` for creation and actions, `PUT` for full replacement, `DELETE` for removal. Actions that do not fit the CRUD model use `POST` on a sub-resource (e.g. `POST /tess-observations/:id/discover-products`).

@api_endpoints_full lists all 28 endpoints.

#figure(
  table(
    columns: (auto, auto, auto, auto),
    align: (left, left, left, left),
    stroke: 0.5pt,
    inset: 5pt,
    table.header([*Method*], [*Route*], [*Auth*], [*Description*]),
    table.cell(colspan: 4, fill: luma(230), [*Authentication*]),
    [POST], [`/auth/register`], [No], [Register with email and password],
    [POST], [`/auth/login`], [No], [Login, returns access token],
    [POST], [`/auth/refresh`], [Cookie], [Refresh access token],
    [POST], [`/auth/logout`], [Cookie], [Revoke refresh token],
    [POST], [`/auth/verify-email`], [No], [Verify email address],
    [POST], [`/auth/resend-verification`], [No], [Resend verification email],
    [POST], [`/auth/google`], [No], [Google OAuth2 code exchange],
    table.cell(colspan: 4, fill: luma(230), [*Profile*]),
    [PUT], [`/users/me/profile`], [JWT], [Complete onboarding profile],
    table.cell(colspan: 4, fill: luma(230), [*Targets*]),
    [POST], [`/targets/resolve`], [No], [Resolve target name via MAST],
    [GET], [`/targets/:id`], [No], [Get target with observations],
    [GET], [`/targets/:id/tess-observations`], [No], [List TESS observations],
    [GET], [`/targets/:id/light-curve-comparison`], [JWT], [Compare TESS and user data],
    table.cell(colspan: 4, fill: luma(230), [*TESS Observations*]),
    [POST], [`/tess-observations/:id/discover-products`], [No], [Discover FITS products],
    [POST], [`/tess-observations/:id/fetch-light-curve`], [No], [Download and parse FITS],
    [GET], [`/tess-observations/:id/light-curve`], [No], [Get stored light curve],
    table.cell(colspan: 4, fill: luma(230), [*Equipment (CRUD --- each resource)*]),
    [POST], [`/telescopes`], [JWT+CSRF], [Create telescope],
    [GET], [`/telescopes`], [JWT], [List user's telescopes],
    [GET], [`/telescopes/:id`], [JWT], [Get telescope],
    [PUT], [`/telescopes/:id`], [JWT+CSRF], [Update telescope],
    [DELETE], [`/telescopes/:id`], [JWT+CSRF], [Delete telescope],
    table.cell(colspan: 4, fill: luma(230), [_(Cameras, Mounts, Filters follow the same CRUD pattern)_]),
    table.cell(colspan: 4, fill: luma(230), [*Locations*]),
    [POST], [`/observing-locations`], [JWT+CSRF], [Create location],
    [GET], [`/observing-locations`], [JWT], [List locations],
    [GET/PUT/DELETE], [`/observing-locations/:id`], [JWT/CSRF], [Read/Update/Delete],
    table.cell(colspan: 4, fill: luma(230), [*Observations*]),
    [POST], [`/observation-sessions`], [JWT+CSRF], [Create session],
    [GET], [`/observation-sessions`], [JWT], [List user's sessions],
    [GET/PUT/DELETE], [`/observation-sessions/:id`], [JWT/CSRF], [Read/Update/Delete],
    [POST], [`/observation-sessions/:id/images`], [JWT+CSRF], [Upload image],
    [DELETE], [`/observation-sessions/:sid/images/:iid`], [JWT+CSRF], [Delete image],
    [POST], [`/observation-sessions/:id/photometry`], [JWT+CSRF], [Run photometry],
  ),
  caption: [Complete API endpoint listing. "JWT" means the `JwtAuthFilter` is applied. "CSRF" means the `CsrfFilter` is additionally applied. "Cookie" means the refresh token is read from an HttpOnly cookie.],
) <api_endpoints_full>

=== Response Envelope

All responses use a consistent JSON envelope. Successful responses:

```json
{
  "data": {
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "canonical_name": "HAT-P-7",
    "target_type": "Star",
    "right_ascension": 292.45638,
    "declination": 47.96946
  },
  "meta": {
    "request_id": "a3b2c1d0-...",
    "timestamp": "2026-04-05T14:30:00"
  }
}
```

Error responses replace `data` with `error`:

```json
{
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Invalid request body",
    "details": [
      { "field": "email", "message": "must be a valid email" },
      { "field": "password", "message": "must be at least 8 characters" }
    ]
  },
  "meta": {
    "request_id": "a3b2c1d0-...",
    "timestamp": "2026-04-05T14:30:00"
  }
}
```

The `meta.request_id` field carries the correlation ID generated by the `CorrelationIdFilter` middleware. This ID appears in every log line produced during the request, enabling end-to-end tracing from the client through the controller, service, and repository layers. The `details` array is populated only for validation errors and contains per-field error messages. Server errors (5xx) never expose internal details to clients --- the service logs the full error context internally and returns a generic `"INTERNAL_ERROR"` message.

The `AppError` struct defines 13 error codes that map to HTTP status codes:

#figure(
  table(
    columns: (auto, auto, auto),
    align: (left, left, left),
    stroke: 0.5pt,
    inset: 5pt,
    table.header([*Error Code*], [*HTTP Status*], [*Usage*]),
    [`VALIDATION_ERROR`], [400], [Invalid input data],
    [`INVALID_JSON`], [400], [Malformed JSON body],
    [`AUTHENTICATION_REQUIRED`], [401], [Missing or invalid credentials],
    [`INVALID_TOKEN`], [401], [Malformed or tampered JWT],
    [`TOKEN_EXPIRED`], [401], [Expired JWT],
    [`PERMISSION_DENIED`], [403], [CSRF failure or access denied],
    [`EMAIL_NOT_VERIFIED`], [403], [Unverified email address],
    [`RESOURCE_NOT_FOUND`], [404], [Entity not found],
    [`CONFLICT`], [409], [Duplicate resource],
    [`RATE_LIMIT_EXCEEDED`], [429], [Too many requests],
    [`INTERNAL_ERROR`], [500], [Unrecoverable server error],
    [`DATABASE_ERROR`], [500], [Database failure],
    [`EXTERNAL_SERVICE_ERROR`], [500], [MAST API or external call failure],
  ),
  caption: [Error code to HTTP status code mapping.],
) <error_codes>

=== Pagination

The API does not implement cursor-based pagination. At the current scale --- a user typically has fewer than 50 equipment items, fewer than 100 observation sessions, and a target typically has fewer than 30 TESS observations --- all list endpoints return complete result sets. The light curve endpoint, which can return tens of thousands of points, uses a streaming JSON approach (PostgreSQL's `json_agg` constructs the JSON array server-side in a single query) rather than pagination, since the frontend needs all data points to render the complete light curve. If the platform scales to thousands of users with large observation histories, pagination would be added to the session and equipment list endpoints as offset-based pagination (simpler to implement for infrequently-changing datasets) rather than cursor-based.

== Data Ingestion Pipeline Design <data_pipeline>

The TESS data ingestion pipeline is a four-stage ETL process that transforms a user-provided target name into a set of persisted light curve time-series records. The pipeline is implemented in `TargetService` and `TessObservationController`, with the MAST API communication delegated to `IMastClient` and the FITS parsing delegated to `IFitsParser`. @pipeline_diagram illustrates the stages.

#figure(
  rect(width: 100%, height: 180pt, stroke: 0.5pt)[
    #align(center + horizon)[_Pipeline flow diagram --- Stage 1: Name Resolution (Mast.Name.Lookup) #sym.arrow Stage 2: Observation Search (Mast.Caom.Filtered.Position) #sym.arrow Stage 3: Product Discovery (Mast.Caom.Products) #sym.arrow Stage 4: FITS Download and Parse (HTTP GET + CFITSIO). Each stage shows input, API call, and persisted output._]
  ],
  caption: [Four-stage TESS data ingestion pipeline.],
) <pipeline_diagram>

=== Stage 1: Name Resolution

The user provides a target name (e.g. "HAT-P-7", "pi Mensae", "TIC 261136679"). The `TargetService::resolve_target()` method first checks whether a target with the same canonical name already exists in the `targets` table. If it does, the cached record is returned immediately, avoiding a redundant API call.

If the target is not cached, `IMastClient::resolve_target()` calls the MAST `Mast.Name.Lookup` service. The request payload is:

```json
{
  "service": "Mast.Name.Lookup",
  "params": { "input": "HAT-P-7", "format": "json" }
}
```

This JSON is serialised, URL-encoded, and sent as the `request` parameter in a form-encoded POST body to `https://mast.stsci.edu/api/v0/invoke`. The MAST API requires this unusual encoding: it does not accept JSON directly in the request body @stsdci2024.

The response contains a `resolvedCoordinate` array. The first element provides the canonical name, object type, right ascension (`ra`), and declination (`decl`). These are extracted and persisted to the `targets` table.

=== Stage 2: Observation Search

Using the resolved sky coordinates, `IMastClient::search_tess_timeseries()` calls the MAST `Mast.Caom.Filtered.Position` service. The request specifies:

```json
{
  "service": "Mast.Caom.Filtered.Position",
  "params": {
    "columns": "obsid,t_exptime,t_min,t_max",
    "filters": [
      { "paramName": "obs_collection", "values": ["TESS"] },
      { "paramName": "dataproduct_type", "values": ["timeseries"] }
    ],
    "position": "292.45638, 47.96946, 0.005",
    "format": "json"
  }
}
```

The `position` parameter is a comma-separated string of `ra, dec, radius` in degrees. The 0.005-degree radius (approximately 18 arcseconds) is tight enough to avoid matching nearby stars while accommodating TESS's relatively large pixel scale (21 arcseconds per pixel) @ricker2015.

The response uses the MAST columnar format: a `Tables` array containing a single table object with `Fields` (column definitions) and `Rows` (arrays of values). This is not row-oriented JSON --- each row is a positional array whose element ordering matches the `Fields` array. The `MastClient` must therefore first scan the `Fields` array to locate the column indices for `obsid`, `t_exptime`, `t_min`, and `t_max`, then iterate over the `Rows` array and extract values by index. This reconstructs row-oriented records from the columnar response.

The pipeline applies a multi-sector filter at this stage. If an observation's time span (`t_max - t_min`) exceeds 30 days, it is classified as a multi-sector combined observation and skipped. Multi-sector observations duplicate data from individual sectors and would lead to redundant light curve points. Only single-sector observations (approximately 27 days each) are retained.

The pipeline then performs idempotent re-ingestion. It calls `find_existing_obsids()` with the list of obsids from the MAST response. This method executes a single `SELECT obsid FROM tess_observations WHERE obsid = ANY($1)` query and returns a set of already-persisted obsids. The pipeline inserts only observations whose obsids are not in this set, then bulk-inserts the new records via `bulk_create()`.

=== Stage 3: Product Discovery

Stages 1 and 2 execute together when a target is first resolved. Stage 3 executes separately, triggered by a `POST /tess-observations/:id/discover-products` request for a specific observation.

The `TargetService::discover_products()` method first checks whether the observation already has a `data_uri`. If it does, the cached value is returned. Otherwise, `IMastClient::get_data_products()` calls the MAST `Mast.Caom.Products` service:

```json
{
  "service": "Mast.Caom.Products",
  "params": { "obsid": "428836211", "format": "json" }
}
```

The response uses the same columnar format. The pipeline locates the `dataURI`, `description`, `productType`, and `productFilename` columns, then filters for the light curve FITS file. The filter criteria are:

1. `description` equals `"Light curves"`
2. `productType` equals `"SCIENCE"`
3. `filename` ends with `"_lc.fits"`

The first matching product's `dataURI` is stored in the `data_uri` column of the `tess_observations` table via `update_data_uri()`.

=== Stage 4: FITS Download and Parse

Stage 4 is triggered by a `POST /tess-observations/:id/fetch-light-curve` request. The `TargetService::fetch_light_curve()` method first checks whether light curve points already exist for the observation (via `count_by_observation_id()`). If they do, the cached metadata is returned.

Otherwise, `IMastClient::download_fits()` sends an HTTP GET request to `https://mast.stsci.edu/api/v0.1/Download/file?uri=<data_uri>`. MAST responds with either the FITS binary data directly (HTTP 200) or an HTTP redirect (301, 302, or 307) to a CDN URL. The `MastClient` handles redirects manually: it parses the `Location` header, extracts the host and path, creates a new HTTP client for the redirect host, and sends a second GET request.

The downloaded binary data is passed to `IFitsParser::parse_light_curve()`, which extracts the light curve columns as described in @fits_design. The resulting `LightCurvePoint` records are bulk-inserted into the `light_curve_points` table in batches of 5,000 rows within a single database transaction.

=== Caching Strategy

The pipeline uses PostgreSQL as its sole cache layer --- there is no Redis, Memcached, or other intermediary. The caching is layered:

1. *Target-level cache*: `find_by_canonical_name()` returns the existing target record, skipping Stage 1 entirely.
2. *Observation-level cache*: `find_existing_obsids()` identifies which observations are already persisted, skipping them during Stage 2 bulk insert.
3. *Product-level cache*: the `data_uri` column on `tess_observations` records whether Stage 3 has run. A non-null `data_uri` causes `discover_products()` to return immediately.
4. *Light curve-level cache*: `count_by_observation_id()` checks whether Stage 4 has run. A positive count causes `fetch_light_curve()` to return the stored metadata.

This design means that the first resolution of a new target triggers MAST API calls, but all subsequent requests for the same target are served entirely from PostgreSQL. Re-ingestion (fetching new observations for an existing target) is supported by re-resolving the target: the pipeline detects the cached target, re-queries MAST for observations, and inserts only genuinely new ones.

== FITS File Format and Parsing Design <fits_design>

The Flexible Image Transport System (FITS) is the standard file format for astronomical data @pence2010. TESS light curve FITS files are produced by the Science Processing Operations Center (SPOC) pipeline @jenkins2016 @tenenbaum2018 and distributed via MAST.

=== FITS File Structure

A TESS light curve FITS file (`*_lc.fits`) contains multiple Header Data Units (HDUs):

- *Primary HDU (HDU 1)*: A header-only unit containing metadata about the observation (target name, TESS Input Catalog ID, sector number, camera, CCD, observation dates). No image or table data.
- *Binary Table Extension (HDU 2)*: The light curve data as a binary table with one row per cadence. This is the HDU that the parser reads.
- *Additional HDUs*: Aperture pixel mask and other auxiliary data. Not used by Nyx.

@fits_structure illustrates the layout.

#figure(
  rect(width: 100%, height: 160pt, stroke: 0.5pt)[
    #align(center + horizon)[_FITS file structure diagram --- HDU 1 (Primary): header keywords only. HDU 2 (Binary Table Extension): TIME, SAP\_FLUX, PDCSAP\_FLUX, PDCSAP\_FLUX\_ERR, QUALITY columns, one row per cadence (~18,000 rows at 2-min cadence). HDU 3+: aperture mask, auxiliary data (not used)._]
  ],
  caption: [Structure of a TESS light curve FITS file.],
) <fits_structure>

=== Light Curve Columns

The binary table in HDU 2 contains the following columns relevant to Nyx:

- *TIME* (DOUBLE PRECISION): Barycentric TESS Julian Date (BTJD), defined as BJD $minus$ 2457000.0 @eastman2010. This is the time axis of the light curve.
- *SAP_FLUX* (FLOAT): Simple Aperture Photometry flux in electrons per second. This is the raw sum of pixel values within the optimal aperture, after background subtraction. It contains instrumental systematics (spacecraft pointing jitter, thermal drift, scattered light) that produce trends and discontinuities unrelated to the target's intrinsic variability.
- *PDCSAP_FLUX* (FLOAT): Pre-search Data Conditioning SAP flux. This is the SAP flux after correction for systematic effects using cotrending basis vectors derived from quiet stars on the same CCD @stumpe2012. PDCSAP flux is the standard data product for transit detection and variable star analysis because the dominant instrumental signals have been removed.
- *PDCSAP_FLUX_ERR* (FLOAT): The 1-sigma uncertainty on the PDCSAP flux, propagated from the photon noise, readout noise, and background estimation uncertainty.
- *QUALITY* (INTEGER): A bitmask encoding data quality flags. Each bit indicates a specific anomaly (e.g. spacecraft attitude tweak, cosmic ray, momentum dump). A value of 0 means no known quality issues. The frontend allows toggling a quality filter that hides points with non-zero quality flags.

=== CFITSIO Parsing Implementation

The `FitsParser` class implements the `IFitsParser` interface. CFITSIO is a C library that requires file-based access --- it has no API for reading FITS data from an in-memory buffer. The parser therefore uses a temporary file approach:

1. Create a temporary file with `mkstemp("/tmp/nyx_fits_XXXXXX")`.
2. Write the FITS binary data (received as a `std::string` from the HTTP download) to the file descriptor.
3. Close the file descriptor.
4. Open the file with CFITSIO's `fits_open_file()`.
5. Move to HDU 2 with `fits_movabs_hdu()`.
6. Query the number of rows with `fits_get_num_rows()`.
7. Locate each column by name with `fits_get_colnum()` (case-insensitive).
8. Read each column into a pre-allocated `std::vector` with `fits_read_col()`.
9. Close the file with `fits_close_file()`.
10. Delete the temporary file.

Cleanup is guaranteed by the `FitsFileGuard` RAII class, which calls `std::remove()` on the temporary file in its destructor. This ensures the file is deleted even if an exception is thrown during parsing.

=== NaN Handling

FITS uses IEEE 754 NaN (Not a Number) as the standard representation for missing or undefined values. This is different from SQL NULL and must be handled explicitly. The parser checks each value with `std::isnan()`:

- If `TIME` is NaN, the entire row is skipped (a data point without a timestamp is useless).
- If `PDCSAP_FLUX`, `PDCSAP_FLUX_ERR`, or `SAP_FLUX` is NaN, the corresponding field is set to `std::nullopt` in the `LightCurvePoint` struct. The PostgreSQL repository serialises `std::nullopt` as an empty string, which is then cast to `NULL` via `CAST(NULLIF($n, '') AS REAL)` in the parameterised INSERT statement.

This approach preserves partial data: a point with a valid time and SAP flux but a NaN PDCSAP flux is still useful for analysis.

=== Column Reading Strategy

The parser reads all rows for each column in a single `fits_read_col()` call rather than reading one row at a time. This is significantly more efficient because CFITSIO can read contiguous blocks from the binary table in a single I/O operation, whereas row-by-row access would require repeated seeking. For a typical 2-minute cadence observation with approximately 18,000 rows, this means 5 bulk column reads rather than 90,000 individual cell reads.

== Image Processing and Photometry Design <photometry_design>

The image processing subsystem handles ground-based observation images uploaded by users. It consists of three stages: EXIF metadata extraction, optional DNG raw image decoding, and aperture photometry. These stages are modelled as three application-layer interfaces with infrastructure-layer implementations.

=== EXIF Metadata Extraction

The `IExifParser` interface declares a single method:

```
parse(file_path) -> Result<ExifData>
```

The `ExifData` struct contains eight optional fields extracted from the image's EXIF headers:

- `captured_at` --- the timestamp when the image was taken (from `Exif.Photo.DateTimeOriginal`).
- `camera_model` --- the camera body model (from `Exif.Image.Model`).
- `exposure_time_seconds` --- the exposure duration (from `Exif.Photo.ExposureTime`).
- `iso_speed` --- the ISO sensitivity (from `Exif.Photo.ISOSpeedRatings`).
- `gps_latitude` and `gps_longitude` --- the geographic coordinates (from `Exif.GPSInfo.GPSLatitude` and `GPSLongitude`).
- `image_width` and `image_height` --- the pixel dimensions (from `Exif.Photo.PixelXDimension` and `PixelYDimension`).

All fields are optional because different cameras populate different subsets of the EXIF specification. The `Exiv2ExifParser` implementation uses the Exiv2 library to read these fields, handling missing tags gracefully by returning `std::nullopt`.

The extracted metadata is persisted directly to the `observation_images` table. The `captured_at` timestamp is particularly important because it provides the time axis for ground-based light curve construction: each image's capture time, combined with its measured flux, forms one data point.

=== DNG Raw Image Decoding

The `IDngDecoder` interface declares:

```
decode(file_path) -> Result<DecodedImage>
```

The `DecodedImage` struct contains a `std::vector<uint16_t>` pixel array plus width, height, and channel count. Raw DNG images preserve the linear sensor response (unlike processed JPEG images that apply gamma correction, white balance, and compression), making them suitable for photometric analysis. The `LibrawDngDecoder` implementation uses the LibRaw library to demosaic the Bayer-pattern sensor data into a linear RGB image.

=== Aperture Photometry Algorithm

The `IPhotometryProcessor` interface declares two methods:

```
detect_sources(image, threshold_sigma) -> vector<DetectedSource>
measure_aperture(image, center_x, center_y,
  aperture_radius, annulus_inner_radius,
  annulus_outer_radius) -> Result<PhotometryResult>
```

The `AperturePhotometer` implementation performs aperture photometry following standard practices in amateur exoplanet observation @conti2018 @gary2007:

+ *Source detection*: The algorithm computes the mean and standard deviation of pixel values across the image. Pixels exceeding the mean by more than `threshold_sigma` standard deviations are candidate source pixels. Connected components of candidate pixels are grouped, and the centroid of each group yields a `DetectedSource` with `(x, y)` coordinates and a `peak_value`.

+ *Aperture definition*: A circular aperture of radius `aperture_radius` pixels is centred on the target star's coordinates `(center_x, center_y)`.

+ *Background estimation*: An annulus between `annulus_inner_radius` and `annulus_outer_radius` (both larger than the aperture radius) provides a sample of the local sky background. The median pixel value within the annulus is taken as the background estimate, which is more robust to contamination by faint stars than the mean.

+ *Flux calculation*: The raw flux is the sum of pixel values within the circular aperture minus the product of the background estimate and the aperture area (number of pixels within the aperture). In notation: $F_"raw" = sum_(i in "aperture") p_i - B_"sky" dot A$, where $p_i$ is the pixel value, $B_"sky"$ is the median background, and $A$ is the aperture area.

+ *Error estimation*: The flux error combines three noise sources: Poisson noise from the source ($sqrt(F_"raw" / g)$ where $g$ is the gain), readout noise ($sigma_"read"^2 dot A$), and background noise ($sigma_"sky"^2 dot A$). The total error is the square root of the sum of these variances.

The `PhotometryResult` contains `raw_flux` and `raw_flux_error`. The relative flux (normalised to a reference star or the mean) is computed by the `ObservationService` after all images in a session have been measured, and stored in the `relative_flux` and `relative_flux_error` columns of `observation_images`.

The `photometry_status` column tracks the processing state. When a user triggers photometry via `POST /observation-sessions/:id/photometry`, the service sets `photometry_status = 'pending'` for all images, then processes each image sequentially. On success, the status is updated to `'completed'` with the flux values. On failure (e.g. no source detected at the given coordinates, corrupted image file), the status is updated to `'failed'` with a diagnostic message in `photometry_error_message`.

== Time System Design <time_systems>

Comparing space-based and ground-based observations requires aligning their timestamps to a common reference frame. Three time systems are relevant to Nyx.

=== Julian Date (JD)

The Julian Date is the continuous count of days since the beginning of the Julian Period (12:00 UTC, January 1, 4713 BC). It is the universal time system in astronomy because it avoids the discontinuities and ambiguities of calendar-based systems (leap years, month lengths, time zones). For example, 2026-04-05T12:00:00 UTC corresponds to JD 2461402.0.

=== Barycentric Julian Date (BJD)

The Barycentric Julian Date corrects JD for the light travel time between the observer and the Solar System barycentre (centre of mass). Without this correction, the timestamps of the same event as measured from different locations in Earth's orbit can differ by up to $plus.minus$8.3 minutes --- the light travel time across 1 AU @eastman2010. For transit timing, where the ingress and egress of an exoplanet transit can last only minutes @winn2010, this correction is essential.

BJD is computed from JD by adding the light travel time correction: $"BJD" = "JD" + Delta t_"bary"$, where $Delta t_"bary"$ depends on the observer's position in the Solar System and the sky coordinates of the target. Precise computation requires ephemeris tables (e.g. JPL DE430).

=== Barycentric TESS Julian Date (BTJD)

TESS light curve timestamps use BTJD, defined as:

$ "BTJD" = "BJD" - 2457000.0 $

This offset subtracts a constant to produce smaller, more human-readable numbers. For example, a BJD of 2459580.5 becomes BTJD 2580.5. The offset 2457000.0 corresponds approximately to the J2015.0 epoch.

=== Ground-Based Timestamp Conversion

Ground-based observations record timestamps in UTC via EXIF metadata (ISO 8601 format). To overlay these on TESS light curves, Nyx must convert UTC to BTJD. The `TimeConversion` utility class implements this conversion:

1. Parse the ISO 8601 string into year, month, day, hour, minute, second.
2. Convert to JD using the standard formula for the Gregorian calendar:
   $ "JD" = floor(365.25 (Y + 4716)) + floor(30.6001 (M + 1)) + D + B - 1524.5 + (H + M\/60 + S\/3600)\/24 $
   where $Y$ and $M$ are adjusted for January/February ($Y = Y - 1$, $M = M + 12$ when month $lt.eq$ 2), and $B = 2 - floor(Y\/100) + floor(Y\/400)$ is the Gregorian correction.
3. Approximate BJD as JD (the barycentric correction is omitted because it requires ephemeris data and the target's sky coordinates; the resulting error of up to $plus.minus$8.3 minutes is acceptable for visual comparison but would need to be addressed for precise transit timing analysis @eastman2010 @hessman2004).
4. Convert to BTJD by subtracting the constant offset: $"BTJD" = "BJD" - 2457000.0$.

The `LightCurveComparisonService` applies this conversion when assembling the comparison response, placing both TESS and user data points on the same BTJD time axis.

== Authentication Architecture Design <auth_design>

The authentication system implements a dual-token strategy following OAuth 2.0 security best practices @lodderstedt2020. It supports two authentication flows: local email/password registration and Google OAuth2 login.

=== Token Strategy

The system uses two token types with different lifetimes and storage mechanisms:

- *Access token*: A JSON Web Token @jones2015 (JWT) signed with HS256 (HMAC-SHA256), containing the user ID and email as claims, with a 15-minute expiry. The short lifetime limits the window of exploitation if the token is stolen. The access token is returned in the JSON response body and stored by the frontend in memory (not in localStorage or cookies).

- *Refresh token*: An opaque UUID (not a JWT), with a 7-day expiry. The raw token is sent to the client via an HttpOnly, Secure, SameSite=Strict cookie scoped to the `/api/v1/auth` path. This cookie configuration ensures: (1) JavaScript cannot read the token (HttpOnly), (2) it is only sent over HTTPS (Secure), (3) it is not sent with cross-origin requests (SameSite=Strict), and (4) it is only sent to authentication endpoints (path scoping).

The token is never stored in raw form on the server. Before persistence, it is hashed with SHA-256. The database stores only the hash, so a database breach does not compromise active sessions.

=== Refresh Token Rotation

When the client's access token expires, it sends `POST /auth/refresh`. The refresh token is automatically included by the browser via the HttpOnly cookie. The `AuthService::refresh_access_token()` method:

1. Hashes the presented refresh token.
2. Looks up the token hash in the `refresh_tokens` table.
3. If the token is not found, returns 401 (invalid token).
4. If the token is found but `is_revoked = TRUE`, this indicates token reuse --- a potential theft. The service revokes the entire token family (all tokens with the same `family_id`) and returns 401.
5. If the token is found, not revoked, and not expired, the service:
   a. Marks the current token as revoked (`is_revoked = TRUE`).
   b. Generates a new access/refresh token pair.
   c. Stores the new refresh token hash with the same `family_id`.
   d. Returns the new access token in the response body and sets the new refresh token cookie.

@token_rotation_sequence illustrates this flow.

#figure(
  rect(width: 100%, height: 220pt, stroke: 0.5pt)[
    #align(center + horizon)[_Sequence diagram for refresh token rotation. Client #sym.arrow Server: POST \/auth\/refresh (cookie: refresh\_token=T1). Server: hash(T1) #sym.arrow lookup #sym.arrow found, not revoked #sym.arrow revoke T1 #sym.arrow generate T2 (same family\_id) #sym.arrow store hash(T2) #sym.arrow return access\_token + Set-Cookie: refresh\_token=T2. Reuse scenario: Client sends T1 again #sym.arrow Server: hash(T1) #sym.arrow found but is\_revoked=TRUE #sym.arrow revoke ALL tokens with same family\_id #sym.arrow return 401._]
  ],
  caption: [Refresh token rotation with family-based reuse detection.],
) <token_rotation_sequence>

The family-based reuse detection protects against a specific attack: if an attacker steals refresh token T1 and uses it to obtain T2 before the legitimate client does, the legitimate client's subsequent use of T1 triggers the reuse alarm and revokes the entire family, including the attacker's T2. This is the approach recommended by @lodderstedt2020.

=== CSRF Protection

Because the refresh token is stored in a cookie, state-changing requests are vulnerable to Cross-Site Request Forgery (CSRF) @owasp2024. Nyx implements the Double Submit Cookie pattern:

1. On login, the server generates a cryptographically random CSRF token and sets it as a non-HttpOnly, Secure, SameSite=Strict cookie (`csrf_token`). The non-HttpOnly flag is intentional --- the frontend JavaScript must be able to read this cookie.
2. For every state-changing request (POST, PUT, DELETE), the frontend reads the `csrf_token` cookie and includes its value in the `X-CSRF-Token` request header.
3. The `CsrfFilter` middleware compares the cookie value against the header value using `sodium_memcmp()` @bernstein2012 for constant-time comparison, preventing timing side-channel attacks.
4. GET, HEAD, and OPTIONS requests are exempt from CSRF validation because they are safe (non-state-changing) methods.

=== Google OAuth2 Flow

Google login uses the Authorization Code flow @hardt2012 @openid2014:

1. The frontend redirects the user to Google's authorization endpoint with the application's client ID, redirect URI, and requested scopes (`openid`, `email`, `profile`).
2. After the user consents, Google redirects back to the frontend with an authorization code.
3. The frontend sends `POST /auth/google` with the authorization code and redirect URI.
4. The `GoogleAuthClient` exchanges the code for an access token by calling Google's token endpoint.
5. The `GoogleAuthClient` uses the access token to fetch the user's profile from Google's userinfo endpoint.
6. The `AuthService` checks whether a user with the returned Google ID already exists. If so, it logs them in. If not, it creates a new user account with `auth_provider = 'google'` and `email_verified = TRUE` (Google has already verified the email).
7. The service generates an access/refresh token pair and returns it as in the local login flow.

=== Password Hashing

Passwords are hashed using Argon2id @biryukov2016, the winner of the 2015 Password Hashing Competition and the algorithm recommended by OWASP @owasp_password2024. The `ArgonPasswordHasher` uses libsodium's `crypto_pwhash_str()` function with `MODERATE` parameters (3 iterations, 256 MB memory). Argon2id combines the resistance to side-channel attacks of Argon2i with the resistance to GPU cracking of Argon2d.

@login_sequence illustrates the complete login flow.

#figure(
  rect(width: 100%, height: 200pt, stroke: 0.5pt)[
    #align(center + horizon)[_Sequence diagram for login. Client #sym.arrow Server: POST \/auth\/login {email, password}. Server: find user by email #sym.arrow verify password hash (Argon2id) #sym.arrow generate access\_token (JWT, 15min) + refresh\_token (UUID, 7d) #sym.arrow hash refresh\_token (SHA-256) #sym.arrow store in refresh\_tokens table #sym.arrow return {access\_token, csrf\_token} + Set-Cookie: refresh\_token=..., Set-Cookie: csrf\_token=..._]
  ],
  caption: [Login sequence showing token generation and cookie setting.],
) <login_sequence>

== Middleware Pipeline Design <middleware_design>

Every inbound HTTP request passes through a pipeline of Drogon filters before reaching the controller. Filters are applied in the order they are listed in the `ADD_METHOD_TO` macro for each route. The four middleware filters form a chain: CorrelationIdFilter, then RateLimitFilter (on auth routes), then JwtAuthFilter (on protected routes), then CsrfFilter (on state-changing protected routes). @middleware_chain illustrates this.

#figure(
  rect(width: 100%, height: 140pt, stroke: 0.5pt)[
    #align(center + horizon)[_Middleware filter chain diagram. HTTP Request #sym.arrow CorrelationIdFilter (generates UUID, creates scoped logger, attaches to request attributes) #sym.arrow RateLimitFilter (sliding window 10 req\/min\/IP, rejects with 429) #sym.arrow JwtAuthFilter (extracts Bearer token, verifies JWT, injects user\_id) #sym.arrow CsrfFilter (compares cookie vs header, constant-time) #sym.arrow Controller._]
  ],
  caption: [HTTP middleware filter chain.],
) <middleware_chain>

=== CorrelationIdFilter

The CorrelationIdFilter runs on every route. It generates a UUID v4 correlation ID, creates a request-scoped spdlog logger with the correlation ID embedded in its format pattern, and attaches both the correlation ID string and the logger instance to the request's attribute map. All subsequent code --- other filters, controllers, services, repositories --- retrieves the logger from the request attributes and uses it for logging. This ensures that every log line produced during a request carries the same correlation ID, enabling end-to-end tracing through aggregated logs.

=== RateLimitFilter

The RateLimitFilter implements a sliding window rate limiter. It maintains an in-memory `std::unordered_map<std::string, std::deque<time_point>>` keyed by client IP address, protected by a `std::mutex`. For each request:

1. Retrieve the client IP from the request's peer address.
2. Lock the mutex.
3. Remove all timestamps older than the window (1 minute) from the deque.
4. If the deque size equals or exceeds `max_requests` (10), reject the request with a 429 Too Many Requests response.
5. Otherwise, push the current timestamp onto the deque and allow the request to proceed.

The in-memory approach is appropriate because the backend runs as a single process with multiple threads (Drogon's thread pool). In a multi-instance deployment, a distributed rate limiter (e.g. Redis-backed) would be needed. The rate limiter is applied only to authentication endpoints (`/auth/*`), which are the primary targets for brute-force attacks.

=== JwtAuthFilter

The JwtAuthFilter runs on all routes that require authentication (equipment, observations, profile, light curve comparison). It:

1. Extracts the `Authorization` header.
2. Validates that the header begins with `"Bearer "`.
3. Passes the token string to `ITokenService::verify_access_token()`, which verifies the JWT signature (HS256), checks the expiry, and validates that the `type` claim is `"access"`.
4. If verification succeeds, injects the `user_id` claim into the request's attribute map.
5. If verification fails (missing header, invalid format, expired, tampered), returns a 401 response.

=== CsrfFilter

The CsrfFilter runs on state-changing routes (POST, PUT, DELETE) that are behind the JwtAuthFilter. It:

1. Skips validation for safe methods (GET, HEAD, OPTIONS).
2. Reads the `csrf_token` cookie and the `X-CSRF-Token` header.
3. If either is missing, returns 403 with `"CSRF token is missing"`.
4. Compares the cookie and header values using `sodium_memcmp()` for constant-time comparison. If they differ, returns 403 with `"CSRF token is invalid"`.
5. If they match, allows the request to proceed.

The constant-time comparison prevents an attacker from inferring the correct CSRF token one byte at a time via timing analysis @bernstein2012.

== Frontend Architecture Design <frontend_design>

The frontend is built with Next.js @nextjs2024 using the App Router and TypeScript. It serves as the primary user interface for target search, light curve visualisation, equipment management, observation session management, and image upload.

=== Directory Structure

```
frontend/
+-- app/
|   +-- (auth)/
|   |   +-- login/          Login page
|   |   +-- register/       Registration page
|   |   +-- verify-email/   Email verification page
|   +-- (dashboard)/
|   |   +-- targets/
|   |   |   +-- page.tsx         Target search page
|   |   |   +-- [id]/
|   |   |       +-- page.tsx     Target detail + light curve
|   |   +-- observations/
|   |   |   +-- page.tsx         Session list
|   |   |   +-- [id]/
|   |   |       +-- page.tsx     Session detail + image upload
|   |   +-- equipment/
|   |       +-- page.tsx         Equipment management
|   +-- layout.tsx           Root layout with auth provider
+-- components/
|   +-- charts/
|   |   +-- LightCurveChart.tsx   Plotly.js light curve viewer
|   +-- ui/                  Shared UI components
+-- lib/
|   +-- api.ts               API client with 401 retry
|   +-- auth.ts              Auth context and hooks
+-- types/                   TypeScript type definitions
```

=== API Client with Automatic Token Refresh

The API client module (`lib/api.ts`) wraps `fetch()` with authentication handling. On every request to a protected endpoint, it attaches the access token (stored in a React context / in-memory variable) as a `Bearer` header and the CSRF token (read from the `csrf_token` cookie) as an `X-CSRF-Token` header. If the server returns 401 (access token expired), the client automatically sends a refresh request (`POST /auth/refresh`), receives a new access token, and retries the original request. If the refresh also fails (refresh token expired or revoked), the client redirects to the login page. This retry logic is transparent to calling components.

=== Light Curve Viewer

The light curve viewer is the central visualisation component. It uses Plotly.js @plotly2024 with WebGL rendering to handle datasets of up to 50,000 data points with smooth zoom and pan interactions. The chart displays:

- *X-axis*: BTJD (Barycentric TESS Julian Date). Tick labels are formatted to show the fractional day.
- *Y-axis*: Normalised flux (PDCSAP flux divided by the median PDCSAP flux), centred near 1.0. Normalisation enables visual comparison between different observation sectors and between TESS and ground-based data.
- *Error bars*: Vertical error bars derived from PDCSAP_FLUX_ERR, also normalised.
- *Quality filter toggle*: A checkbox that hides data points with non-zero quality flags. Enabled by default.
- *Data series*: TESS PDCSAP flux as the primary scatter plot. When viewing a comparison, user observation data is overlaid as a second series with distinct styling.

Plotly.js was chosen over D3.js @bostock2011 because Plotly provides built-in WebGL scatter plot support (via `scattergl` trace type), automatic zoom/pan handling, and hover tooltips without requiring manual DOM manipulation. D3 would offer more customisation but at significantly higher implementation cost for the same visualisation quality.

=== Server-Sent Events for Ingestion Progress

The FITS download and parse stage (Stage 4 of the pipeline) can take several seconds per observation. To provide real-time feedback, the frontend establishes a Server-Sent Events (SSE) connection @w3c_sse2015 during the ingestion process. The server sends events with fields like `stage`, `progress`, and `message` that the frontend renders as a progress indicator. SSE was chosen over WebSocket because the communication is unidirectional (server to client only) and SSE has simpler semantics: automatic reconnection, event typing, and native browser support via `EventSource`.

== Design Decisions and Trade-offs <design_tradeoffs>

=== Why C++ over Python, Go, or Rust

Python is the dominant language in the astronomy data ecosystem (Lightkurve @lightkurve2018, Astropy, etc.), and would have provided the easiest integration with existing libraries. However, this project is a data science dissertation with emphasis on system design and data engineering, not on reusing high-level astronomy libraries. C++ was chosen for three reasons:

1. *Performance*: C++ enables zero-cost abstractions, predictable memory layout, and direct control over allocation patterns @stroustrup2013. For a backend that parses multi-megabyte FITS binary data and serves tens of thousands of light curve data points per request, this control matters. The FITS parser operates on raw byte arrays; the bulk insert constructs parameterised SQL strings in a tight loop; the JSON serialisation for large responses uses `json_agg` to avoid intermediate object construction.

2. *Systems-level learning*: Building a production-grade HTTP backend in C++ exercises skills in memory management (RAII, smart pointers), concurrency (mutex-protected shared state), binary format parsing (CFITSIO C API), and ABI boundary management (C library interop) that higher-level languages abstract away. These skills are the subject of examination in a data science programme that values understanding the full stack.

3. *Modern C++ expressiveness*: C++23 features --- `std::expected` for error handling, designated initialisers for struct construction, `std::optional` for nullable fields, structured bindings for tuple unpacking, `std::string::ends_with` for string matching --- provide expressiveness comparable to Go or Rust without sacrificing performance @cppreference2024.

Go was considered but rejected due to its limited type system (no generics at the time of project inception, no sum types). Rust was considered but rejected due to its steeper learning curve and smaller web framework ecosystem.

=== Why Drogon over Other C++ Frameworks

Drogon @drogon2024 was chosen over alternative C++ web frameworks (Crow, Pistache, cpp-httplib, Oat++) for four reasons. First, Drogon includes a built-in ORM with PostgreSQL support, eliminating the need for a separate database library. Second, it provides a filter (middleware) system that maps naturally to the correlation ID, rate limiting, JWT, and CSRF cross-cutting concerns. Third, it is actively maintained and widely used, with reliable documentation. Fourth, it includes a synchronous HTTP client (`HttpClient::sendRequest`) that simplifies the MAST API communication (no callback chains or futures).

=== Why PostgreSQL B-tree Indexes over TimescaleDB Hypertables

As discussed in @why_postgresql, the light curve data volume does not justify the additional operational complexity of TimescaleDB. The composite B-tree index on `(tess_observation_id, time)` provides the same query performance for the primary access pattern (retrieve all points for one observation, ordered by time). TimescaleDB's advantages --- chunk-level compression, automatic partitioning, chunk-level retention policies, and continuous aggregates --- are designed for write-heavy, high-cardinality time-series workloads @timescale2024. Nyx's workload is write-once-read-many: light curves are bulk-inserted once and then read repeatedly. The standard PostgreSQL B-tree index handles this pattern efficiently @winand2012.

=== Why Synchronous MAST Requests

The MAST API communication uses Drogon's synchronous HTTP client (`sendRequest` with a timeout). An asynchronous approach (using `sendRequest` with a callback, or using coroutines) would avoid blocking the Drogon event loop thread during the multi-second MAST API calls. The synchronous approach was chosen for simplicity: the ingestion pipeline is triggered by explicit user action (not background processing), the number of concurrent MAST requests is limited (one per user interaction), and Drogon runs multiple event loop threads so a single blocked thread does not prevent other requests from being served. If the platform needed to ingest data for hundreds of targets concurrently, the pipeline would need to be made asynchronous or offloaded to a background worker.

=== Why Form-Encoded POST to the MAST API

The MAST Portal API's `/api/v0/invoke` endpoint requires that the JSON request payload be URL-encoded and sent as a form parameter named `request` with content type `application/x-www-form-urlencoded` @stsdci2024. This is not a design choice made by Nyx --- it is a constraint imposed by the MAST API. The `MastClient` implementation accordingly constructs the form body as `"request=" + urlEncodeComponent(json_string)`.

=== Why `std::expected` over Exceptions

The codebase uses `std::expected<T, AppError>` (aliased as `Result<T>` @cppreference2024) for all recoverable errors in application code, rather than C++ exceptions. This choice has three motivations. First, error paths are visible in the function signature --- a caller cannot accidentally ignore an error because `std::expected` must be explicitly checked. Second, the performance characteristics are predictable: `std::expected` is a stack-allocated discriminated union with no heap allocation or stack unwinding. Third, the pattern aligns with the functional error handling style used in Rust and modern C++, where errors are values rather than control flow interrupts.

Exceptions are still used at infrastructure boundaries where third-party libraries throw them (Drogon ORM's `DrogonDbException`, nlohmann/json's `json::exception`, CFITSIO's error codes). These are caught at the boundary and converted to `AppError` values. No exception propagates out of the infrastructure layer into the application layer.
