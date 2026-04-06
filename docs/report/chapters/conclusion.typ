= Conclusion <conclusion>

This chapter evaluates the project against the objectives defined in @introduction, discusses technical decisions and their outcomes, and identifies directions for future work.

== Evaluation Against Objectives

@objectives_evaluation summarises the achievement status of each project objective.

#figure(
  table(
    columns: (auto, 1fr, auto),
    align: (left, left, left),
    stroke: 0.5pt,
    inset: 6pt,
    table.header([*Obj.*], [*Description*], [*Status*]),
    [1], [REST API backend with background processing and SSE], [Achieved],
    [2], [Data ingestion pipeline (MAST API, FITS parsing)], [Achieved],
    [3], [PostgreSQL persistence layer], [Achieved],
    [4], [Authentication system (JWT, OAuth2, email verification)], [Achieved],
    [5], [Observation management module], [Achieved],
    [6], [Frontend application with interactive visualisation], [Achieved],
    [7], [Validation against known astronomical data], [Achieved],
  ),
  caption: [Objective achievement summary.],
) <objectives_evaluation>

*Objective 1* was met by implementing 25 RESTful API endpoints across five domains (authentication, targets, equipment, observations, profiles) using the Drogon framework. Background processing for long-running FITS downloads and SSE for real-time progress reporting were implemented for the light curve ingestion pipeline.

*Objective 2* was met by building a four-stage ingestion pipeline: MAST name resolution, TESS observation search, FITS product discovery, and FITS download with parsing via CFITSIO. The pipeline handles 2-minute and 20-second cadence TESS data products.

*Objective 3* was met with 18 database tables managed by goose migrations, 13 PostgreSQL repository implementations, and parameterised queries throughout. The `light_curve_points` table stores parsed time-series data with a composite index on `(tess_observation_id, time)` for efficient range scans.

*Objective 4* was met with local registration (Argon2id password hashing), email verification via SMTP, Google OAuth2, JWT access tokens (15-minute expiry), refresh token rotation with reuse detection, CSRF double-submit cookie validation, and rate limiting on authentication endpoints.

*Objective 5* was met with CRUD operations for four equipment types (telescopes, cameras, mounts, filters), observing locations, and observation sessions with image upload support (JPEG, PNG, FITS, DNG) including EXIF metadata extraction.

*Objective 6* was met with a Next.js frontend using the App Router, featuring interactive light curve charts rendered with Plotly.js and WebGL acceleration, a target search interface, and observation session management.

*Objective 7* was met by ingesting TESS data for pi Mensae and confirming that the extracted light curve shows transit dips consistent with the published orbital period (6.27 days) and transit depth (~300 ppm) of pi Mensae c, as described in @data_validation.

== Requirements Coverage

@requirements_coverage maps functional requirements to their implementation and test evidence.

#figure(
  table(
    columns: (auto, auto, auto),
    align: (left, left, left),
    stroke: 0.5pt,
    inset: 6pt,
    table.header([*Requirement*], [*Implementation*], [*Test Evidence*]),
    [FR1.1--1.3], [`AuthService`], [25 unit tests],
    [FR1.4--1.6], [Email verification flow], [6 unit tests],
    [FR1.7], [Refresh token rotation], [3 unit tests],
    [FR1.8], [`AuthService::logout`], [3 unit tests],
    [FR2.1--2.3], [`TargetService::resolve_target`], [8 unit tests],
    [FR2.4], [`TargetService::list_tess_observations`], [3 unit tests],
    [FR2.5], [`TargetService::discover_products`], [5 unit tests],
    [FR2.6--2.7], [`TargetService::fetch_light_curve`], [4 unit tests],
    [FR3.1--3.4], [`EquipmentService`], [16 unit tests],
    [FR4.1], [`LocationService`], [13 unit tests],
    [FR4.2--4.4], [`ObservationService`], [30 unit tests],
    [FR5.1--5.3], [Next.js frontend], [Manual testing],
    [FR6.1--6.2], [Background processing + SSE], [Manual testing],
    [FR6.3], [Idempotent ingestion], [1 unit test],
  ),
  caption: [Requirements coverage by tests.],
) <requirements_coverage>

All 25 functional requirements have corresponding implementations. Of the 10 non-functional requirements, NFR2 (Argon2id), NFR3 (token expiry), NFR4 (CSRF), NFR5 (rate limiting), NFR6 (JSON envelope), NFR7 (error masking), and NFR8 (correlation IDs) are verified through the implementation and manual testing. NFR1 (response time) and NFR9 (concurrent access) were validated informally during development.

