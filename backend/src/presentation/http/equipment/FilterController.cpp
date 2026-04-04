#include "presentation/http/equipment/FilterController.hpp"
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
  FilterController::FilterController() {
    this->equipment_service = std::make_shared<
      Nyx::Application::Equipment::EquipmentService
    >(
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresTelescopeRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresCameraRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresMountRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::PostgresFilterRepository>(),
      std::make_shared<Nyx::Infrastructure::Util::DrogonUuidGenerator>()
    );

    spdlog::debug("FilterController initialized");
  }

  auto FilterController::create(
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
      logger->warn("Invalid JSON in create filter request: {}", exception.what());
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
      request_body, create_filter_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto create_request = Nyx::Application::Equipment::CreateFilterRequest{
      .name = request_body["name"].get<std::string>(),
      .band = request_body["band"].get<std::string>(),
      .bandwidth_nm = request_body.value("bandwidth_nm", 0.0),
      .brand = request_body.value("brand", ""),
      .model = request_body.value("model", ""),
    };

    auto result = this->equipment_service->create_filter(
      user_id, create_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"band", result->band},
      {"bandwidth_nm", result->bandwidth_nm},
      {"brand", result->brand}, {"model", result->model},
    };

    callback(ResponseHelper::created(response_data, correlation_id));
  }

  auto FilterController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) logger = spdlog::default_logger();

    auto result = this->equipment_service->list_filters(user_id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_array = nlohmann::json::array();
    for (const auto& filter : result.value()) {
      response_array.push_back(nlohmann::json{
        {"id", filter.id}, {"user_id", filter.user_id},
        {"name", filter.name}, {"band", filter.band},
        {"bandwidth_nm", filter.bandwidth_nm},
        {"brand", filter.brand}, {"model", filter.model},
      });
    }

    callback(ResponseHelper::success(response_array, correlation_id));
  }

  auto FilterController::get_by_id(
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

    auto result = this->equipment_service->get_filter(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"band", result->band},
      {"bandwidth_nm", result->bandwidth_nm},
      {"brand", result->brand}, {"model", result->model},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto FilterController::update(
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
      logger->warn("Invalid JSON in update filter request: {}", exception.what());
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
      request_body, update_filter_schema
    );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto update_request = Nyx::Application::Equipment::UpdateFilterRequest{
      .name = request_body["name"].get<std::string>(),
      .band = request_body["band"].get<std::string>(),
      .bandwidth_nm = request_body.value("bandwidth_nm", 0.0),
      .brand = request_body.value("brand", ""),
      .model = request_body.value("model", ""),
    };

    auto result = this->equipment_service->update_filter(
      user_id, id, update_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id}, {"user_id", result->user_id},
      {"name", result->name}, {"band", result->band},
      {"bandwidth_nm", result->bandwidth_nm},
      {"brand", result->brand}, {"model", result->model},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }

  auto FilterController::remove(
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

    auto result = this->equipment_service->delete_filter(user_id, id, logger);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    callback(ResponseHelper::no_content());
  }
} // namespace Nyx::Presentation::Http::Equipment
