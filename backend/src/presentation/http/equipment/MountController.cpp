#include "presentation/http/equipment/MountController.hpp"
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
  MountController::MountController() {
    this->equipment_service = std::make_shared<
      Nyx::Application::Equipment::EquipmentService
    >(
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresTelescopeRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresCameraRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresMountRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresFilterRepository>(),
      std::make_shared<Nyx::Infrastructure::Util::DrogonUuidGenerator>()
    );

    spdlog::debug("MountController initialized");
  }

  auto MountController::create(
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
      logger->warn("Invalid JSON in create mount request: {}", exception.what());
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
      request_body, create_mount_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto create_request = Nyx::Application::Equipment::CreateMountRequest{
      .name = request_body["name"].get<std::string>(),
      .mount_type = request_body["mount_type"].get<std::string>(),
      .is_tracked = request_body.value("is_tracked", false),
      .has_goto = request_body.value("has_goto", false),
      .brand = request_body.value("brand", ""),
      .model = request_body.value("model", ""),
    };

    auto result = this->equipment_service->create_mount(
      user_id, create_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"mount_type", result->mount_type},
      {"is_tracked", result->is_tracked}, {"has_goto", result->has_goto},
      {"brand", result->brand}, {"model", result->model},
    };

    callback(ResponseHelper::created(response_data, correlation_id));
  }

  auto MountController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    auto result = this->equipment_service->list_mounts(user_id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_array = nlohmann::json::array();
    for (const auto& mount : result.value()) {
      response_array.push_back(nlohmann::json{
        {"id", mount.id}, {"user_id", mount.user_id},
        {"name", mount.name}, {"mount_type", mount.mount_type},
        {"is_tracked", mount.is_tracked}, {"has_goto", mount.has_goto},
        {"brand", mount.brand}, {"model", mount.model},
      });
    }

    callback(ResponseHelper::success(response_array, correlation_id));
  }

  auto MountController::get_by_id(
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

    auto result = this->equipment_service->get_mount(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"mount_type", result->mount_type},
      {"is_tracked", result->is_tracked}, {"has_goto", result->has_goto},
      {"brand", result->brand}, {"model", result->model},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto MountController::update(
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
      logger->warn("Invalid JSON in update mount request: {}", exception.what());
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
      request_body, update_mount_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto update_request = Nyx::Application::Equipment::UpdateMountRequest{
      .name = request_body["name"].get<std::string>(),
      .mount_type = request_body["mount_type"].get<std::string>(),
      .is_tracked = request_body.value("is_tracked", false),
      .has_goto = request_body.value("has_goto", false),
      .brand = request_body.value("brand", ""),
      .model = request_body.value("model", ""),
    };

    auto result = this->equipment_service->update_mount(
      user_id, id, update_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"mount_type", result->mount_type},
      {"is_tracked", result->is_tracked}, {"has_goto", result->has_goto},
      {"brand", result->brand}, {"model", result->model},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto MountController::remove(
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

    auto result = this->equipment_service->delete_mount(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    callback(ResponseHelper::no_content());
  }
} // namespace Nyx::Presentation::Http::Equipment