== Technical Decisions

=== C++ as Backend Language

Choosing C++23 for the backend was motivated by the dissertation's data science focus: the language enables direct control over memory layout and CPU cache behaviour, which is relevant for processing large FITS files containing tens of thousands of data points. The `std::expected` type (C++23) enabled explicit error propagation without exceptions, producing a consistent error-handling pattern across all 13 repository implementations and 5 service classes.

The trade-off was development velocity. The Drogon ecosystem is smaller than those of frameworks in Python, Go, or Rust. Some features that would be built-in or trivially available in other frameworks --- such as JSON schema validation, SMTP email sending, and CSRF middleware --- required manual implementation.

=== Clean Architecture

The strict separation between domain, application, infrastructure, and presentation layers proved valuable for testability. All 136 unit tests run without a database, HTTP server, or network connection because every external dependency is behind an interface. The downside is verbosity: each new feature requires touching interface definitions, concrete implementations, service methods, controller routes, and request schemas across multiple files.

=== PostgreSQL for Time-Series Data

PostgreSQL with a composite B-tree index on `(tess_observation_id, time)` was sufficient for the project's scale. A single TESS sector produces approximately 18,000 data points per observation, and light curve queries retrieve all points for a single observation. For this access pattern, the composite index ensures index-only scans. TimescaleDB @timescale2024 would be warranted if the system needed to query across multiple observations simultaneously or if the dataset grew to millions of points per target.

== Lessons Learned

+ *Interface proliferation*: The clean architecture pattern resulted in 15 interface headers across the domain and application layers. For a single-developer project, this level of abstraction added significant boilerplate. However, it also caught design errors early --- any dependency that would violate the layer hierarchy is immediately visible as an `#include` that crosses the wrong boundary.

+ *FITS parsing complexity*: CFITSIO's C API requires careful resource management. The RAII `FitsFileGuard` wrapper and column-by-column reading approach were essential to avoid resource leaks. The need to write binary data to a temporary file before parsing (CFITSIO does not support in-memory buffers) added latency to the pipeline.

+ *MAST API response format*: The columnar JSON response format from MAST (`fields` array + `data` array) required a reconstruction step to produce row-oriented records. This is an API design decision on NASA's side that optimises for bandwidth (column names appear once) at the cost of client-side complexity.

+ *Token rotation security*: Implementing refresh token rotation with family-based reuse detection required careful state management. The `family_id` column on `refresh_tokens` enables a single `UPDATE` statement to revoke all tokens in a chain when reuse is detected, which is simpler than maintaining explicit linked lists of token generations.

== Future Work

+ *Automated integration tests*: Add a test harness that starts a PostgreSQL container and Drogon instance, enabling automated testing of repository SQL, HTTP routing, and middleware behaviour. This would address the main gap in the current test suite.

+ *Photometric reduction*: Implement aperture photometry on uploaded observation images using source extraction algorithms. This would allow users to extract flux measurements directly from their FITS or DNG images rather than requiring pre-reduced data.

+ *Observation planner*: Add a feature that calculates target visibility from the user's location based on altitude, azimuth, and local sidereal time. This would use the stored RA/Dec coordinates and the user's observing location to generate nightly observation schedules.

+ *Additional data sources*: Integrate the NASA Exoplanet Archive API for confirmed exoplanet parameters (mass, radius, equilibrium temperature) and the AAVSO International Database for community variable star observations.

+ *Horizontal scaling*: The current single-instance architecture could be extended with connection pooling (PgBouncer), read replicas for light curve queries, and a distributed task queue (Redis-backed) for parallel FITS processing.

+ *Period folding*: Implement phase-folding of light curves on the known orbital period, which would produce a single combined transit shape from multiple TESS sectors. This is a standard analysis technique that would enhance the visualisation capabilities.

== Summary

The Nyx platform achieves its aim of integrating NASA TESS mission data with ground-based astronomical observations. The system resolves targets by name, ingests TESS time-series data through a four-stage ETL pipeline, stores structured light curve data in PostgreSQL, and presents interactive visualisations that overlay space-based and ground-based observations. The authentication system implements industry-standard security practices, and the clean architecture enables comprehensive unit testing with 136 test cases covering all business logic. Validation against the known exoplanet host star pi Mensae confirms that the ingestion pipeline produces data consistent with published astronomical parameters.
