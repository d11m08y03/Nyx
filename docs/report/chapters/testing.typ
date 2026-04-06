= Testing and Validation <testing>

This test exercises the entire pipeline, from name resolution to TESS observation search and insert. All of the external components of this search are mocked in this test, so it completes in milliseconds without accessing the network or the database.

== Testing Strategy

The test suite is based on the following two-tier approach:

- Unit tests verify each of the individual methods within the service class. All external dependencies are mocked.
- Manual integration tests verify the end-to-end behaviour of the application by manually running the application and using HTTP client tools (Bruno) to make requests against a local PostgreSQL instance making actual calls to the MAST API.

While integration tests that started the database and Drogon server within the test were considered for inclusion, they were not implemented due to the complexity of initialising the Drogon event loop within a test. As such, all unit tests for the application layer have been implemented and manual tests have been performed for the infrastructure layer.

== Test Infrastructure

=== Framework

The test suite uses GoogleTest 1.14 @googletest2024 with GoogleMock. The test binary is a separate CMake target (nyx-tests) that links against the same libraries as the main binary, but excluding infrastructure and presentation.

=== Mock Design

Each interface has a corresponding mock class generated with the MOCK_METHOD macro. Mock classes are defined locally in test files so that test suites do not have to be coupled to each other. For example, the MockMastClient class defined in TargetServiceTest.cpp:

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

Each test fixture injects a std::shared_ptr instance of each mock class into the service under test through its constructor - the same injection mechanism that is employed in production.

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

This ensures complete isolation between tests, no shared mutable state can leak between cases.

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

The AppErrorTest suite validates the AppError factory methods and the Result<T> type alias. Each error code is tested to ensure that it has the correct HTTP status and string representation. The ValidationErrorWithDetails test ensures that the errors created with field details correctly display those details:

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

The RequestValidatorTest suite tests that a request with a schema that requires an email field with a format of “email” and a password field with minLength of 8 validates correctly against valid inputs, missing fields, invalid emails, wrong password length, and extra fields.

=== Authentication (25 cases)

The AuthServiceTest suite covers all authentication flows:

- Registration: successful registration with password hashing and verification token generation, duplicate email, password hash failure, and verification of sent verification email upon registration.
- Login: successful login, user not found, wrong password, attempting to login with unverified email (EmailNotVerified error code), and attempting to login in with password by Google-only users.
- Email Verification: successful verification of email and verification token, verification token not found, verification token already used, and expired verification token.
- Resend Verification: successful resend of verification email with old verification tokens revoked, and silent success in case user is not found (preventing enumeration of users by email).
- Google OAuth: successful creation of new users with verified email via Google OAuth with auth_provider set to "google", login of existing Google OAuth users, preventing creation of users with same email as local user account, and failure of Google OAuth code exchange.
- Refresh Token Rotation: successful rotation of refresh token to issue new token pair, refreshing of token pair when token not found in database, and detection of reuse of refresh token which invalidates entire refresh token family.
- Logout: successful logout, invalid token upon failed verification of token hash, and refresh token hash not found in database.

The test for detecting reuse of refresh tokens is important for security. When a refresh token is revoked, all tokens associated with that refresh token are invalidated via the revoke_family function:

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

The following tests ensure coverage of the data ingestion pipeline:

- Target Resolution: Tests for a new target that has TESS observations to ingest (full ingestion pipeline), a new target that does not have TESS observations, a cached target that already has its observations stored, a target whose MAST name is not found, an unavailable MAST API, failed TESS search, database error on target creation, and a case where there are duplicate obsid values for observations (only new observations are ingested).
- Target Retrieval: Tests getting a target by ID (success and not found) and listing the TESS observations for a target (success and target not found).
- Product Discovery: Tests discovering a product successfully (choosing the \_lc.fits file), a case where the observation already has a data_uri, a case where the observation is not found, a case where there is no light curve file within the products for that observation, and a failed request to the MAST API.
- Light Curve Fetch: Tests the successful fetch and storage of a light curve, a case where the observation does not have a data_uri set, a case where the light curve has already been fetched and imported (points exist), and a failed attempt to download the light curve.
- Light Curve Retrieval: Tests retrieving a light curve successfully, retrieving a light curve with the quality filter enabled, and a case where the result from the MAST API is empty.

