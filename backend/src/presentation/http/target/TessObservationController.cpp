#include "presentation/http/target/TessObservationController.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/persistence/PostgresTargetRepository.hpp"
#include "infrastructure/persistence/PostgresTessObservationRepository.hpp"
#include "infrastructure/persistence/PostgresLightCurvePointRepository.hpp"
#include "infrastructure/nasa/MastClient.hpp"
#include "infrastructure/nasa/FitsParser.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <trantor/utils/Date.h>

namespace Nyx::Presentation::Http::Target {
  TessObservationController::TessObservationController() {
    auto config =
      Nyx::Infrastructure::Config::EnvironmentConfig{};

    auto mast_client = std::make_shared<
      Nyx::Infrastructure::Nasa::MastClient
    >(config.nasa_mast_base_url());

    auto target_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresTargetRepository
    >();

    auto tess_observation_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresTessObservationRepository
    >();

    auto uuid_generator = std::make_shared<
      Nyx::Infrastructure::Util::DrogonUuidGenerator
    >();

    auto light_curve_point_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresLightCurvePointRepository
    >();

    auto fits_parser = std::make_shared<
      Nyx::Infrastructure::Nasa::FitsParser
    >();

    this->target_service = std::make_shared<
      Nyx::Application::Target::TargetService
    >(
      mast_client, target_repository,
      tess_observation_repository, uuid_generator,
      light_curve_point_repository, fits_parser
    );

    spdlog::debug("TessObservationController initialized");
  }

  auto TessObservationController::discover_products(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()->get<
        std::shared_ptr<spdlog::logger>
      >("logger");

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(id)) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto result = this->target_service->discover_products(
      id, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto response_json = nlohmann::json{
      {"id", result->id},
      {"obsid", result->obsid},
      {"cadence_seconds", result->cadence_seconds},
      {"start_time", result->start_time},
      {"end_time", result->end_time},
      {"data_uri", result->data_uri.has_value()
        ? nlohmann::json(result->data_uri.value())
        : nlohmann::json(nullptr)},
    };

    callback(ResponseHelper::success(
      response_json, correlation_id
    ));
  }

  auto TessObservationController::fetch_light_curve(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()->get<
        std::shared_ptr<spdlog::logger>
      >("logger");

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(id)) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto result = this->target_service->fetch_light_curve(
      id, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto response_json = nlohmann::json{
      {"tess_observation_id", result->tess_observation_id},
      {"obsid", result->obsid},
      {"point_count", result->point_count},
      {"time_min", result->time_min},
      {"time_max", result->time_max},
    };

    callback(ResponseHelper::success(
      response_json, correlation_id
    ));
  }

  auto TessObservationController::get_light_curve(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()->get<
        std::shared_ptr<spdlog::logger>
      >("logger");

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(id)) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto quality_filter_param =
      request->getParameter("quality_filter");
    auto quality_filter = (quality_filter_param == "true"
      || quality_filter_param == "1");

    auto result = this->target_service->get_light_curve_json(
      id, quality_filter, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

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
  }
} // namespace Nyx::Presentation::Http::Target
