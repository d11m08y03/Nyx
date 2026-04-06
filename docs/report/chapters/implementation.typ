= Implementation <implementation>

This chapter presents the implementation of every major subsystem in the Nyx platform. Each section walks through the real code, explains the design rationale behind non-obvious decisions, and links back to the architectural choices described in @design. The chapter is organised bottom-up: foundational infrastructure first (build system, error handling), then the data ingestion pipeline (MAST client, FITS parser, bulk insert), then authentication and security, then the middleware pipeline, then the database access layer and response formatting, and finally the frontend application.

== Build System and Dependencies <build_system>

The backend uses CMake 3.20+ as the build system and vcpkg as the C++ package manager. The `vcpkg.json` manifest declares 12 dependencies:

```cpp
{
  "name": "nyx-backend",
  "version": "0.1.0",
  "dependencies": [
    { "name": "drogon", "features": ["postgres"] },
    "spdlog", "nlohmann-json", "simdjson",
    "jwt-cpp", "libsodium", "json-schema-validator",
    "gtest", "cfitsio", "exiv2", "libraw", "curl"
  ]
}
```

Drogon @drogon2024 provides the HTTP server, routing, and ORM layer; the `postgres` feature enables the PostgreSQL client driver. `nlohmann-json` handles JSON serialisation for request/response bodies and API payloads, while `simdjson` is available for high-throughput parsing of large NASA API responses. `spdlog` provides structured logging with nanosecond timestamps. `jwt-cpp` handles JWT creation and verification. `libsodium` supplies Argon2id password hashing, SHA-256 token hashing, CSRF constant-time comparison, and cryptographic random number generation. `cfitsio` is the reference C library for reading FITS files @pence2010. `exiv2` extracts EXIF metadata from observation images. `libraw` decodes DNG raw images from the telescope camera. `curl` is used by the SMTP email sender for sending verification emails over TLS.

The `CMakeLists.txt` targets C++23 and enforces strict compiler diagnostics:

```cmake
cmake_minimum_required(VERSION 3.20)
project(nyx-backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
)
```

The `-Werror` flag promotes all warnings to errors, ensuring that the codebase compiles warning-free. `-Wno-unused-parameter` is the only suppression, needed because Drogon's filter callback signatures often include parameters that are unused in specific filter implementations.

Database migrations are managed by goose. The project includes 18 migration files covering user authentication (`create_users`, `create_refresh_tokens`, `add_email_verified_to_users`, `create_verification_tokens`, `add_oauth_fields_to_users`), astronomical data (`create_targets`, `create_tess_observations`, `create_light_curve_points`), equipment (`create_telescopes`, `create_cameras`, `create_mounts`, `create_filters`), observation sessions (`create_observation_sessions`, `add_equipment_fks_to_sessions`, `add_photometry_to_images`), and location data (`create_observing_locations`, `add_unique_location_name_per_user`). Migrations are created via `goose -dir backend/migrations create <name> sql` and applied with `goose -dir backend/migrations up`.

The application entry point in `main.cpp` initialises the logger, configures the database connection pool, and starts the Drogon HTTP server:

```cpp
auto main() -> int {
  try {
    auto config = Nyx::Infrastructure::Config::EnvironmentConfig();
    Nyx::Core::Logger::initialize(config.log_level());

    spdlog::info("Starting Nyx backend v0.1.0");
    spdlog::info("Configuring server on port {} with {} threads",
      config.server_port(), config.server_threads());

    Nyx::Infrastructure::Database::DatabaseManager::initialize(config);

    drogon::app()
      .setLogLevel(trantor::Logger::kFatal)
      .addListener("0.0.0.0", config.server_port())
      .setThreadNum(config.server_threads())
      .registerBeginningAdvice([] {
        spdlog::info("Nyx backend is running");
      })
      .run();
  } catch (const std::exception& exception) {
    spdlog::critical("Fatal error during startup: {}", exception.what());
    return 1;
  }
  return 0;
}
```

Drogon's internal logging is silenced (`kFatal`) because all application logging goes through spdlog. Controller registration is handled declaratively via macros in each controller's header file; Drogon discovers them at compile time through its code generation system.

== Error Handling with `std::expected` <error_handling>

Every fallible operation in Nyx returns `Nyx::Core::Result<T>`, a type alias for `std::expected<T, AppError>` @cppreference2024:

```cpp
template<typename T>
using Result = std::expected<T, AppError>;
```

The `AppError` struct carries a machine-readable `ErrorCode` enum, a human-readable message string, and an optional vector of field-level validation errors:

```cpp
enum class ErrorCode {
  ValidationError,        // 400
  InvalidJson,            // 400
  AuthenticationRequired, // 401
  InvalidToken,           // 401
  TokenExpired,           // 401
  PermissionDenied,       // 403
  EmailNotVerified,       // 403
  ResourceNotFound,       // 404
  Conflict,               // 409
  RateLimitExceeded,      // 429
  InternalError,          // 500
  DatabaseError,          // 500
  ExternalServiceError,   // 500
};
```

Each error code maps to an HTTP status code via `http_status_code()` and to a string representation via `error_code_string()`. The mapping is implemented as a switch over the enum:

```cpp
[[nodiscard]] auto http_status_code() const -> int {
  switch (code) {
    case ErrorCode::ValidationError:
    case ErrorCode::InvalidJson:
      return 400;
    case ErrorCode::AuthenticationRequired:
    case ErrorCode::InvalidToken:
    case ErrorCode::TokenExpired:
      return 401;
    case ErrorCode::PermissionDenied:
    case ErrorCode::EmailNotVerified:
      return 403;
    case ErrorCode::ResourceNotFound:
      return 404;
    case ErrorCode::Conflict:
      return 409;
    case ErrorCode::RateLimitExceeded:
      return 429;
    case ErrorCode::InternalError:
    case ErrorCode::DatabaseError:
    case ErrorCode::ExternalServiceError:
      return 500;
  }
  return 500;
}
```

`AppError` provides static factory methods for common error types, eliminating the need for callers to construct the full struct:

```cpp
static auto validation(
  std::string message,
  std::vector<FieldError> details = {}
) -> AppError {
  return AppError{ErrorCode::ValidationError,
    std::move(message), std::move(details)};
}

static auto not_found(std::string message) -> AppError {
  return AppError{ErrorCode::ResourceNotFound,
    std::move(message), {}};
}

static auto internal(std::string message) -> AppError {
  return AppError{ErrorCode::InternalError,
    std::move(message), {}};
}
```

This approach replaces exceptions with explicit error propagation. Callers check `result.has_value()` and access `result.error()` on failure. The pattern is consistent across all three layers of the architecture described in @architecture: repositories return `Result<T>`, services return `Result<T>`, and controllers map errors to HTTP responses via `ResponseHelper::error()`. Because `std::expected` is a value type, there is no heap allocation for the error path, and the compiler can optimise the success path as a direct return.

The consistency of this pattern is demonstrated by a typical service method. Every operation that can fail returns a `Result`, and the caller checks each one before proceeding:

```cpp
auto obs_result =
  this->tess_observation_repository->find_by_id(observation_id);
if (!obs_result.has_value()) {
  return std::unexpected(obs_result.error());
}
if (!obs_result->has_value()) {
  return std::unexpected(
    Nyx::Core::AppError::not_found("TESS observation not found")
  );
}
auto observation = obs_result->value();
```

This two-level unwrapping (first checking for database errors, then for "not found") is a deliberate design choice. The repository returns `Result<std::optional<T>>` rather than using the error channel for "not found", because absence is a normal condition (not an error), while a database timeout or connection failure is genuinely exceptional.

== MAST API Client Implementation <mast_client_impl>

The `MastClient` implements the `IMastClient` interface defined in the application layer, as described in @architecture. All MAST API calls go through a single base method, `invoke_mast_service`, which handles HTTP transport, error detection, and response extraction.

=== Base Method: `invoke_mast_service` <invoke_mast>

Every MAST API call @stsdci2024 follows the same protocol: a form-encoded POST to `https://mast.stsci.edu/api/v0/invoke` with a `request` parameter containing a JSON payload. The `invoke_mast_service` method encapsulates this:

```cpp
auto MastClient::invoke_mast_service(
  const std::string& request_json
) -> Nyx::Core::Result<std::string> {
  try {
    auto client = drogon::HttpClient::newHttpClient(
      "https://mast.stsci.edu"
    );
    client->setUserAgent("Nyx/1.0");

    auto path = this->base_url + "/invoke";
    auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Post);
    request->setPath(path);
    request->setContentTypeCode(drogon::CT_APPLICATION_X_FORM);
    auto encoded_body = "request="
      + drogon::utils::urlEncodeComponent(request_json);
    request->setBody(encoded_body);

    spdlog::debug("MAST API request: POST {}", path);
    auto [result, response] = client->sendRequest(request, 60.0);

    if (result != drogon::ReqResult::Ok) {
      spdlog::error("MAST API request failed: network error");
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST API request failed", {}
      });
    }

    if (response->getStatusCode() != drogon::k200OK) {
      spdlog::error("MAST API returned status {}",
        static_cast<int>(response->getStatusCode()));
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST API returned non-200 status", {}
      });
    }

    return std::string(response->body());
  } catch (const std::exception& exception) {
    spdlog::error("MAST API exception: {}", exception.what());
    return std::unexpected(Nyx::Core::AppError{
      Nyx::Core::ErrorCode::ExternalServiceError,
      "MAST API request failed", {}
    });
  }
}
```

