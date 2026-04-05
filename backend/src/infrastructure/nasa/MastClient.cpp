#include "infrastructure/nasa/MastClient.hpp"

#include <drogon/HttpClient.h>
#include <drogon/utils/Utilities.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Nasa {
  MastClient::MastClient(std::string base_url)
    : base_url(std::move(base_url)) {
    spdlog::debug("MastClient initialized with base_url={}", this->base_url);
  }

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

      auto body = std::string(response->body());

      spdlog::debug(
        "MAST API response: status={}, body={}",
        static_cast<int>(response->getStatusCode()), body
      );

      if (response->getStatusCode() != drogon::k200OK) {
        spdlog::error(
          "MAST API returned status {}",
          static_cast<int>(response->getStatusCode())
        );
        return std::unexpected(Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST API returned non-200 status", {}
        });
      }

      return body;
    } catch (const std::exception& exception) {
      spdlog::error("MAST API exception: {}", exception.what());
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST API request failed", {}
      });
    }
  }

  auto MastClient::resolve_target(
    const std::string& name
  ) -> Nyx::Core::Result<Nyx::Application::Target::ResolvedTarget> {
    spdlog::debug("Resolving target name='{}'", name);

    auto request_payload = nlohmann::json{
      {"service", "Mast.Name.Lookup"},
      {"params", {{"input", name}, {"format", "json"}}}
    };

    auto body_result = this->invoke_mast_service(request_payload.dump());
    if (!body_result.has_value()) {
      return std::unexpected(body_result.error());
    }

    try {
      auto response_json = nlohmann::json::parse(body_result.value());

      auto resolved = response_json.value("resolvedCoordinate",
        nlohmann::json::array()
      );

      if (resolved.empty()) {
        spdlog::warn("Target '{}' could not be resolved", name);
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

      spdlog::info(
        "Resolved target '{}' -> canonical='{}', ra={}, dec={}",
        name, target.canonical_name,
        target.right_ascension.value_or(0.0),
        target.declination.value_or(0.0)
      );

      return target;
    } catch (const nlohmann::json::exception& exception) {
      spdlog::error(
        "Failed to parse MAST name resolution response: {}",
        exception.what()
      );
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "Failed to parse MAST response", {}
      });
    }
  }

  auto MastClient::search_tess_timeseries(
    double ra, double dec, double radius
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Application::Target::MastObservation>
  > {
    spdlog::debug(
      "Searching TESS timeseries at ra={}, dec={}, radius={}",
      ra, dec, radius
    );

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

    auto body_result = this->invoke_mast_service(request_payload.dump());
    if (!body_result.has_value()) {
      return std::unexpected(body_result.error());
    }

    try {
      auto response_json = nlohmann::json::parse(body_result.value());

      auto tables = response_json.value("Tables", nlohmann::json::array());
      if (tables.empty()) {
        spdlog::info("No TESS observations found (no tables)");
        return std::vector<Nyx::Application::Target::MastObservation>{};
      }

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

      if (obsid_index < 0 || exptime_index < 0
        || tmin_index < 0 || tmax_index < 0) {
        if (rows.empty()) {
          spdlog::info("No TESS observations found");
          return std::vector<Nyx::Application::Target::MastObservation>{};
        }
        spdlog::error("MAST response missing expected columns");
        return std::unexpected(Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST response missing expected columns", {}
        });
      }

      auto observations =
        std::vector<Nyx::Application::Target::MastObservation>{};

      for (const auto& row : rows) {
        auto obsid_value = row[obsid_index];
        auto exptime_value = row[exptime_index];
        auto tmin_value = row[tmin_index];
        auto tmax_value = row[tmax_index];

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

      spdlog::info(
        "Found {} TESS observations", observations.size()
      );

      return observations;
    } catch (const nlohmann::json::exception& exception) {
      spdlog::error(
        "Failed to parse MAST TESS search response: {}",
        exception.what()
      );
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "Failed to parse MAST response", {}
      });
    }
  }
  auto MastClient::get_data_products(
    const std::string& obsid
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Application::Target::DataProduct>
  > {
    spdlog::debug("Getting data products for obsid='{}'", obsid);

    auto request_payload = nlohmann::json{
      {"service", "Mast.Caom.Products"},
      {"params", {{"obsid", obsid}, {"format", "json"}}}
    };

    auto body_result = this->invoke_mast_service(
      request_payload.dump()
    );
    if (!body_result.has_value()) {
      return std::unexpected(body_result.error());
    }

    try {
      auto response_json = nlohmann::json::parse(
        body_result.value()
      );

      auto tables = response_json.value(
        "Tables", nlohmann::json::array()
      );
      if (tables.empty()) {
        spdlog::info(
          "No data products found for obsid={}", obsid
        );
        return std::vector<
          Nyx::Application::Target::DataProduct
        >{};
      }

      auto table = tables[0];
      auto fields = table.value(
        "Fields", nlohmann::json::array()
      );
      auto rows = table.value(
        "Rows", nlohmann::json::array()
      );

      auto uri_index = -1;
      auto desc_index = -1;
      auto type_index = -1;
      auto filename_index = -1;

      for (auto i = 0;
        i < static_cast<int>(fields.size()); ++i) {
        auto field_name = fields[i].value("name", "");
        if (field_name == "dataURI") uri_index = i;
        else if (field_name == "description") desc_index = i;
        else if (field_name == "productType") type_index = i;
        else if (field_name == "productFilename") {
          filename_index = i;
        }
      }

      if (uri_index < 0 || filename_index < 0) {
        if (rows.empty()) {
          return std::vector<
            Nyx::Application::Target::DataProduct
          >{};
        }
        spdlog::error(
          "MAST products response missing expected columns"
        );
        return std::unexpected(Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST products response missing expected columns",
          {}
        });
      }

      auto products = std::vector<
        Nyx::Application::Target::DataProduct
      >{};

      for (const auto& row : rows) {
        auto product = Nyx::Application::Target::DataProduct{
          .data_uri = row[uri_index].is_string()
            ? row[uri_index].get<std::string>() : "",
          .description = (desc_index >= 0
            && row[desc_index].is_string())
            ? row[desc_index].get<std::string>() : "",
          .product_type = (type_index >= 0
            && row[type_index].is_string())
            ? row[type_index].get<std::string>() : "",
          .filename = row[filename_index].is_string()
            ? row[filename_index].get<std::string>() : "",
        };
        products.push_back(std::move(product));
      }

      spdlog::info(
        "Found {} data products for obsid={}",
        products.size(), obsid
      );

      return products;
    } catch (const nlohmann::json::exception& exception) {
      spdlog::error(
        "Failed to parse MAST products response: {}",
        exception.what()
      );
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "Failed to parse MAST response", {}
      });
    }
  }

  auto MastClient::download_fits(
    const std::string& data_uri
  ) -> Nyx::Core::Result<std::string> {
    spdlog::debug("Downloading FITS file: {}", data_uri);

    try {
      auto client = drogon::HttpClient::newHttpClient(
        "https://mast.stsci.edu"
      );
      client->setUserAgent("Nyx/1.0");

      auto download_path = std::string{"/api/v0.1/Download/file"};
      auto full_path = download_path + "?uri=" + data_uri;

      auto request = drogon::HttpRequest::newHttpRequest();
      request->setMethod(drogon::Get);
      request->setPath(full_path);

      spdlog::debug("MAST download request: GET {}", full_path);

      auto [result, response] = client->sendRequest(
        request, 120.0
      );

      if (result != drogon::ReqResult::Ok) {
        spdlog::error("MAST download request failed: network error");
        return std::unexpected(Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST download request failed", {}
        });
      }

      auto status_code = response->getStatusCode();
      spdlog::debug(
        "MAST download response: status={}",
        static_cast<int>(status_code)
      );

      if (status_code == drogon::k307TemporaryRedirect
        || status_code == drogon::k302Found
        || status_code == drogon::k301MovedPermanently) {
        auto location = std::string{
          response->getHeader("Location")
        };

        if (location.empty()) {
          spdlog::error("MAST redirect missing Location header");
          return std::unexpected(Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "MAST redirect missing Location header", {}
          });
        }

        spdlog::debug("Following redirect to: {}", location);

        auto scheme_end = location.find("://");
        if (scheme_end == std::string::npos) {
          spdlog::error("Invalid redirect URL: {}", location);
          return std::unexpected(Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "Invalid redirect URL", {}
          });
        }

        auto path_start = location.find('/', scheme_end + 3);
        auto redirect_host = (path_start != std::string::npos)
          ? location.substr(0, path_start) : location;
        auto redirect_path = (path_start != std::string::npos)
          ? location.substr(path_start) : "/";

        auto redirect_client =
          drogon::HttpClient::newHttpClient(redirect_host);
        redirect_client->setUserAgent("Nyx/1.0");

        auto redirect_request =
          drogon::HttpRequest::newHttpRequest();
        redirect_request->setMethod(drogon::Get);
        redirect_request->setPath(redirect_path);

        auto [redirect_result, redirect_response] =
          redirect_client->sendRequest(redirect_request, 120.0);

        if (redirect_result != drogon::ReqResult::Ok) {
          spdlog::error("FITS download failed after redirect");
          return std::unexpected(Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "FITS download failed after redirect", {}
          });
        }

        if (redirect_response->getStatusCode()
          != drogon::k200OK) {
          spdlog::error(
            "FITS download returned status {}",
            static_cast<int>(
              redirect_response->getStatusCode()
            )
          );
          return std::unexpected(Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "FITS download returned non-200 status", {}
          });
        }

        auto body = std::string(redirect_response->body());
        spdlog::info(
          "Downloaded FITS file: {} bytes", body.size()
        );
        return body;
      }

      if (status_code != drogon::k200OK) {
        spdlog::error(
          "MAST download returned status {}",
          static_cast<int>(status_code)
        );
        return std::unexpected(Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST download returned non-200 status", {}
        });
      }

      auto body = std::string(response->body());
      spdlog::info(
        "Downloaded FITS file: {} bytes", body.size()
      );
      return body;
    } catch (const std::exception& exception) {
      spdlog::error(
        "MAST download exception: {}", exception.what()
      );
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST download request failed", {}
      });
    }
  }
} // namespace Nyx::Infrastructure::Nasa