The test for duplicate observations ensures that only new observations are passed to the bulk_create method on the Observation model:

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

EquipmentServiceTest (16 tests) tests all CRUD operations for each of the four different types of equipment.

LocationServiceTest (13 tests) tests all of the CRUD operations for observing locations, as well as additional tests for cases like duplicate location names, updating a location with the same name, and ownership.

=== Observation Sessions (30 cases)

The ObservationServiceTest suite tests the creation, listing, retrieval, updating, deletion of sessions, uploading and deleting images from those sessions. Furthermore, each of these actions validates that the user owns the equipment or location being used. Thus, no users will be able to access another user’s observations or observation images.

=== Light Curve Comparison (11 cases)

The LightCurveComparisonServiceTest suite allows for testing the placement of ground-based observations onto TESS light curves. The implementation accounts for the conversion between UTC timestamps and BTJD, normalising the flux from the ground-based observations, and handling edge cases like no TESS data for a given target, no images taken of the target, or targets that did not have any observation sessions allocated to them.

== Data Validation <data_validation>

To validate the correctness of our ingestion pipeline, we employed the known exoplanet hosting star pi Mensae (HD 39091). pi Mensae hosts the confirmed hot super-Earth planet pi Mensae c, which was discovered by the TESS mission with an orbital period of 6.27 days and a depth of approximately 300 ppm @huang2018.

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

For the first of the two sector observations, the file was downloaded and parsed. The parser revealed 18,317 data points within the TIME, PDCSAP_FLUX, SAP_FLUX, and QUALITY columns. After filtering the data points that contained NaN values, there remained 17,842 data points.

#figure(
  rect(width: 100%, height: 200pt, stroke: 0.5pt)[
    #align(
      center + horizon,
    )[_Screenshot placeholder --- Light curve of pi Mensae from TESS Sector 1 showing normalised PDCSAP flux with transit dips visible at ~6.27-day intervals._]
  ],
  caption: [TESS Sector 1 light curve for pi Mensae displayed in the Nyx frontend.],
) <pi_mensae_light_curve>

The light curve displayed in @pi_mensae_light_curve indicates the presence of dips in the light that are in accordance with the published orbital period of the planet pi Mensae c. The depth of the dips, at approximately 300 ppm, is visible in the PDCSAP flux values following the correction for systematics introduced by the SPOC pipeline @stumpe2012.

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

All of these coordinates match exactly what is reported from MAST. The period and depth parameters also match those published from @huang2018, indicating that the FITS files are correctly parsed.

== Test Execution

All 136 unit tests pass. The test binary is compiled and executed with:

```
cmake --build build --target nyx-tests
./build/tests/nyx-tests
```

#figure(
  rect(width: 100%, height: 120pt, stroke: 0.5pt)[
    #align(
      center + horizon,
    )[_Terminal output placeholder --- GoogleTest output showing `[==========] 136 tests from 10 test suites ran. [  PASSED  ] 136 tests.`_]
  ],
  caption: [GoogleTest output showing all 136 tests passing.],
) <test_output>

== Limitations

The test suite has several limitations:

- There are no automated integration tests for the API; database queries and routing are not tested automatically. Repository implementations are only tested manually on PostgreSQL.
- There is no load testing of the API to ensure it can handle the required number of concurrent users (NFR1). The response time of 200ms has been informally tested during development.
- The infrastructure components (HTTP client, FITS parser, SMTP sender) are mocked in unit tests. Bugs in these components would not be detected by unit tests but only during manual testing.

The frontend has not been tested with an automated test suite; the buttons and forms have been manually tested.

Despite these limitations, the unit tests ensure that the business logic of the API is implemented correctly for all supported flows and edge cases.