The JSON payload is URL-encoded and sent as `application/x-www-form-urlencoded`, which is the encoding that the MAST Portal API expects. The 60-second timeout accommodates the occasionally slow MAST responses for large cone searches. Drogon's `sendRequest` returns a structured result that distinguishes network errors (`ReqResult::Ok` vs other values) from HTTP-level errors (non-200 status codes), so both are checked.

=== Name Resolution <name_resolution>

Target name resolution uses the `Mast.Name.Lookup` service. Given a user-supplied target name (e.g. "Pi Mensae", "HAT-P-7", "TIC 261136679"), MAST returns the canonical name, object type, right ascension, and declination:

```cpp
auto request_payload = nlohmann::json{
  {"service", "Mast.Name.Lookup"},
  {"params", {{"input", name}, {"format", "json"}}}
};

auto body_result = this->invoke_mast_service(request_payload.dump());
if (!body_result.has_value()) {
  return std::unexpected(body_result.error());
}

auto response_json = nlohmann::json::parse(body_result.value());
auto resolved = response_json.value("resolvedCoordinate",
  nlohmann::json::array()
);

if (resolved.empty()) {
  return std::unexpected(
    Nyx::Core::AppError::validation("Target could not be resolved")
  );
}

auto first = resolved[0];
auto target = Nyx::Application::Target::ResolvedTarget{
  .canonical_name = first.value("canonicalName", ""),
  .target_type = first.contains("objectType")
    ? std::optional<std::string>(first["objectType"].get<std::string>())
    : std::nullopt,
  .right_ascension = first.contains("ra")
    ? std::optional<double>(first["ra"].get<double>())
    : std::nullopt,
  .declination = first.contains("decl")
    ? std::optional<double>(first["decl"].get<double>())
    : std::nullopt,
};
```

The `resolvedCoordinate` array may contain multiple matches; only the first is used. Optional fields (`objectType`, `ra`, `decl`) are wrapped in `std::optional` because MAST does not guarantee their presence for all object types.

=== TESS Observation Search and Columnar Response Parsing <tess_search>

The `search_tess_timeseries` method queries `Mast.Caom.Filtered.Position` with two filters: `obs_collection = "TESS"` and `dataproduct_type = "timeseries"`. The search uses a cone search centred on the resolved coordinates:

```cpp
auto position = std::to_string(ra) + ", "
  + std::to_string(dec) + ", " + std::to_string(radius);

auto request_payload = nlohmann::json{
  {"service", "Mast.Caom.Filtered.Position"},
  {"params", {
    {"columns", "obsid,t_exptime,t_min,t_max"},
    {"filters", nlohmann::json::array({
      {{"paramName", "obs_collection"}, {"values", {"TESS"}}},
      {{"paramName", "dataproduct_type"}, {"values", {"timeseries"}}}
    })},
    {"position", position},
    {"format", "json"}
  }}
};
```

MAST returns data in a columnar format rather than the row-oriented JSON most APIs use. The response contains `Tables[0].Fields` (column definitions) and `Tables[0].Rows` (data rows). Each row is a positional array, not a named object. The client must first discover column indices by scanning `Fields`, then access row elements by index:

```cpp
auto table = tables[0];
auto fields = table.value("Fields", nlohmann::json::array());
auto rows = table.value("Rows", nlohmann::json::array());

auto obsid_index = -1;
auto exptime_index = -1;
auto tmin_index = -1;
auto tmax_index = -1;

for (auto i = 0; i < static_cast<int>(fields.size()); ++i) {
  auto field_name = fields[i].value("name", "");
  if (field_name == "obsid") obsid_index = i;
  else if (field_name == "t_exptime") exptime_index = i;
  else if (field_name == "t_min") tmin_index = i;
  else if (field_name == "t_max") tmax_index = i;
}
```

This index-based reconstruction is necessary because MAST does not guarantee column ordering. The same pattern is repeated for every columnar MAST endpoint. If any required column is missing and there are data rows present, the method returns an error; if rows are empty, it returns an empty vector (missing columns on an empty result set is not an error because the server may omit column metadata when there are no results).

Row extraction handles type polymorphism in the `obsid` field: MAST sometimes returns it as a string, sometimes as an integer:

```cpp
for (const auto& row : rows) {
  auto obsid_value = row[obsid_index];
  auto obsid = obsid_value.is_string()
    ? obsid_value.get<std::string>()
    : std::to_string(obsid_value.get<int64_t>());

  auto cadence = static_cast<int>(
    exptime_value.get<double>()
  );

  observations.push_back(
    Nyx::Application::Target::MastObservation{
      .obsid = obsid,
      .cadence_seconds = cadence,
      .start_time = tmin_value.get<double>(),
      .end_time = tmax_value.get<double>(),
    }
  );
}
```

=== Product Discovery <product_discovery>

The `get_data_products` method queries `Mast.Caom.Products` for a given observation ID. It uses the same columnar parsing approach to extract the `dataURI`, `description`, `productType`, and `productFilename` fields:

```cpp
auto request_payload = nlohmann::json{
  {"service", "Mast.Caom.Products"},
  {"params", {{"obsid", obsid}, {"format", "json"}}}
};
```

The service layer filters the results to find the TESS light curve FITS file, identified by three criteria: the description is `"Light curves"`, the product type is `"SCIENCE"`, and the filename ends with `_lc.fits`:

```cpp
for (const auto& product : products_result.value()) {
  if (product.description == "Light curves"
    && product.product_type == "SCIENCE"
    && product.filename.ends_with("_lc.fits")) {
    lc_data_uri = product.data_uri;
    break;
  }
}
```

This filter reliably identifies the Pre-search Data Conditioning Simple Aperture Photometry (PDCSAP) light curve files produced by the TESS Science Processing Operations Center (SPOC) pipeline @jenkins2016.

=== FITS File Download with Redirect Following <fits_download>

The `download_fits` method sends an HTTP GET to `https://mast.stsci.edu/api/v0.1/Download/file?uri=<data_uri>`. MAST's download endpoint frequently responds with an HTTP redirect (301, 302, or 307) to a CDN URL rather than serving the file directly. Drogon's HTTP client does not follow redirects automatically, so the `MastClient` implements redirect handling manually:

```cpp
auto MastClient::download_fits(const std::string& data_uri)
  -> Nyx::Core::Result<std::string> {
  auto client = drogon::HttpClient::newHttpClient(
    "https://mast.stsci.edu"
  );
  client->setUserAgent("Nyx/1.0");
  auto full_path = std::string{"/api/v0.1/Download/file"}
    + "?uri=" + data_uri;

  // ... create and send request with 120s timeout ...

  auto status_code = response->getStatusCode();
  if (status_code == drogon::k307TemporaryRedirect
    || status_code == drogon::k302Found
    || status_code == drogon::k301MovedPermanently) {
    auto location = std::string{response->getHeader("Location")};

    if (location.empty()) {
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST redirect missing Location header", {}
      });
    }

    auto scheme_end = location.find("://");
    auto path_start = location.find('/', scheme_end + 3);
    auto redirect_host = (path_start != std::string::npos)
      ? location.substr(0, path_start) : location;
    auto redirect_path = (path_start != std::string::npos)
      ? location.substr(path_start) : "/";

    auto redirect_client =
      drogon::HttpClient::newHttpClient(redirect_host);
    redirect_client->setUserAgent("Nyx/1.0");
    // ... create and send redirect request ...

    auto body = std::string(redirect_response->body());
    spdlog::info("Downloaded FITS file: {} bytes", body.size());
    return body;
  }

  // Direct response (no redirect)
  auto body = std::string(response->body());
  return body;
}
```

The redirect URL is parsed to extract the host (scheme + authority) and path components. A new HTTP client is created for the redirect host because Drogon clients are bound to a single host at construction time. The 120-second timeout on both the initial and redirect requests accommodates large FITS files (typically 5--30 MB for TESS light curves).

== FITS Parser Implementation <fits_parser_impl>

The `FitsParser` implements the `IFitsParser` interface. It uses the CFITSIO library @pence2010 to read TESS light curve FITS files, as described in the design in @fits_design.

=== Temporary File Approach and RAII Cleanup

CFITSIO operates on file descriptors and cannot read from in-memory buffers. Since the FITS data arrives as a `std::string` from the download step, the parser must write it to a temporary file first:

```cpp
auto temp_template = std::string{"/tmp/nyx_fits_XXXXXX"};
auto fd = mkstemp(temp_template.data());
if (fd < 0) {
  spdlog::error("Failed to create temp file for FITS data");
  return std::unexpected(
    Nyx::Core::AppError::internal("Failed to create temp file")
  );
}

auto guard = FitsFileGuard{temp_template};

auto bytes_written = write(fd, fits_data.data(), fits_data.size());
close(fd);
```

The `FitsFileGuard` is a RAII class that deletes the temporary file when the function exits, regardless of the exit path:

```cpp
class FitsFileGuard {
  public:
    explicit FitsFileGuard(const std::string& path) : path(path) {}
    ~FitsFileGuard() {
      if (!this->path.empty()) {
        std::remove(this->path.c_str());
      }
    }
    FitsFileGuard(const FitsFileGuard&) = delete;
    auto operator=(const FitsFileGuard&) -> FitsFileGuard& = delete;
  private:
    std::string path;
};
```

