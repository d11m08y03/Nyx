#include "presentation/http/equipment/CameraController.hpp"
#include "presentation/http/equipment/EquipmentSchemas.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/persistence/PostgresCameraRepository.hpp"
#include "infrastructure/persistence/PostgresFilterRepository.hpp"
#include "infrastructure/persistence/PostgresMountRepository.hpp"
#include "infrastructure/persistence/PostgresTelescopeRepository.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Equipment {
  CameraController::CameraController() {
    this->equipment_service = std::make_shared<
      Nyx::Application::Equipment::EquipmentService
    >(
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresTelescopeRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresCameraRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresMountRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresFilterRepository>(),
      std::make_shared<Nyx::Infrastructure::Util::DrogonUuidGenerator>()
    );

    spdlog::debug("CameraController initialized");
  }

  auto CameraController::create(
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
      logger->warn("Invalid JSON in create camera request: {}", exception.what());
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
      request_body, create_camera_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto create_request = Nyx::Application::Equipment::CreateCameraRequest{
      .name = request_body["name"].get<std::string>(),
      .sensor_type = request_body["sensor_type"].get<std::string>(),
      .brand = request_body.value("brand", ""),
      .model = request_body.value("model", ""),
      .pixel_size_um = request_body.value("pixel_size_um", 0.0),
      .resolution = request_body.value("resolution", ""),
    };

    auto result = this->equipment_service->create_camera(
      user_id, create_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"sensor_type", result->sensor_type},
      {"brand", result->brand}, {"model", result->model},
      {"pixel_size_um", result->pixel_size_um},
      {"resolution", result->resolution},
    };

    callback(ResponseHelper::created(response_data, correlation_id));
  }

  auto CameraController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    auto result = this->equipment_service->list_cameras(user_id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_array = nlohmann::json::array();
    for (const auto& camera : result.value()) {
      response_array.push_back(nlohmann::json{
        {"id", camera.id}, {"user_id", camera.user_id},
        {"name", camera.name}, {"sensor_type", camera.sensor_type},
        {"brand", camera.brand}, {"model", camera.model},
        {"pixel_size_um", camera.pixel_size_um},
        {"resolution", camera.resolution},
      });
    }

    callback(ResponseHelper::success(response_array, correlation_id));
  }

  auto CameraController::get_by_id(
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

    auto result = this->equipment_service->get_camera(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"sensor_type", result->sensor_type},
      {"brand", result->brand}, {"model", result->model},
      {"pixel_size_um", result->pixel_size_um},
      {"resolution", result->resolution},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto CameraController::update(
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
      logger->warn("Invalid JSON in update camera request: {}", exception.what());
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
      request_body, update_camera_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto update_request = Nyx::Application::Equipment::UpdateCameraRequest{
      .name = request_body["name"].get<std::string>(),
      .sensor_type = request_body["sensor_type"].get<std::string>(),
      .brand = request_body.value("brand", ""),
      .model = request_body.value("model", ""),
      .pixel_size_um = request_body.value("pixel_size_um", 0.0),
      .resolution = request_body.value("resolution", ""),
    };

    auto result = this->equipment_service->update_camera(
      user_id, id, update_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"sensor_type", result->sensor_type},
      {"brand", result->brand}, {"model", result->model},
      {"pixel_size_um", result->pixel_size_um},
      {"resolution", result->resolution},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto CameraController::remove(
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

    auto result = this->equipment_service->delete_camera(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    callback(ResponseHelper::no_content());
  }
} // namespace Nyx::Presentation::Http::Equipment
