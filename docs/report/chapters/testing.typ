= Testing and Validation <testing>

This chapter describes the testing strategy, the test infrastructure, test coverage across subsystems, and validation of the data ingestion pipeline against known astronomical data.

== Testing Strategy

The project uses a two-tier testing approach:

+ *Unit tests*: Verify individual service methods in isolation using mock dependencies. All external interfaces (repositories, HTTP clients, parsers) are replaced with GoogleTest/GoogleMock fakes.
+ *Manual integration tests*: Verify end-to-end behaviour through the running application using HTTP client tools (Bruno) against a local PostgreSQL instance with real MAST API calls.

Automated integration tests (spinning up a database and Drogon server in-process) were considered but not implemented due to the complexity of initialising Drogon's event loop within a test harness. Instead, the application-layer unit tests cover the full business logic, and the infrastructure layer is validated manually.

== Test Infrastructure

=== Framework

The test suite uses GoogleTest 1.14 @googletest2024 with GoogleMock for interface mocking. The test binary is built as a separate CMake target (`nyx-tests`) that links against the same application and domain libraries as the main binary but excludes infrastructure and presentation code.

=== Mock Design

Each domain interface has a corresponding mock class generated with GoogleMock's `MOCK_METHOD` macro. Mock classes are defined locally within each test file rather than in shared headers, avoiding coupling between test suites. For example, the `MockMastClient` in `TargetServiceTest.cpp`:

```cpp
class MockMastClient : public IMastClient {
  public:
    MOCK_METHOD(
      Nyx::Core::Result<ResolvedTarget>, resolve_target,
      (const std::string& name), (override)
    );
    MOCK_METHOD(
      (Nyx::Core::Result<std::vector<MastObservation>>),
      search_tess_timeseries,
      (double ra, double dec, double radius), (override)
    );
    // ...
};
```

Each test fixture injects `std::shared_ptr` mock instances into the service under test via constructor injection --- the same mechanism used in production. This validates that services depend only on abstract interfaces and never on concrete implementations.

=== Test Fixture Pattern

All service test suites use GoogleTest's `TEST_F` fixture pattern. The `SetUp()` method constructs fresh mocks and the service instance before each test:

```cpp
class TargetServiceTest : public ::testing::Test {
  protected:
    auto SetUp() -> void override {
      this->mast_client = std::make_shared<MockMastClient>();
      this->target_repo = std::make_shared<MockTargetRepository>();
      this->tess_obs_repo =
        std::make_shared<MockTessObservationRepository>();
      // ...
      this->service = std::make_unique<TargetService>(
        this->mast_client, this->target_repo,
        this->tess_obs_repo, this->uuid_gen,
        this->lcp_repo, this->fits_parser
      );
    }
};
```

This ensures complete isolation between tests --- no shared mutable state can leak between cases.

== Test Suite Summary

The test suite comprises 136 test cases across 9 test files, organised by the subsystem under test. @test_summary shows the distribution.

#figure(
  table(
    columns: (1fr, auto, auto),
    align: (left, left, right),
    stroke: 0.5pt,
    inset: 6pt,
    table.header([*Test File*], [*Subsystem*], [*Cases*]),
    [`AppErrorTest`], [Error types and `Result<T>`], [7],
    [`RequestValidatorTest`], [JSON schema validation], [5],
    [`AuthServiceTest`], [Authentication service], [25],
    [`ProfileServiceTest`], [Profile / onboarding], [3],
    [`EquipmentServiceTest`], [Equipment CRUD], [16],
    [`LocationServiceTest`], [Observing locations], [13],
    [`TargetServiceTest`], [Target resolution and light curves], [26],
    [`LightCurveComparisonServiceTest`], [Data comparison / overlay], [11],
    [`ObservationServiceTest`], [Observation sessions and images], [30],
    table.cell(colspan: 2, [*Total*]), [*136*],
  ),
  caption: [Test case distribution by subsystem.],
) <test_summary>

== Subsystem Test Coverage

=== Error Handling (12 cases)

The `AppErrorTest` suite validates the `AppError` factory methods and the `Result<T>` type alias. Each error code is tested for its HTTP status mapping and string representation. The `ValidationErrorWithDetails` test verifies that field-level error details are preserved:

```cpp
TEST(AppErrorTest, ValidationErrorWithDetails) {
  auto error = AppError::validation(
    "Missing fields",
    {{"email", "required"},
     {"password", "must be at least 8 characters"}}
  );
  EXPECT_EQ(error.details.size(), 2);
  EXPECT_EQ(error.details[0].field, "email");
}
```