This ensures no temporary files are leaked, even when the parser returns early due to a CFITSIO error.

=== HDU Navigation and Column Discovery

TESS light curve FITS files follow the standard structure: HDU 1 contains the primary header, and HDU 2 contains the binary table extension with the time-series data @pence2010. The parser moves to HDU 2 and reads the row count:

```cpp
fitsfile* fptr = nullptr;
auto status = 0;
fits_open_file(&fptr, temp_template.c_str(), READONLY, &status);

fits_movabs_hdu(fptr, 2, nullptr, &status);
if (status != 0) {
  char error_text[FLEN_STATUS];
  fits_get_errstatus(status, error_text);
  spdlog::error("Failed to move to HDU 2: {}", error_text);
  fits_close_file(fptr, &status);
  return std::unexpected(Nyx::Core::AppError{
    Nyx::Core::ErrorCode::ExternalServiceError,
    "FITS file missing binary table extension", {}
  });
}

long num_rows = 0;
fits_get_num_rows(fptr, &num_rows, &status);
```

Column indices are discovered by name using `fits_get_colnum` with case-insensitive matching. The `TIME` column is required; all other columns (`PDCSAP_FLUX`, `PDCSAP_FLUX_ERR`, `SAP_FLUX`, `QUALITY`) are optional. If a column is missing, its index is set to zero and the corresponding values in the output are set to `std::nullopt`:

```cpp
auto time_col = 0;
auto pdcsap_col = 0;
// ...

fits_get_colnum(fptr, CASEINSEN,
  const_cast<char*>("TIME"), &time_col, &status);
if (status != 0) {
  spdlog::error("FITS file missing TIME column");
  fits_close_file(fptr, &status);
  return std::unexpected(Nyx::Core::AppError{
    Nyx::Core::ErrorCode::ExternalServiceError,
    "FITS file missing TIME column", {}
  });
}

status = 0;
fits_get_colnum(fptr, CASEINSEN,
  const_cast<char*>("PDCSAP_FLUX"), &pdcsap_col, &status);
if (status != 0) {
  spdlog::warn("FITS file missing PDCSAP_FLUX column");
  pdcsap_col = 0;
  status = 0;
}
```

The `const_cast<char*>` is necessary because CFITSIO's C API takes a non-const `char*` for the column name parameter, despite not modifying it. The status variable is reset to zero between column lookups because CFITSIO accumulates errors --- a non-zero status from a missing optional column would cause all subsequent calls to fail.

=== Bulk Column Reading into Contiguous Arrays

Rather than reading the FITS table row by row, the parser reads entire columns at once into pre-allocated contiguous `std::vector` arrays. This is both simpler and significantly faster, as CFITSIO can read the entire column in a single I/O operation:

```cpp
auto time_values = std::vector<double>(num_rows);
auto pdcsap_values = std::vector<float>(num_rows);
auto pdcsap_err_values = std::vector<float>(num_rows);
auto sap_values = std::vector<float>(num_rows);
auto quality_values = std::vector<int>(num_rows);

auto any_null = 0;

fits_read_col(fptr, TDOUBLE, time_col, 1, 1, num_rows,
  nullptr, time_values.data(), &any_null, &status);

if (pdcsap_col > 0) {
  auto nan_val = std::numeric_limits<float>::quiet_NaN();
  fits_read_col(fptr, TFLOAT, pdcsap_col, 1, 1, num_rows,
    &nan_val, pdcsap_values.data(), &any_null, &status);
}
```

For nullable float columns, a NaN sentinel value is passed as the `nulval` parameter. CFITSIO replaces null entries in the FITS table with this sentinel, which the post-processing loop then checks. The `QUALITY` column uses integer type (`TINT`) and does not require null handling because TESS always populates this field.

=== NaN Filtering and Point Construction

The final loop iterates over the arrays, skipping rows where the timestamp is NaN (indicating missing or invalid data), and constructs `LightCurvePoint` structs:

```cpp
auto points = std::vector<Nyx::Domain::LightCurvePoint>{};
points.reserve(num_rows);

for (auto i = long{0}; i < num_rows; ++i) {
  if (std::isnan(time_values[i])) continue;

  auto point = Nyx::Domain::LightCurvePoint{
    .id = 0,
    .tess_observation_id = "",
    .time = time_values[i],
    .pdcsap_flux = (pdcsap_col > 0
      && !std::isnan(pdcsap_values[i]))
      ? std::optional<float>(pdcsap_values[i])
      : std::nullopt,
    .pdcsap_flux_err = (pdcsap_err_col > 0
      && !std::isnan(pdcsap_err_values[i]))
      ? std::optional<float>(pdcsap_err_values[i])
      : std::nullopt,
    .sap_flux = (sap_col > 0
      && !std::isnan(sap_values[i]))
      ? std::optional<float>(sap_values[i])
      : std::nullopt,
    .quality = (quality_col > 0)
      ? quality_values[i] : 0,
  };

  points.push_back(point);
}

spdlog::info("Parsed {} light curve points from FITS ({} rows total)",
  points.size(), num_rows);
return points;
```

The `id` and `tess_observation_id` fields are set to placeholder values (0 and empty string) because they are assigned by the database during bulk insert. A typical TESS single-sector 2-minute cadence observation contains approximately 18,000--20,000 rows; after NaN filtering, roughly 17,000--18,000 valid points remain.

The memory layout of the intermediate arrays (`std::vector<double>`, `std::vector<float>`) is contiguous, which enables efficient cache-line utilisation during the filtering loop. The `reserve(num_rows)` call on the output vector avoids reallocations during the push-back loop.

== Data Ingestion Pipeline Implementation <pipeline_impl>

The `TargetService` orchestrates the four-stage data ingestion pipeline described in @data_pipeline. It depends on six interfaces, all injected via constructor:

```cpp
TargetService::TargetService(
  std::shared_ptr<IMastClient> mast_client,
  std::shared_ptr<Nyx::Domain::ITargetRepository> target_repository,
  std::shared_ptr<Nyx::Domain::ITessObservationRepository>
    tess_observation_repository,
  std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator,
  std::shared_ptr<Nyx::Domain::ILightCurvePointRepository>
    light_curve_point_repository,
  std::shared_ptr<IFitsParser> fits_parser
)
  : mast_client(std::move(mast_client))
  , target_repository(std::move(target_repository))
  , tess_observation_repository(std::move(tess_observation_repository))
  , uuid_generator(std::move(uuid_generator))
  , light_curve_point_repository(std::move(light_curve_point_repository))
  , fits_parser(std::move(fits_parser)) {}
```

All dependencies are interfaces (pure abstract classes), which means the service can be tested with mock implementations --- a property exploited extensively in the test suite, as discussed in @architecture.

=== Stage 1--2: `resolve_target` --- Name Resolution and Observation Cataloguing <resolve_target_impl>

The `resolve_target` method implements the first two stages of the pipeline. It follows a cache-first strategy: if the target has already been resolved and stored, it returns the cached record with its associated TESS observations rather than querying MAST again:

```cpp
auto resolved = this->mast_client->resolve_target(request.target_name);
if (!resolved.has_value()) {
  return std::unexpected(resolved.error());
}

auto cached = this->target_repository->find_by_canonical_name(
  resolved->canonical_name
);
if (!cached.has_value()) {
  return std::unexpected(cached.error());
}

if (cached->has_value()) {
  auto existing = cached->value();
  logger->info("Target '{}' already cached as id={}",
    existing.canonical_name, existing.id);

  auto observations = this->build_observation_responses(
    existing.id, logger
  );
  if (!observations.has_value()) {
    return std::unexpected(observations.error());
  }
  return TargetResponse{
    .id = existing.id,
    .canonical_name = existing.canonical_name,
    .target_type = existing.target_type,
    .right_ascension = existing.right_ascension,
    .declination = existing.declination,
    .tess_observations = observations.value(),
  };
}
```

If the target is not cached, a new record is created and the TESS observation search is triggered. The search results are filtered to exclude multi-sector stacked observations (those spanning more than 30 days), which would contain duplicate data from individual sector observations:

```cpp
constexpr auto max_single_sector_days = 30.0;

auto single_sector_obs = std::vector<MastObservation>{};
for (const auto& mast_obs : tess_result.value()) {
  auto span_days = mast_obs.end_time - mast_obs.start_time;
  if (span_days > max_single_sector_days) {
    logger->debug(
      "Skipping multi-sector observation obsid={} (span={:.1f} days)",
      mast_obs.obsid, span_days
    );
    continue;
  }
  single_sector_obs.push_back(mast_obs);
}
```

A TESS sector is approximately 27 days @ricker2015, so the 30-day threshold provides a small margin while reliably filtering out multi-sector stacks (which typically span 54+ days).

To ensure idempotent re-ingestion, the method queries the database for already-stored observation IDs and filters them out before the bulk insert:

```cpp
auto existing_result =
  this->tess_observation_repository->find_existing_obsids(obsids);
// ...
auto new_observations = std::vector<Nyx::Domain::TessObservation>{};
for (const auto& mast_obs : single_sector_obs) {
  if (existing_obsids.contains(mast_obs.obsid)) {
    logger->debug("TESS observation obsid={} already exists, skipping",
      mast_obs.obsid);
    continue;
  }
  new_observations.push_back(Nyx::Domain::TessObservation{
    .id = this->uuid_generator->generate(),
    .target_id = created.id,
    .obsid = mast_obs.obsid,
    .cadence_seconds = mast_obs.cadence_seconds,
    .start_time = mast_obs.start_time,
    .end_time = mast_obs.end_time,
    .data_uri = std::nullopt,
  });
}

if (!new_observations.empty()) {
  auto bulk_result =
    this->tess_observation_repository->bulk_create(new_observations);
  // ...
}
```

