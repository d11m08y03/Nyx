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

Objective 1 was achieved through the implementation of 25 API endpoints distributed across 5 domains within Drogon, as well as the implementation of background processing for the downloading of the FITS files, as well as the implementation of server-sent-event functionality to allow for the observation of the download progress in real time.

Objective 2 was achieved through the implementation of the four-stage data ingestion pipeline, which ingests data from MAST databases for TESS observations, downloads the relevant FITS files using the CFITSIO library, and parses those files into the database.

Objective 3 was achieved through the implementation of 18 database tables managed through goose migrations, 13 implementations of PostgreSQL repositories, and the implementation of parameterised queries throughout the codebase. The light_curve_points table stores all of the data from the parsed light curves, and features a composite index on the tess_observation_id and time fields to allow for fast access of these values.

Objective 4 was achieved through the implementation of local registration using Argon2id hashing for passwords, verification of users via SMTP, Google OAuth2, JSON Web Tokens with a 15-minute expiry time for access tokens, refresh tokens that are rotated on each use with detection of attempted reuse of those tokens, protection against cross-site request forgery through the use of a double-submit cookie, and the implementation of rate limiting on the authentication endpoints.

Objective 5 was achieved through the implementation of complete CRUD (create, read, update, delete) operations for equipment of four different types, locations, and observation sessions and images (of types JPEG, PNG, FITS, and DNG) that can be uploaded to those observation sessions.

Objective 6 was achieved through the development of the frontend application using Next.js and the App Router, which displays light curves (using Plotly.js and WebGL acceleration), enables the user to search for astronomical targets, and enables the user to manage their observation sessions.

Objective 7 was achieved through the ingestion of TESS data for the target star pi Mensae, and through the confirmation that the extracted light curve from those observations has dips in brightness that match the published orbital period (of 6.27 days) and depth (~300 ppm) of the transiting star pi Mensae c, as described in @data_validation.

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

All 25 functional requirements have corresponding implementations. Of the remaining 10 non-functional requirements, NFR2 (Argon2id), NFR3 (token expiry), NFR4 (CSRF), NFR5 (rate limiting), NFR6 (JSON envelope), NFR7 (error masking), NFR8 (correlation IDs), NFR1 (response time) and NFR9 (concurrent access) have been verified through the implementations and testing. NFR1 and NFR9 were informally tested during the development phase of the system.

== Technical Decisions

=== C++ as Backend Language

The decision to use C++23 for the backend was motivated by the focus on data science within this dissertation. C++23’s std::expected type allowed for better handling of errors without using exceptions, which was applied to all 13 implementations within the codebase and 5 service classes.

The trade-off was velocity of development. The Drogon framework is relatively small in comparison to other backend frameworks like Python, Go, or Rust. For example, features like JSON schema validation, SMTP email servers, and CSRF protections have to be manually implemented with Drogon instead of being built into the framework as with other languages.

=== Clean Architecture

The separation of these layers has also made the application easy to test. All 136 unit tests run without a database, HTTP server, or network connection. This is because all external dependencies are behind an interface. The downside of this architecture is that any new feature requires changing several files, including the interfaces, implementations, services, controllers, and request schemas.

=== PostgreSQL for Time-Series Data

The PostgreSQL database with a composite B-tree index on the (tess_observation_id, time) columns was sufficient for the scale of the project. Each TESS sector contains approximately 18,000 data points per observation, and the light curve queries retrieve all of the data points for a given observation. TimescaleDB @timescale2024 would be warranted for applications that require querying across multiple observations or that contain millions of data points per target.

== Lessons Learned

- Interface proliferation: The clean architecture pattern resulted in 15 interface headers. While this is fine for a project with a single developer, it does introduce considerable boilerplate code.
- FITS parsing complexity: The C API provided by CFITSIO requires care when allocating and deallocating resources. A RAII class and reading the file column by column were necessary to prevent resource leaks. Additionally, writing to a temporary file was required to parse the FITS file since CFITSIO does not support writing to in-memory buffers.
- MAST API response format: The response from the MAST API is in the form of a JSON object whose fields and data are represented as arrays. A class to reconstruct rows from these responses was necessary. This is a decision made by NASA in the design of the API to reduce the amount of data that must be transmitted across the network since the names of the columns appear only once in the response.
- Token rotation security: A refresh token refresh mechanism that includes the detection of refresh token reuse by a family of devices requires some careful state management. The inclusion of a family_id field in the refresh_tokens table allows for a single database UPDATE statement to invalidate all refresh tokens issued to a client upon the detection of reuse. This is simpler than implementing data structures like linked lists to store the tokens issued to a client.

== Future Work

- Automated integration tests: Write a test harness that launches a PostgreSQL database and Drogon server instance to test the repository code. This would address the main missing element of testing the current code.
- Photometric reduction: Allow users to perform aperture photometry on the observation images that they upload to the server. This will allow for the extraction of photometric measurements from the raw observation data.
- Observation planner: Allow users to view the visibility of their chosen astronomical objects from their geographical location. This will use the coordinates of the objects in the database and the user’s geographical location to calculate visibility.
- Additional data sources: Expand the database queries to allow for additional data endpoints from databases like the NASA Exoplanet Archive and the AAVSO International Database.
- Horizontal scaling: The current Drogon server could be horizontally scaled out through the introduction of technologies like PgBouncer, read replicas, and distributed task queues.
- Period folding: Allow for period folding on the light curves to combine the observations from the two TESS observation sectors into a single light curve. This will allow for easier visualization of the objects’ periodic behaviors.

== Summary

The Nyx platform achieves its aim of integrating data from the NASA TESS mission with ground-based astronomical observations. The system can resolve astronomical objects by their name, ingest TESS data through a four-stage ETL process, store the light curves in a PostgreSQL database, and display the observations in visualisations. The authentication system implements industry-standard security practices. The software architecture allows for unit testing of all components of the software, with 136 separate test cases. The system has been tested using the exoplanet host star pi Mensae, confirming that the data ingested from TESS matches the parameters of that observed star.
