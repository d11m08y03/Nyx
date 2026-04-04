#include "presentation/http/location/LocationController.hpp"
#include "presentation/http/location/LocationSchemas.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/persistence/PostgresObservingLocationRepository.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Location {
  LocationController::LocationController() {
    auto location_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresObservingLocationRepository
    >();

    auto uuid_generator = std::make_shared<
      Nyx::Infrastructure::Util::DrogonUuidGenerator
    >();

    this->location_service = std::make_shared<
      Nyx::Application::Location::LocationService
    >(location_repository, uuid_generator);

    spdlog::debug("LocationController initialized");
  }

  auto LocationController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (const nlohmann::json::parse_error& exception) {
      logger->warn(
        "Invalid JSON in create location request: {}", exception.what()
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
      request_body, create_location_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto bortle_class = std::optional<int16_t>{};
    if (request_body.contains("bortle_class")) {
      bortle_class = request_body["bortle_class"].get<int16_t>();
    }

    auto create_request = Nyx::Application::Location::CreateLocationRequest{
      .name = request_body["name"].get<std::string>(),
      .latitude = request_body["latitude"].get<double>(),
      .longitude = request_body["longitude"].get<double>(),
      .bortle_class = bortle_class,
    };

    auto result = this->location_service->create_location(
      user_id, create_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name},
      {"latitude", result->latitude}, {"longitude", result->longitude},
      {"bortle_class", result->bortle_class.has_value()
        ? nlohmann::json(result->bortle_class.value())
        : nlohmann::json(nullptr)},
    };

    callback(ResponseHelper::created(response_data, correlation_id));
  }

  auto LocationController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    auto result = this->location_service->list_locations(user_id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_array = nlohmann::json::array();
    for (const auto& location : result.value()) {
      response_array.push_back(nlohmann::json{
        {"id", location.id}, {"user_id", location.user_id},
        {"name", location.name},
        {"latitude", location.latitude}, {"longitude", location.longitude},
        {"bortle_class", location.bortle_class.has_value()
          ? nlohmann::json(location.bortle_class.value())
          : nlohmann::json(nullptr)},
      });
    }

    callback(ResponseHelper::success(response_array, correlation_id));
  }

  auto LocationController::get_by_id(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(id)) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format", {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto result = this->location_service->get_location(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name},
      {"latitude", result->latitude}, {"longitude", result->longitude},
      {"bortle_class", result->bortle_class.has_value()
        ? nlohmann::json(result->bortle_class.value())
        : nlohmann::json(nullptr)},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto LocationController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(id)) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format", {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (const nlohmann::json::parse_error& exception) {
      logger->warn(
        "Invalid JSON in update location request: {}", exception.what()
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
      request_body, update_location_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto bortle_class = std::optional<int16_t>{};
    if (request_body.contains("bortle_class")) {
      bortle_class = request_body["bortle_class"].get<int16_t>();
    }

    auto update_request = Nyx::Application::Location::UpdateLocationRequest{
      .name = request_body["name"].get<std::string>(),
      .latitude = request_body["latitude"].get<double>(),
      .longitude = request_body["longitude"].get<double>(),
      .bortle_class = bortle_class,
    };

    auto result = this->location_service->update_location(
      user_id, id, update_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name},
      {"latitude", result->latitude}, {"longitude", result->longitude},
      {"bortle_class", result->bortle_class.has_value()
        ? nlohmann::json(result->bortle_class.value())
        : nlohmann::json(nullptr)},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto LocationController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(id)) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format", {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto result = this->location_service->delete_location(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    callback(ResponseHelper::no_content());
  }
} // namespace Nyx::Presentation::Http::Location