This `find_existing_obsids` check avoids violating the unique constraint on `obsid` and eliminates redundant database writes when the same target is resolved multiple times.

=== Stage 3: `discover_products` --- FITS Data Product Selection

The `discover_products` method implements stage 3: querying MAST for data products associated with a specific observation and selecting the light curve FITS file. The method is idempotent --- if the observation already has a `data_uri`, it returns immediately:

```cpp
if (observation.data_uri.has_value()) {
  logger->info(
    "Observation {} already has data_uri, skipping discovery",
    observation_id
  );
  return TessObservationResponse{ /* ... */ };
}

auto products_result = this->mast_client->get_data_products(
  observation.obsid
);
```

The FITS file selection criteria (`"Light curves"` + `"SCIENCE"` + `_lc.fits` suffix) are applied to find the SPOC-produced PDCSAP light curve among potentially dozens of data products per observation.

=== Stage 4: `fetch_light_curve` --- Download, Parse, and Store <fetch_light_curve_impl>

The `fetch_light_curve` method implements the final stage. It includes a caching check: if light curve points already exist in the database for this observation, the method returns metadata without re-downloading:

```cpp
auto count_result =
  this->light_curve_point_repository->count_by_observation_id(
    observation_id
  );
if (!count_result.has_value()) {
  return std::unexpected(count_result.error());
}

if (count_result.value() > 0) {
  logger->info(
    "Light curve already fetched for observation_id={} ({} points)",
    observation_id, count_result.value()
  );
  // Return cached metadata (point_count, time_min, time_max)
}
```

If no cached data exists, the method proceeds through download, parse, and bulk insert:

```cpp
auto download_result = this->mast_client->download_fits(
  observation.data_uri.value()
);
// ... error handling ...

auto parse_result = this->fits_parser->parse_light_curve(
  download_result.value()
);
// ... error handling ...

auto& points = parse_result.value();
logger->info("Inserting {} light curve points for observation_id={}",
  points.size(), observation_id);

auto insert_result =
  this->light_curve_point_repository->bulk_create(
    observation_id, points
  );
```

The method returns a `LightCurveMetadataResponse` containing the point count, minimum time, and maximum time --- not the full point data, which can be retrieved separately via `get_light_curve`.

=== Performance Timing in `get_light_curve`

The `get_light_curve` method includes explicit timing instrumentation to monitor database query performance for large result sets:

```cpp
auto db_start = std::chrono::steady_clock::now();

auto points_result =
  this->light_curve_point_repository->find_by_observation_id(
    observation_id, quality_filter
  );

auto db_elapsed =
  std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - db_start
  ).count();
logger->debug("Light curve DB fetch took {}ms ({} points)",
  db_elapsed,
  points_result.has_value() ? points_result->size() : 0);
```

This timing data is logged at the debug level and is used during development to identify slow queries. For a typical 18,000-point light curve, the fetch completes in 10--50ms depending on database load.

== Bulk Insert Optimisation <bulk_insert>

Inserting 18,000 light curve points one row at a time would require 18,000 round trips to PostgreSQL @postgresql2024. The `PostgresLightCurvePointRepository` instead uses batched multi-row `INSERT` statements with parameterised placeholders:

```cpp
static constexpr auto BATCH_SIZE = 5000;
static constexpr auto PARAMS_PER_ROW = 6;

auto PostgresLightCurvePointRepository::bulk_create(
  const std::string& observation_id,
  const std::vector<Nyx::Domain::LightCurvePoint>& points
) -> Nyx::Core::Result<int> {
  if (points.empty()) return 0;

  try {
    auto db = drogon::app().getDbClient();
    auto transaction = db->newTransaction();
    auto total_inserted = 0;

    for (auto batch_start = size_t{0};
      batch_start < points.size();
      batch_start += BATCH_SIZE) {
      auto batch_end = std::min(
        batch_start + BATCH_SIZE, points.size()
      );
      auto batch_size = static_cast<int>(batch_end - batch_start);

      auto sql = std::ostringstream{};
      sql << "INSERT INTO light_curve_points "
          << "(tess_observation_id, time, pdcsap_flux, "
          << "pdcsap_flux_err, sap_flux, quality) VALUES ";

      for (auto i = 0; i < batch_size; ++i) {
        if (i > 0) sql << ", ";
        auto offset = i * PARAMS_PER_ROW;
        sql << "($" << (offset + 1)
            << ", $" << (offset + 2)
            << ", CAST(NULLIF($" << (offset + 3)
            << ", '') AS REAL)"
            << ", CAST(NULLIF($" << (offset + 4)
            << ", '') AS REAL)"
            << ", CAST(NULLIF($" << (offset + 5)
            << ", '') AS REAL)"
            << ", $" << (offset + 6) << ")";
      }

      auto query = sql.str();
      auto binder = *transaction << query;

      for (auto i = batch_start; i < batch_end; ++i) {
        const auto& point = points[i];
        binder << observation_id;
        binder << std::to_string(point.time);
        binder << optional_float_to_string(point.pdcsap_flux);
        binder << optional_float_to_string(point.pdcsap_flux_err);
        binder << optional_float_to_string(point.sap_flux);
        binder << std::to_string(point.quality);
      }

      binder << drogon::orm::Mode::Blocking;
      binder >> [](const drogon::orm::Result&) {};
      binder.exec();

      total_inserted += batch_size;
    }
    return total_inserted;
  } catch (const drogon::orm::DrogonDbException& exception) {
    spdlog::error(
      "Database error bulk inserting light curve points: {}",
      exception.base().what()
    );
    return std::unexpected(
      Nyx::Core::AppError::internal("Failed to insert light curve points")
    );
  }
}
```

Several design decisions are visible here. First, the batch size of 5,000 was chosen empirically: it stays well within PostgreSQL's parameter limit (65,535) at 30,000 parameters per batch (6 per row times 5,000 rows), while being large enough to amortise the per-statement overhead. Second, nullable float columns use the `CAST(NULLIF($N, '') AS REAL)` pattern: optional values are serialised as empty strings when absent, and `NULLIF` converts the empty string to SQL `NULL` before `CAST` converts the non-empty string to `REAL`. This avoids the complexity of Drogon's `std::optional` parameter binding, which does not reliably work with its SQL binder for bulk operations. Third, the entire operation is wrapped in a transaction, ensuring atomicity: either all points are inserted or none are.

=== Server-Side JSON Aggregation <json_aggregation>

When the frontend requests light curve data for rendering, the naive approach would be to fetch 18,000 rows from PostgreSQL, map each row to a C++ struct, serialise each struct to JSON, and assemble the JSON array. This involves 18,000 `nlohmann::json` object constructions and a large number of small heap allocations.

The `find_by_observation_id_as_json` method delegates JSON construction entirely to PostgreSQL using `json_agg` and `json_build_object`:

```cpp
auto query = std::string{
  "SELECT COALESCE(json_agg("
  "json_build_object("
  "'time', time, "
  "'pdcsap_flux', pdcsap_flux, "
  "'pdcsap_flux_err', pdcsap_flux_err, "
  "'sap_flux', sap_flux, "
  "'quality', quality"
  ") ORDER BY time ASC), '[]'::json)::text "
  "FROM light_curve_points "
  "WHERE tess_observation_id = $1"
};

if (quality_filter) {
  query += " AND quality = 0";
}

auto result = db->execSqlSync(query, observation_id);
return result[0][0].as<std::string>();
```

PostgreSQL returns a single text value containing the complete JSON array. The C++ layer receives this as a `std::string` and inserts it directly into the response body without parsing or re-serialising it. The controller constructs the response envelope using string concatenation rather than `nlohmann::json` to avoid parsing the large JSON array:

```cpp
auto buf = std::string{};
buf.reserve(result->points_json.size() + 256);

buf += "{\"data\":{\"tess_observation_id\":\"";
buf += result->tess_observation_id;
buf += "\",\"obsid\":\"";
buf += result->obsid;
buf += "\",\"point_count\":";
buf += std::to_string(result->point_count);
buf += ",\"points\":";
buf += result->points_json;
buf += "},\"meta\":{\"request_id\":\"";
buf += correlation_id;
buf += "\",\"timestamp\":\"";
buf += trantor::Date::now().toFormattedString(false);
buf += "\"}}";

auto response = drogon::HttpResponse::newHttpResponse();
response->setStatusCode(drogon::k200OK);
response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
response->setBody(std::move(buf));
callback(response);
```

The `reserve` call pre-allocates buffer space based on the JSON string size plus a small overhead for the envelope, avoiding reallocations during concatenation. This approach eliminates all C++ JSON serialisation overhead for light curve data and transfers the serialisation work to PostgreSQL's highly optimised JSON functions.

== Authentication Implementation <auth_impl>

The authentication system implements the architecture described in @auth_design, covering password hashing, JWT token management, refresh token rotation, Google OAuth2 integration, and email verification.

=== Password Hashing: Argon2id via libsodium <password_hashing>

Password hashing uses Argon2id @biryukov2016, accessed through libsodium's high-level `crypto_pwhash_str` API. Argon2id combines the side-channel resistance of Argon2i with the GPU-resistance of Argon2d, making it the recommended choice for password hashing @owasp_password2024:

