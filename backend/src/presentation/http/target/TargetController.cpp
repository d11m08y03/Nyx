#include "presentation/http/target/TargetController.hpp"
#include "presentation/http/target/TargetSchemas.hpp"
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

namespace Nyx::Presentation::Http::Target {
  TargetController::TargetController() {
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

    spdlog::debug("TargetController initialized");
  }

  static auto target_response_to_json(
    const Nyx::Application::Target::TargetResponse& target
  ) -> nlohmann::json {
    auto observations_array = nlohmann::json::array();
    for (const auto& obs : target.tess_observations) {
      observations_array.push_back(nlohmann::json{
        {"id", obs.id},
        {"obsid", obs.obsid},
        {"cadence_seconds", obs.cadence_seconds},
        {"start_time", obs.start_time},
        {"end_time", obs.end_time},
        {"data_uri", obs.data_uri.has_value()
          ? nlohmann::json(obs.data_uri.value())
          : nlohmann::json(nullptr)},
      });
    }

    return nlohmann::json{
      {"id", target.id},
      {"canonical_name", target.canonical_name},
      {"target_type", target.target_type.has_value()
        ? nlohmann::json(target.target_type.value())
        : nlohmann::json(nullptr)},
      {"right_ascension", target.right_ascension.has_value()
        ? nlohmann::json(target.right_ascension.value())
        : nlohmann::json(nullptr)},
      {"declination", target.declination.has_value()
        ? nlohmann::json(target.declination.value())
        : nlohmann::json(nullptr)},
      {"tess_observations", observations_array},
    };
  }

  auto TargetController::resolve(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>(
        "logger"
      );

    if (!logger) logger = spdlog::default_logger();

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (const nlohmann::json::parse_error& exception) {
      logger->warn(
        "Invalid JSON in resolve target request: {}",
        exception.what()
      );
      callback(ResponseHelper::error(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::InvalidJson,
          "Request body must be valid JSON", {}
        },
        correlation_id
      ));
      return;
    }

    auto validated = Nyx::Core::RequestValidator::validate(
      request_body, resolve_target_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto resolve_request =
      Nyx::Application::Target::ResolveTargetRequest{
        .target_name =
          request_body["target_name"].get<std::string>(),
      };

    auto result = this->target_service->resolve_target(
      resolve_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::created(
      target_response_to_json(result.value()), correlation_id
    ));
  }

  auto TargetController::get_by_id(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>(
        "logger"
      );

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

    auto result = this->target_service->get_target(id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::success(
      target_response_to_json(result.value()), correlation_id
    ));
  }

  auto TargetController::list_tess_observations(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>(
        "logger"
      );

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

    auto result = this->target_service->list_tess_observations(
      id, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto response_array = nlohmann::json::array();
    for (const auto& obs : result.value()) {
      response_array.push_back(nlohmann::json{
        {"id", obs.id},
        {"obsid", obs.obsid},
        {"cadence_seconds", obs.cadence_seconds},
        {"start_time", obs.start_time},
        {"end_time", obs.end_time},
        {"data_uri", obs.data_uri.has_value()
          ? nlohmann::json(obs.data_uri.value())
          : nlohmann::json(nullptr)},
      });
    }

    callback(ResponseHelper::success(
      response_array, correlation_id
    ));
  }
} // namespace Nyx::Presentation::Http::Target