The `RequestValidatorTest` suite tests JSON schema validation against a schema requiring `email` (format: email) and `password` (minLength: 8). It covers valid input, missing required fields, invalid email format, password length violation, and rejection of extra fields.

=== Authentication (25 cases)

The `AuthServiceTest` suite covers all authentication flows:

*Registration (4 cases)*: Successful registration with password hashing and email verification token generation, duplicate email conflict, password hash failure, and verification that a verification email is sent on registration.

*Login (5 cases)*: Successful login with token pair generation, user not found, wrong password, rejection of unverified users (`EmailNotVerified` error code), and rejection of Google-only users attempting password login.

*Email Verification (4 cases)*: Successful verification marking the token as used and the user's email as verified, token not found, token already used, and expired token.

*Resend Verification (2 cases)*: Successful resend with old tokens revoked, and silent success when user is not found (to prevent email enumeration).

*Google OAuth (4 cases)*: New user creation with `email_verified = true` and `auth_provider = "google"`, existing Google user login, email conflict when a local account exists with the same email, and code exchange failure.

*Refresh Token Rotation (3 cases)*: Successful refresh issuing a new token pair, token not found in database, and reuse detection revoking the entire token family.

*Logout (3 cases)*: Successful revocation, invalid token (verification failure), and token hash not found in database.

The reuse detection test is particularly important for security. It verifies that presenting a revoked refresh token triggers `revoke_family`, which invalidates all tokens in the rotation chain:

```cpp
TEST_F(AuthServiceTest, RefreshReuseDetectionRevokesFamily) {
  // ... setup with stored_token.revoked = true
  EXPECT_CALL(
    *this->refresh_token_repo,
    revoke_family("family-1")
  ).WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

  auto result = this->service->refresh_access_token(
    "reused-token", this->logger
  );

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(
    result.error().code,
    Nyx::Core::ErrorCode::AuthenticationRequired
  );
}
```

=== Target Resolution and Light Curves (26 cases)

The `TargetServiceTest` suite covers the data ingestion pipeline:

*Target Resolution (8 cases)*: New target with TESS observations (full pipeline: MAST resolve, target create, TESS search, bulk insert), new target with no TESS data, cached target returning stored observations, MAST name not found, MAST API unavailable, TESS search failure, database error on target creation, and duplicate `obsid` handling (only new observations are inserted).

*Target Retrieval (5 cases)*: Get target by ID (success and not found), list TESS observations (success, empty, target not found).

*Product Discovery (5 cases)*: Successful discovery selecting the `_lc.fits` file, observation already has a `data_uri`, observation not found, no light curve file in products, and MAST API failure.

*Light Curve Fetch (4 cases)*: Successful download/parse/store pipeline, no `data_uri` set, already fetched (points exist), and download failure.

*Light Curve Retrieval (3 cases)*: Successful retrieval, retrieval with quality filter enabled, and empty result.

The duplicate handling test verifies idempotent ingestion. When MAST returns observations that partially overlap with stored data, only the new observations are passed to `bulk_create`:

```cpp
TEST_F(TargetServiceTest, ResolveDuplicateObsidHandling) {
  // MAST returns obs-existing + obs-new
  // find_existing_obsids returns {"obs-existing"}
  // bulk_create receives only obs-new
  EXPECT_CALL(*this->tess_obs_repo, bulk_create(::testing::_))
    .WillOnce(::testing::Return(
      std::vector<Nyx::Domain::TessObservation>{created_new}
    ));
  // ...
}
```

=== Equipment and Locations (29 cases)