```cpp
auto ArgonPasswordHasher::hash(
  const std::string& password
) -> Nyx::Core::Result<std::string> {
  char hashed_password[crypto_pwhash_STRBYTES];

  if (crypto_pwhash_str(
        hashed_password,
        password.c_str(),
        password.length(),
        crypto_pwhash_OPSLIMIT_MODERATE,
        crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
    return std::unexpected(
      Nyx::Core::AppError::internal("Password hashing failed")
    );
  }

  return std::string(hashed_password);
}

auto ArgonPasswordHasher::verify(
  const std::string& password,
  const std::string& hash
) -> bool {
  return crypto_pwhash_str_verify(
    hash.c_str(),
    password.c_str(),
    password.length()
  ) == 0;
}
```

The `OPSLIMIT_MODERATE` and `MEMLIMIT_MODERATE` parameters configure Argon2id to use moderate CPU and memory costs, balancing security against server load. The `crypto_pwhash_str` function produces a self-describing output string that includes the algorithm identifier, salt, parameters, and hash, so no separate salt storage is needed. The `crypto_pwhash_str_verify` function performs constant-time comparison internally, preventing timing attacks.

The constructor initialises libsodium, which is a required one-time setup:

```cpp
ArgonPasswordHasher::ArgonPasswordHasher() {
  if (sodium_init() < 0) {
    spdlog::critical("Failed to initialize libsodium");
    throw std::runtime_error("Failed to initialize libsodium");
  }
}
```

This is the only place in the codebase where an exception is thrown rather than returning a `Result`, because libsodium initialisation failure is a fatal startup error --- the application cannot function without cryptographic primitives.

=== JWT Token Generation and Verification <jwt_impl>

The `JwtTokenService` generates access/refresh token pairs and verifies them using HS256 (HMAC-SHA256) @jones2015. Access tokens carry a 15-minute expiry; refresh tokens carry a 7-day expiry:

```cpp
auto JwtTokenService::generate_token_pair(
  const std::string& user_id,
  const std::string& email,
  const std::string& token_id,
  const std::optional<std::string>& family_id
) -> Nyx::Application::Auth::TokenPair {
  auto now = std::chrono::system_clock::now();
  auto resolved_family_id = family_id.value_or(token_id);

  auto access_token = jwt::create()
    .set_issuer("nyx")
    .set_type("access")
    .set_issued_at(now)
    .set_expires_at(now + std::chrono::seconds(
      this->config->jwt_access_token_expiry_seconds()
    ))
    .set_payload_claim("sub", jwt::claim(user_id))
    .set_payload_claim("email", jwt::claim(email))
    .sign(jwt::algorithm::hs256{this->config->jwt_secret()});

  auto refresh_token = jwt::create()
    .set_issuer("nyx")
    .set_type("refresh")
    .set_id(token_id)
    .set_issued_at(now)
    .set_expires_at(refresh_expiry)
    .set_payload_claim("sub", jwt::claim(user_id))
    .set_payload_claim("email", jwt::claim(email))
    .set_payload_claim("fid", jwt::claim(resolved_family_id))
    .sign(jwt::algorithm::hs256{this->config->jwt_secret()});

  return Nyx::Application::Auth::TokenPair{
    .access_token = std::move(access_token),
    .refresh_token = std::move(refresh_token),
    .refresh_token_id = token_id,
    .refresh_token_family_id = resolved_family_id,
    .refresh_token_expires_at = expiry_stream.str(),
  };
}
```

The `type` claim distinguishes access tokens from refresh tokens at the verification stage. The `fid` (family ID) claim in the refresh token links all tokens in a rotation chain to a common family, enabling family-wide revocation on reuse detection.

Token verification uses the `jwt-cpp` library's verifier, which checks the signature, issuer, and type:

```cpp
auto JwtTokenService::verify_access_token(
  const std::string& token
) -> Nyx::Core::Result<Nyx::Application::Auth::TokenClaims> {
  try {
    auto decoded_token = jwt::decode(token);
    auto verifier = jwt::verify()
      .allow_algorithm(jwt::algorithm::hs256{this->config->jwt_secret()})
      .with_issuer("nyx")
      .with_type("access");
    verifier.verify(decoded_token);

    return Nyx::Application::Auth::TokenClaims{
      .user_id = decoded_token.get_payload_claim("sub").as_string(),
      .email = decoded_token.get_payload_claim("email").as_string(),
    };
  } catch (const jwt::error::token_verification_exception& exception) {
    return std::unexpected(
      Nyx::Core::AppError::invalid_token("Access token is invalid")
    );
  }
}
```

=== Token Hashing with SHA-256 <token_hashing>

Refresh tokens are never stored in plaintext in the database. Instead, a SHA-256 hash is computed via libsodium and stored. This way, a database breach does not directly expose valid refresh tokens @bernstein2012:

```cpp
auto JwtTokenService::hash_token(
  const std::string& token
) -> std::string {
  unsigned char hash[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(
    hash,
    reinterpret_cast<const unsigned char*>(token.c_str()),
    token.size()
  );

  char hex[crypto_hash_sha256_BYTES * 2 + 1];
  sodium_bin2hex(hex, sizeof(hex), hash, crypto_hash_sha256_BYTES);
  return std::string(hex);
}
```

The hash is stored as a lowercase hexadecimal string. When a refresh token is presented for rotation, the same hashing function is applied and the result is used to look up the stored record.

=== Refresh Token Rotation with Family-Based Reuse Detection <token_rotation>

The `refresh_access_token` method in `AuthService` implements the rotation logic described in @auth_design. When a valid refresh token is presented:

1. The token is verified (signature + expiry).
2. The SHA-256 hash is computed and used to look up the stored record.
3. If the stored record is revoked, the entire token family is revoked and an error is returned.
4. Otherwise, the current token is revoked and a new token pair is issued with the same family ID.

```cpp
auto stored_token = stored_token_result->value();

if (stored_token.is_revoked) {
  logger->warn(
    "Reuse detected for token family_id={}, revoking entire family",
    stored_token.family_id
  );
  auto revoke_result =
    this->refresh_token_repository->revoke_family(
      stored_token.family_id
    );
  return std::unexpected(
    Nyx::Core::AppError::invalid_token(
      "Refresh token has been revoked"
    )
  );
}

auto revoke_old_result =
  this->refresh_token_repository->revoke(stored_token.id);
// ...

auto new_token_pair =
  this->token_service->generate_token_pair(
    claims.user_id, claims.email,
    new_token_id, stored_token.family_id
  );
```

The reuse detection works as follows @lodderstedt2020: if an attacker steals a refresh token and uses it, the legitimate user's next refresh attempt will present a token that has already been revoked. This triggers family-wide revocation, invalidating both the attacker's and the user's tokens. The user must re-authenticate, but the attacker loses access.

=== Google OAuth2 Integration <google_oauth>

The `GoogleAuthClient` exchanges an authorization code for user information by POSTing to Google's token endpoint @hardt2012:

```cpp
auto GoogleAuthClient::exchange_code(
  const std::string& code,
  const std::string& redirect_uri
) -> Nyx::Core::Result<Nyx::Application::Auth::GoogleUserInfo> {
  // ...
  auto http_client = drogon::HttpClient::newHttpClient(
    "https://oauth2.googleapis.com"
  );

  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Post);
  request->setPath("/token");
  request->setContentTypeCode(drogon::CT_APPLICATION_X_FORM);
  request->setBody(
    "code=" + code
    + "&client_id=" + client_id
    + "&client_secret=" + client_secret
    + "&redirect_uri=" + redirect_uri
    + "&grant_type=authorization_code"
  );

  auto [result, response] = http_client->sendRequest(request, 10.0);
  // ... error handling ...

  auto response_body = nlohmann::json::parse(response->body());
  auto id_token_str = response_body["id_token"].get<std::string>();
  auto decoded_token = jwt::decode(id_token_str);

  auto google_id = decoded_token.get_payload_claim("sub").as_string();
  auto email = decoded_token.get_payload_claim("email").as_string();
  // ...

  return Nyx::Application::Auth::GoogleUserInfo{
    .google_id = std::move(google_id),
    .email = std::move(email),
    .email_verified = email_verified,
    .name = std::move(name),
  };
}
```

The ID token is decoded without signature verification because it was received directly from Google over HTTPS --- verifying the TLS connection is sufficient per the OpenID Connect specification @openid2014. The `sub` claim contains the stable Google user ID.

The `AuthService::google_login` method handles three cases for the user lookup:

```cpp
// Case 1: Existing user found by Google ID
auto existing_google_user =
  this->user_repository->find_by_google_id(google_user.google_id);
if (existing_google_user->has_value()) {
  auto user = existing_google_user->value();
  // Issue tokens and return
}

// Case 2: Email conflict with local account
auto existing_email_user =
  this->user_repository->find_by_email(google_user.email);
if (existing_email_user->has_value()) {
  return std::unexpected(Nyx::Core::AppError::conflict(
    "An account with this email already exists. "
    "Please log in with your password."
  ));
}

// Case 3: New user
auto new_user = Nyx::Domain::User{
  .id = this->uuid_generator->generate(),
  .email = google_user.email,
  .password_hash = std::nullopt,
  .display_name = google_user.name,
  .email_verified = true,
  .auth_provider = "google",
  .google_id = google_user.google_id,
};
auto create_result = this->user_repository->create(new_user);
```

Google-authenticated users are created with `email_verified = true` (Google has already verified the email), `password_hash = std::nullopt` (no local password), and `auth_provider = "google"`. The conflict case prevents automatic account linking: if a user registered with email/password and later tries Google login with the same email, they receive an explicit error rather than silently merging accounts, which would be a security risk.

=== Email Verification Flow <email_verification>

When a user registers with email/password, the `AuthService` generates a verification token, hashes it with SHA-256, stores the hash with a 24-hour expiry in the `verification_tokens` table, and sends the raw token to the user's email. The verification endpoint looks up the token by hash, checks expiry, marks it as used, and sets `email_verified = true` on the user record.

The email sending infrastructure uses a strategy pattern. `IEmailSender` is the interface, with two implementations selected at startup based on configuration:

```cpp
auto email_sender = [&config]()
  -> std::shared_ptr<Nyx::Application::Auth::IEmailSender> {
  if (!config->smtp_host().empty()) {
    return std::make_shared<
      Nyx::Infrastructure::Email::SmtpEmailSender
    >(config);
  }
  return std::make_shared<
    Nyx::Infrastructure::Email::ConsoleEmailSender
  >(config);
}();
```

`SmtpEmailSender` sends real emails via SMTP using libcurl with TLS and authentication. `ConsoleEmailSender` logs the verification URL to stdout, used during development when no SMTP server is configured.

== Middleware Pipeline Implementation <middleware_impl>

Every inbound HTTP request passes through a chain of Drogon filters before reaching the controller, implementing the pipeline described in @middleware_design.

=== CorrelationIdFilter: Request Tracing <correlation_filter>

The `CorrelationIdFilter` generates a unique correlation ID for each request and creates a request-scoped logger that includes the ID in every log line:

```cpp
auto CorrelationIdFilter::doFilter(
  const drogon::HttpRequestPtr& request,
  drogon::FilterCallback&& /*failure_callback*/,
  drogon::FilterChainCallback&& chain_callback
) -> void {
  auto correlation_id =
    std::string(request->getHeader("X-Request-Id"));

  if (correlation_id.empty()) {
    correlation_id = drogon::utils::getUuid();
  }

  request->addHeader("X-Correlation-Id", correlation_id);
  request->getAttributes()->insert("correlation_id", correlation_id);

  auto request_logger =
    Nyx::Core::Logger::create_request_logger(correlation_id);
  request->getAttributes()->insert("logger", request_logger);

  request_logger->debug("Incoming {} {}",
    request->methodString(), request->path());

  chain_callback();
}
```

If the client provides an `X-Request-Id` header, it is used as the correlation ID; otherwise a UUID is generated. The logger is stored in Drogon's request attributes, which act as a request-scoped key-value store accessible by all downstream filters and the controller. This avoids thread-local storage, which is unreliable in Drogon's event-loop-based architecture.

=== RateLimitFilter: Sliding Window Algorithm <rate_limit_filter>

The `RateLimitFilter` implements a sliding window rate limiter with an in-memory store, keyed by client IP address:

```cpp
auto RateLimitFilter::doFilter(
  const drogon::HttpRequestPtr& request,
  drogon::FilterCallback&& failure_callback,
  drogon::FilterChainCallback&& chain_callback
) -> void {
  auto client_ip = request->peerAddr().toIp();
  auto now = std::chrono::steady_clock::now();

  auto lock = std::lock_guard<std::mutex>{this->mutex};

  auto& timestamps = this->requests[client_ip];

  while (!timestamps.empty() && (now - timestamps.front()) > window) {
    timestamps.pop_front();
  }

  if (static_cast<int>(timestamps.size()) >= max_requests) {
    auto error = Nyx::Core::AppError{
      Nyx::Core::ErrorCode::RateLimitExceeded,
      "Too many requests, please try again later", {}
    };
    failure_callback(
      Nyx::Presentation::Http::ResponseHelper::error(
        error, correlation_id
      )
    );
    return;
  }

  timestamps.push_back(now);
  chain_callback();
}
```

The algorithm maintains a `std::deque<time_point>` per IP. On each request, expired timestamps (older than the 1-minute window) are removed from the front. If the remaining count exceeds the limit (10 requests), a 429 response is returned. The `std::mutex` protects against concurrent access from Drogon's thread pool. The window and limit are compile-time constants:

```cpp
static constexpr auto max_requests = 10;
static constexpr auto window = std::chrono::minutes(1);
```

This filter is applied only to authentication endpoints (`/api/v1/auth/*`), where brute-force protection is most critical. Data retrieval endpoints do not carry the rate limit filter.

=== JwtAuthFilter: Bearer Token Verification <jwt_auth_filter>

The `JwtAuthFilter` extracts the JWT from the `Authorization` header, verifies it, and injects the authenticated user's ID and email into the request attributes:

```cpp
auto JwtAuthFilter::doFilter(
  const drogon::HttpRequestPtr& request,
  drogon::FilterCallback&& failure_callback,
  drogon::FilterChainCallback&& chain_callback
) -> void {
  // ...
  auto authorization_header =
    std::string(request->getHeader("Authorization"));

  if (authorization_header.empty()
      || authorization_header.substr(0, 7) != "Bearer ") {
    auto error = Nyx::Core::AppError::unauthorized(
      "Bearer token is required"
    );
    failure_callback(
      Nyx::Presentation::Http::ResponseHelper::error(
        error, correlation_id
      )
    );
    return;
  }

  auto token = authorization_header.substr(7);
  auto claims_result =
    this->token_service->verify_access_token(token);

  if (!claims_result.has_value()) {
    failure_callback(
      Nyx::Presentation::Http::ResponseHelper::error(
        claims_result.error(), correlation_id
      )
    );
    return;
  }

  auto claims = claims_result.value();
  request->getAttributes()->insert("user_id", claims.user_id);
  request->getAttributes()->insert("user_email", claims.email);

  logger->debug("Authenticated user_id={}, email={}",
    claims.user_id, claims.email);

  chain_callback();
}
```

The filter constructs its own `JwtTokenService` in the constructor, ensuring the JWT secret is loaded once at startup. If verification fails, the error (which may be `INVALID_TOKEN` or `TOKEN_EXPIRED`) is passed directly to the failure callback, and the request never reaches the controller.

=== CsrfFilter: Constant-Time Token Comparison <csrf_filter>

The `CsrfFilter` protects state-changing endpoints against cross-site request forgery by comparing the `csrf_token` cookie with the `X-CSRF-Token` header using `sodium_memcmp` @owasp2024:

```cpp
auto CsrfFilter::doFilter(
  const drogon::HttpRequestPtr& request,
  drogon::FilterCallback&& failure_callback,
  drogon::FilterChainCallback&& chain_callback
) -> void {
  auto method = request->method();
  if (method == drogon::Get
    || method == drogon::Head
    || method == drogon::Options) {
    chain_callback();
    return;
  }

  auto cookie_token = request->getCookie("csrf_token");
  auto header_token = std::string{
    request->getHeader("X-CSRF-Token")
  };

  if (cookie_token.empty() || header_token.empty()) {
    // Return 403
  }

  if (cookie_token.size() != header_token.size()
    || sodium_memcmp(
        cookie_token.data(),
        header_token.data(),
        cookie_token.size()
      ) != 0) {
    // Return 403
  }

  chain_callback();
}
```

The `sodium_memcmp` function performs constant-time comparison, preventing timing side-channel attacks that could allow an attacker to determine the CSRF token one byte at a time. GET, HEAD, and OPTIONS requests are exempt because they should be safe (idempotent and side-effect-free).

=== Filter Chain Declaration

Controllers declare their filter chains via Drogon's `ADD_METHOD_TO` macro. The order of filter names determines execution order. Authentication routes use `CorrelationIdFilter` and `RateLimitFilter`:

```cpp
ADD_METHOD_TO(
  AuthController::login,
  "/api/v1/auth/login",
  drogon::Post,
  "Nyx::Presentation::Middleware::CorrelationIdFilter",
  "Nyx::Presentation::Middleware::RateLimitFilter"
);
```

Data endpoints use only `CorrelationIdFilter`:

```cpp
ADD_METHOD_TO(
  TessObservationController::get_light_curve,
  "/api/v1/tess-observations/{1}/light-curve",
  drogon::Get,
  "Nyx::Presentation::Middleware::CorrelationIdFilter"
);
```

The `{1}` placeholder in the URL pattern is Drogon's syntax for a path parameter, which is passed as the third argument to the handler method.

== Image Processing Implementation <image_processing>

The image processing subsystem handles ground-based observation images uploaded by users. It consists of three components, each behind an interface: EXIF metadata extraction, DNG raw image decoding, and aperture photometry.

=== EXIF Metadata Extraction <exif_impl>

The `IExifParser` interface defines a single method for extracting metadata from observation images:

```cpp
struct ExifData {
  std::optional<std::string> captured_at;
  std::optional<std::string> camera_model;
  std::optional<double> exposure_time_seconds;
  std::optional<int> iso_speed;
  std::optional<double> gps_latitude;
  std::optional<double> gps_longitude;
  std::optional<int> image_width;
  std::optional<int> image_height;
};

class IExifParser {
  public:
    virtual ~IExifParser() = default;
    virtual auto parse(const std::string& file_path)
      -> Nyx::Core::Result<ExifData> = 0;
};
```