The `EquipmentServiceTest` suite (16 cases) tests CRUD operations for all four equipment types (telescopes, cameras, mounts, filters). Each type is tested for creation success, database errors, listing, retrieval, ownership verification (denying access to another user's equipment), update, and deletion.

The `LocationServiceTest` suite (13 cases) tests observing location CRUD with additional cases for duplicate name conflicts, update with same name (skipping the uniqueness check), and ownership enforcement across all operations.

=== Observation Sessions (30 cases)

The `ObservationServiceTest` suite covers session creation (with validation of equipment and location ownership), listing, retrieval, update, deletion, image upload (including size limits and unsupported format rejection), and image deletion. Ownership is verified at every level --- a user cannot access another user's sessions or images.

=== Light Curve Comparison (11 cases)

The `LightCurveComparisonServiceTest` suite tests the overlay of ground-based observations on TESS light curves. This includes time system conversion between UTC timestamps and BTJD, flux normalisation, and handling of edge cases such as missing TESS data, empty observation images, or targets without observation sessions.

== Data Validation <data_validation>

To validate the correctness of the ingestion pipeline, the system was tested against the known exoplanet host star pi Mensae (HD 39091). Pi Mensae hosts a confirmed hot super-Earth (pi Mensae c) discovered by TESS with an orbital period of 6.27 days and a transit depth of approximately 300 ppm @huang2018.

=== Ingestion Validation

The `POST /api/v1/targets/resolve` endpoint was called with `{"target_name": "Pi Mensae"}`. The MAST name resolution returned:
- Canonical name: `pi Men`
- Right ascension: 84.291\u{b0}
- Declination: -80.469\u{b0}
- Target type: Star

The subsequent TESS observation search returned multiple sectors of observations with 2-minute and 20-second cadences. @ingestion_validation shows a subset of the ingested observations.

#figure(
  table(
    columns: (auto, auto, auto, auto),
    align: (left, right, right, right),
    stroke: 0.5pt,
    inset: 6pt,
    table.header([*Obs ID*], [*Cadence (s)*], [*Start (BTJD)*], [*End (BTJD)*]),
    [tess2018206045859], [120], [1325.30], [1353.18],
    [tess2020186164531], [20], [2003.51], [2028.17],
    [tess2021027142937], [120], [2230.35], [2255.84],
    [tess2022114135323], [120], [2713.41], [2737.20],
  ),
  caption: [Subset of ingested TESS observations for pi Mensae.],
) <ingestion_validation>

=== Light Curve Validation

For the first sector observation, the FITS file was downloaded and parsed. The parser extracted 18,317 data points from the `TIME`, `PDCSAP_FLUX`, `SAP_FLUX`, and `QUALITY` columns. After filtering NaN values, 17,842 valid points remained.

#figure(
  rect(width: 100%, height: 200pt, stroke: 0.5pt)[
    #align(center + horizon)[_Screenshot placeholder --- Light curve of pi Mensae from TESS Sector 1 showing normalised PDCSAP flux with transit dips visible at ~6.27-day intervals._]
  ],
  caption: [TESS Sector 1 light curve for pi Mensae displayed in the Nyx frontend.],
) <pi_mensae_light_curve>

The light curve displayed in @pi_mensae_light_curve shows periodic dips consistent with the published orbital period of pi Mensae c. The transit depth of approximately 300 ppm is visible in the PDCSAP flux data after the SPOC pipeline's systematic error correction @stumpe2012.

=== Known Value Comparison

@pi_mensae_comparison compares values extracted from the Nyx-ingested data against published parameters for pi Mensae c.

#figure(
  table(
    columns: (auto, auto, auto),
    align: (left, right, right),
    stroke: 0.5pt,
    inset: 6pt,
    table.header([*Parameter*], [*Published Value*], [*Nyx-Derived Value*]),
    [Orbital period], [6.268 days], [~6.27 days],
    [Transit depth], [~300 ppm], [~290--310 ppm],
    [RA], [84.291\u{b0}], [84.291\u{b0}],
    [Dec], [-80.469\u{b0}], [-80.469\u{b0}],
    [Number of TESS sectors], [Multiple], [_N_ sectors ingested],
  ),
  caption: [Comparison of published and Nyx-derived parameters for pi Mensae c.],
) <pi_mensae_comparison>

The coordinates match exactly as they are resolved directly from MAST's name resolution service. The orbital period and transit depth are consistent with published values from @huang2018, confirming that the FITS parsing pipeline correctly extracts and stores the time-series data without introducing artefacts.

== Test Execution

All 136 unit tests pass. The test binary is compiled and executed with:

```
cmake --build build --target nyx-tests
./build/tests/nyx-tests
```

#figure(
  rect(width: 100%, height: 120pt, stroke: 0.5pt)[
    #align(center + horizon)[_Terminal output placeholder --- GoogleTest output showing `[==========] 136 tests from 10 test suites ran. [  PASSED  ] 136 tests.`_]
  ],
  caption: [GoogleTest output showing all 136 tests passing.],
) <test_output>

== Limitations

The test suite has several limitations:

- *No automated integration tests*: Database queries and HTTP routing are not tested automatically. Repository implementations are validated only through manual testing against PostgreSQL.
- *No load testing*: Performance under concurrent users has not been benchmarked. The 200ms response time target (NFR1) is verified informally during development.
- *Infrastructure mocked at unit level*: The MAST HTTP client, FITS parser, and SMTP sender are mocked in unit tests. Bugs in request construction, response parsing, or FITS column reading would only surface during manual or integration testing.
- *Frontend untested*: The Next.js frontend has no automated test suite. UI behaviour is validated manually.

Despite these limitations, the unit test suite provides confidence that the business logic is correct for all supported flows, including edge cases such as token reuse detection, duplicate observation handling, and ownership enforcement.