The `ExifData` struct uses `std::optional` for every field because no EXIF tag is guaranteed to be present. The `captured_at` field is particularly important: it provides the observation timestamp, which is later converted to Julian Date for comparison with TESS data. The implementation uses the Exiv2 library to read EXIF tags from JPEG, PNG, TIFF, and DNG files.

=== DNG Raw Image Decoding <dng_impl>

Raw images from the telescope camera (typically in DNG format) must be decoded into a pixel array for photometric analysis. The `IDngDecoder` interface and `DecodedImage` struct model this:

```cpp
struct DecodedImage {
  std::vector<uint16_t> pixels;
  int width;
  int height;
  int channels;
};

class IDngDecoder {
  public:
    virtual ~IDngDecoder() = default;
    virtual auto decode(const std::string& file_path)
      -> Nyx::Core::Result<DecodedImage> = 0;
};
```

The `pixels` vector stores 16-bit unsigned integer values, matching the typical dynamic range of astronomical CCD/CMOS sensors (12--16 bit). The implementation uses LibRaw, which handles Bayer pattern demosaicking and linear scaling. The `channels` field is 1 for monochrome images and 3 for colour images.

=== Aperture Photometry Interface <photometry_impl>

The `IPhotometryProcessor` interface defines two operations: source detection and aperture measurement, as described in @photometry_design:

```cpp
struct PhotometryResult {
  double raw_flux;
  double raw_flux_error;
};

struct DetectedSource {
  int x;
  int y;
  double peak_value;
};

class IPhotometryProcessor {
  public:
    virtual ~IPhotometryProcessor() = default;

    virtual auto detect_sources(
      const DecodedImage& image,
      double threshold_sigma
    ) -> std::vector<DetectedSource> = 0;

    virtual auto measure_aperture(
      const DecodedImage& image,
      int center_x, int center_y,
      double aperture_radius,
      double annulus_inner_radius,
      double annulus_outer_radius
    ) -> Nyx::Core::Result<PhotometryResult> = 0;
};
```

Source detection identifies bright pixels above a sigma threshold relative to the image background. Aperture measurement sums the pixel values within a circular aperture centred on the source, subtracts the sky background estimated from an annulus, and computes the photon noise error. The `raw_flux` value is converted to `relative_flux` by dividing by a reference star's flux, enabling comparison with normalised TESS light curves.

== Time System Conversion <time_conversion>

Comparing ground-based observations with TESS data requires converting between time systems, as designed in @time_systems. User observations record timestamps in ISO 8601 (UTC); TESS light curves use Barycentric TESS Julian Date (BTJD), which is the Barycentric Julian Date @eastman2010 minus a constant offset of 2,457,000.0 @ricker2015.

The `TimeConversion` utility class provides the conversion:

```cpp
class TimeConversion {
  public:
    static auto iso8601_to_jd(
      const std::string& iso_timestamp
    ) -> std::optional<double> {
      auto year = 0;
      auto month = 0;
      auto day = 0;
      auto hour = 0;
      auto minute = 0;
      auto second = 0.0;

      auto count = std::sscanf(
        iso_timestamp.c_str(),
        "%d-%d-%dT%d:%d:%lf",
        &year, &month, &day,
        &hour, &minute, &second
      );

      // Fallback format: "YYYY-MM-DD HH:MM:SS"
      if (count < 6) {
        count = std::sscanf(
          iso_timestamp.c_str(),
          "%d-%d-%d %d:%d:%lf",
          &year, &month, &day,
          &hour, &minute, &second
        );
      }

      if (count < 3) return std::nullopt;

      auto a_year = year;
      auto a_month = month;
      if (a_month <= 2) {
        a_year -= 1;
        a_month += 12;
      }

      auto a = a_year / 100;
      auto b = 2 - a + a / 4;

      auto jd = std::floor(365.25 * (a_year + 4716))
        + std::floor(30.6001 * (a_month + 1))
        + day + b - 1524.5;

      jd += (hour + minute / 60.0 + second / 3600.0) / 24.0;
      return jd;
    }

    static constexpr double btjd_offset = 2457000.0;
};
```

The algorithm implements the standard Gregorian calendar to Julian Date formula @hessman2004. The January/February adjustment (subtracting 1 from the year and adding 12 to the month for January and February) handles the historical quirk that the Julian calendar year originally began in March.

The `LightCurveComparisonService` uses this conversion to place ground-based observations on the same time axis as TESS data:

```cpp
// Convert TESS BTJD to full JD for comparison
for (auto& point : tess_points) {
  point.time += Nyx::Core::TimeConversion::btjd_offset;
}

// Convert user observations from ISO 8601 to JD
auto jd = Nyx::Core::TimeConversion::iso8601_to_jd(
  image.captured_at.value()
);
```

The comparison response includes a note acknowledging the timing limitation: ground-based timestamps are converted from UTC to JD without barycentric correction, introducing a maximum error of approximately 8 minutes (the light-travel time across Earth's orbit). This is acceptable for visual overlay comparison but would need correction for precise timing analysis.

== Database Access Layer <database_access>

All 13 repository implementations follow the same pattern: synchronous Drogon ORM queries with parameterised SQL @postgresql2024, error handling via `try/catch` on `DrogonDbException`, and mapping results to domain entities. This section illustrates the pattern with representative examples.

=== Parameterised Queries for SQL Injection Prevention

Every query uses positional placeholders (`$1`, `$2`, ...) rather than string interpolation, eliminating SQL injection risk @owasp2024:

```cpp
auto result = db->execSqlSync(
  "SELECT id, canonical_name, target_type, "
  "right_ascension, declination "
  "FROM targets WHERE canonical_name = $1",
  name
);
```

Drogon's `execSqlSync` binds the `name` parameter to `$1` as a prepared statement parameter. The database driver handles escaping and type conversion.

=== Nullable Column Mapping

Database columns that may be `NULL` are mapped to `std::optional` using Drogon's `isNull()` check:

```cpp
for (const auto& row : result) {
  points.push_back(Nyx::Domain::LightCurvePoint{
    .id = 0,
    .tess_observation_id = {},
    .time = row[0].as<double>(),
    .pdcsap_flux = row[1].isNull()
      ? std::nullopt
      : std::optional<float>(row[1].as<float>()),
    .pdcsap_flux_err = row[2].isNull()
      ? std::nullopt
      : std::optional<float>(row[2].as<float>()),
    .sap_flux = row[3].isNull()
      ? std::nullopt
      : std::optional<float>(row[3].as<float>()),
    .quality = row[4].as<int>(),
  });
}
```

Columns are accessed by index rather than by name for performance: index-based access avoids the hash lookup that name-based access requires on each row. For a 18,000-row result set, this eliminates 90,000 hash lookups (5 columns per row).

=== Transaction Usage in Bulk Operations

Bulk insert operations wrap all batches in a single database transaction to ensure atomicity:

```cpp
auto db = drogon::app().getDbClient();
auto transaction = db->newTransaction();
// ... multiple batch inserts ...
// Transaction auto-commits on scope exit if no error
```

If any batch fails (e.g. due to a constraint violation), the `DrogonDbException` handler catches the error and the transaction is rolled back automatically when the `transaction` shared pointer goes out of scope. This guarantees that partial inserts never occur.

=== Light Curve Point Indexing

The `light_curve_points` table includes two indexes to support the two primary access patterns:

```sql
CREATE INDEX idx_lcp_observation_id
  ON light_curve_points(tess_observation_id);
CREATE INDEX idx_lcp_observation_time
  ON light_curve_points(tess_observation_id, time);
```

The single-column index on `tess_observation_id` supports the `count_by_observation_id` query (used for the caching check in `fetch_light_curve`). The composite index on `(tess_observation_id, time)` supports the `find_by_observation_id` query with `ORDER BY time ASC`, allowing the database to satisfy the query entirely from the index @winand2012.

== Response Formatting <response_formatting>

All API responses use a consistent JSON envelope as specified in @api_design. The `ResponseHelper` class provides static methods for constructing success and error responses:

```cpp
static auto success(
  const nlohmann::json& data,
  const std::string& correlation_id,
  drogon::HttpStatusCode status_code = drogon::k200OK
) -> drogon::HttpResponsePtr {
  auto meta = nlohmann::json{
    {"request_id", correlation_id},
    {"timestamp",
      trantor::Date::now().toFormattedString(false)}
  };
  auto response_body = nlohmann::json{
    {"data", data}, {"meta", meta}
  };
  return make_json_response(response_body, status_code);
}

static auto error(
  const Nyx::Core::AppError& application_error,
  const std::string& correlation_id
) -> drogon::HttpResponsePtr {
  auto error_body = nlohmann::json{
    {"code", application_error.error_code_string()},
    {"message", application_error.message}
  };
  if (!application_error.details.empty()) {
    auto details_array = nlohmann::json::array();
    for (const auto& detail : application_error.details) {
      details_array.push_back(nlohmann::json{
        {"field", detail.field},
        {"message", detail.message}
      });
    }
    error_body["details"] = details_array;
  }
  auto meta = nlohmann::json{
    {"request_id", correlation_id},
    {"timestamp",
      trantor::Date::now().toFormattedString(false)}
  };
  auto response_body = nlohmann::json{
    {"error", error_body}, {"meta", meta}
  };
  return make_json_response(response_body,
    static_cast<drogon::HttpStatusCode>(
      application_error.http_status_code()
    )
  );
}
```

The `meta` object always includes the `request_id` (correlation ID) and a server-generated `timestamp`. This ensures that every response can be traced back to the corresponding log entries. The `error` method maps the `AppError`'s `ErrorCode` to both an HTTP status code (for the response status line) and a machine-readable string (for the response body). Field-level validation details are included when present, allowing the frontend to display per-field error messages.

Cookie management for refresh tokens and CSRF tokens is also handled by `ResponseHelper`:

```cpp
static auto set_refresh_token_cookie(
  drogon::HttpResponsePtr& response,
  const std::string& refresh_token,
  int max_age_seconds,
  bool secure
) -> void {
  auto cookie = drogon::Cookie("refresh_token", refresh_token);
  cookie.setHttpOnly(true);
  cookie.setSecure(secure);
  cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
  cookie.setPath("/api/v1/auth");
  cookie.setMaxAge(max_age_seconds);
  response->addCookie(cookie);
}
```

The refresh token cookie is `HttpOnly` (inaccessible to JavaScript), `Secure` (HTTPS-only in production), `SameSite=Strict` (not sent on cross-origin requests), and scoped to `/api/v1/auth` (only sent on auth endpoints). The CSRF token cookie is intentionally _not_ `HttpOnly` because the frontend JavaScript must read it and include it as a header.

== Frontend Implementation <frontend_impl>

The frontend is a Next.js @nextjs2024 application using the App Router with TypeScript. It communicates with the backend exclusively through the REST API.

=== API Client with Automatic Token Refresh <api_client>

The API client wraps the browser's `fetch` API with automatic Bearer token attachment, CSRF token inclusion, and transparent 401 retry:

```typescript
async function fetchWithAuth(url: string, options: RequestInit) {
  let response = await fetch(url, {
    ...options,
    headers: {
      ...options.headers,
      'Authorization': `Bearer ${getAccessToken()}`,
      'X-CSRF-Token': getCsrfToken(),
    },
    credentials: 'include',
  });

  if (response.status === 401) {
    const refreshed = await refreshToken();
    if (refreshed) {
      response = await fetch(url, {
        ...options,
        headers: {
          ...options.headers,
          'Authorization': `Bearer ${getAccessToken()}`,
          'X-CSRF-Token': getCsrfToken(),
        },
        credentials: 'include',
      });
    }
  }
  return response;
}
```

The `credentials: 'include'` setting ensures that the `refresh_token` HttpOnly cookie is sent with requests to the auth endpoints. When a 401 response is received, the client calls `refreshToken()`, which POSTs to `/api/v1/auth/refresh`. If the refresh succeeds, the new access token is stored and the original request is retried transparently. If the refresh fails (e.g. due to token revocation), the user is redirected to the login page.

=== Light Curve Viewer <light_curve_viewer>

The light curve viewer renders TESS time-series data using Plotly.js @plotly2024 with WebGL acceleration for smooth interaction with large datasets. The chart displays Barycentric TESS Julian Date (BTJD) on the x-axis and PDCSAP flux @stumpe2012 on the y-axis. Error bars from `PDCSAP_FLUX_ERR` provide visual indication of measurement uncertainty. Users can toggle between SAP and PDCSAP flux, zoom into transit features, and overlay their own ground-based observation points.

=== SSE Progress Updates for Long-Running Operations <sse_progress>

Downloading and parsing a FITS file can take 10--30 seconds. To keep the user informed, the frontend uses the Server-Sent Events (SSE) API @w3c_sse2015 to receive real-time progress updates:

```typescript
const source = new EventSource(
  `/api/v1/targets/observations/${id}/light-curve/progress`
);
source.onmessage = (event) => {
  const data = JSON.parse(event.data);
  setProgress(data.percent);
  setStatus(data.message);
};
source.onerror = () => source.close();
```

The backend sends events in the standard SSE format (`data: {...}\n\n`), with `percent` (0--100) and `message` (human-readable status) fields. The SSE connection is opened before the fetch operation begins and closed automatically when the operation completes or fails. This provides a responsive user experience without polling.

=== Light Curve Comparison View <comparison_view>

The comparison endpoint retrieves TESS light curve data alongside the user's ground-based photometry results for the same target, converting all timestamps to Julian Date for a unified time axis. The `LightCurveComparisonService` implements this:

```cpp
auto LightCurveComparisonService::get_comparison(
  const std::string& user_id,
  const std::string& target_id,
  const std::string& tess_observation_id,
  bool quality_filter,
  std::shared_ptr<spdlog::logger> logger
) -> Nyx::Core::Result<LightCurveComparisonResponse> {
  // Fetch target, TESS observation, and light curve points
  // ...

  // Convert BTJD to full JD
  for (auto& point : tess_points) {
    point.time += Nyx::Core::TimeConversion::btjd_offset;
  }

  // Fetch user's observation sessions for this target
  auto sessions_result =
    this->session_repository->find_by_user_id_and_target_id(
      user_id, target_id
    );
  // ...

  // Convert each image's captured_at to JD
  for (const auto& image : images_result.value()) {
    if (!image.photometry_status.has_value()
        || image.photometry_status.value() != "completed") {
      continue;
    }

    auto jd = Nyx::Core::TimeConversion::iso8601_to_jd(
      image.captured_at.value()
    );
    // ...
    user_points.push_back(UserObservationPoint{
      .time = jd.value(),
      .relative_flux = image.relative_flux.value(),
      .relative_flux_error = image.relative_flux_error.value_or(0.0),
      .session_id = session.id,
      .captured_at = image.captured_at.value(),
    });
  }

  // Sort user points chronologically
  std::sort(user_points.begin(), user_points.end(),
    [](const auto& a, const auto& b) {
      return a.time < b.time;
    }
  );

  return LightCurveComparisonResponse{
    .target = std::move(target_response),
    .time_system = "bjd_tdb",
    .time_system_note =
      "TESS times converted from BTJD (+2457000). "
      "User times converted from UTC to JD "
      "(barycentric correction not applied, max error ~8 min).",
    .tess = TessComparisonData{ /* ... */ },
    .user_observations = UserObservationData{ /* ... */ },
  };
}
```

The response includes a `time_system_note` that documents the conversion method and its limitations, ensuring transparency about the approximately 8-minute timing uncertainty in the user observation timestamps due to the omitted barycentric correction.

== Unit Testing <unit_testing>

The test suite uses Google Test @googletest2024 with Google Mock for mock objects. The `TargetServiceTest` class demonstrates the testing approach: all six dependencies of `TargetService` are replaced with mocks:

```cpp
class TargetServiceTest : public ::testing::Test {
  protected:
    auto SetUp() -> void override {
      this->mast_client = std::make_shared<MockMastClient>();
      this->target_repo = std::make_shared<MockTargetRepository>();
      this->tess_obs_repo =
        std::make_shared<MockTessObservationRepository>();
      this->uuid_gen = std::make_shared<MockUuidGenerator>();
      this->lcp_repo =
        std::make_shared<MockLightCurvePointRepository>();
      this->fits_parser = std::make_shared<MockFitsParser>();
      this->logger = spdlog::default_logger();

      this->service = std::make_unique<TargetService>(
        this->mast_client, this->target_repo,
        this->tess_obs_repo, this->uuid_gen,
        this->lcp_repo, this->fits_parser
      );
    }
    // ... member declarations ...
};
```

Mock classes are generated using the `MOCK_METHOD` macro. For example, the `MockMastClient` mocks all four `IMastClient` methods:

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
    MOCK_METHOD(
      (Nyx::Core::Result<std::vector<DataProduct>>),
      get_data_products,
      (const std::string& obsid), (override)
    );
    MOCK_METHOD(
      Nyx::Core::Result<std::string>, download_fits,
      (const std::string& data_uri), (override)
    );
};
```

A representative test case verifies the full `resolve_target` pipeline for a new target with TESS data:

```cpp
TEST_F(TargetServiceTest, ResolveNewTargetWithTessData) {
  auto request = ResolveTargetRequest{.target_name = "Pi Mensae"};

  auto resolved = ResolvedTarget{
    .canonical_name = "pi Men",
    .target_type = std::optional<std::string>("Star"),
    .right_ascension = std::optional<double>(84.291),
    .declination = std::optional<double>(-80.469),
  };

  EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
    .WillOnce(::testing::Return(resolved));
  EXPECT_CALL(*this->target_repo,
    find_by_canonical_name("pi Men"))
    .WillOnce(::testing::Return(
      std::optional<Nyx::Domain::Target>(std::nullopt)
    ));
  EXPECT_CALL(*this->uuid_gen, generate())
    .WillOnce(::testing::Return("t-1"))
    .WillOnce(::testing::Return("to-1"))
    .WillOnce(::testing::Return("to-2"));
  // ... additional EXPECT_CALL for create, search, bulk_create ...

  auto result = this->service->resolve_target(request, this->logger);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->id, "t-1");
  EXPECT_EQ(result->canonical_name, "pi Men");
  EXPECT_EQ(result->tess_observations.size(), 2);
  EXPECT_EQ(result->tess_observations[0].obsid, "obs-1");
  EXPECT_EQ(result->tess_observations[0].cadence_seconds, 120);
}
```

This test exercises the full pipeline: name resolution via MAST, cache miss, target creation, TESS observation search, multi-sector filtering, duplicate detection, and bulk insert. By mocking all external dependencies, the test runs in milliseconds without network access or a database connection, validating the orchestration logic in isolation.
