#include "presentation/http/observation/ObservationSessionController.hpp"
#include "presentation/http/observation/ObservationSchemas.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/imaging/Exiv2ExifParser.hpp"
#include "infrastructure/persistence/PostgresObservationImageRepository.hpp"
#include "infrastructure/persistence/PostgresObservationSessionRepository.hpp"
#include "infrastructure/persistence/PostgresTargetRepository.hpp"
#include "infrastructure/storage/LocalFileStorage.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Observation {
  static auto image_to_json(
    const Nyx::Application::Observation::ImageResponse&
      image
  ) -> nlohmann::json {
    auto json = nlohmann::json{
      {"id", image.id},
      {"session_id", image.session_id},
      {"original_filename", image.original_filename},
      {"file_size_bytes", image.file_size_bytes},
      {"mime_type", image.mime_type},
      {"created_at", image.created_at},
    };
    json["captured_at"] = image.captured_at.has_value()
      ? nlohmann::json(image.captured_at.value())
      : nlohmann::json(nullptr);
    json["camera_model"] = image.camera_model.has_value()
      ? nlohmann::json(image.camera_model.value())
      : nlohmann::json(nullptr);
    json["exposure_time_seconds"] =
      image.exposure_time_seconds.has_value()
        ? nlohmann::json(
            image.exposure_time_seconds.value()
          )
        : nlohmann::json(nullptr);
    json["iso_speed"] = image.iso_speed.has_value()
      ? nlohmann::json(image.iso_speed.value())
      : nlohmann::json(nullptr);
    json["gps_latitude"] = image.gps_latitude.has_value()
      ? nlohmann::json(image.gps_latitude.value())
      : nlohmann::json(nullptr);
    json["gps_longitude"] =
      image.gps_longitude.has_value()
        ? nlohmann::json(image.gps_longitude.value())
        : nlohmann::json(nullptr);
    json["image_width"] = image.image_width.has_value()
      ? nlohmann::json(image.image_width.value())
      : nlohmann::json(nullptr);
    json["image_height"] = image.image_height.has_value()
      ? nlohmann::json(image.image_height.value())
      : nlohmann::json(nullptr);
    return json;
  }

  static auto session_to_json(
    const Nyx::Application::Observation::SessionResponse&
      session
  ) -> nlohmann::json {
    auto images_array = nlohmann::json::array();
    for (const auto& image : session.images) {
      images_array.push_back(image_to_json(image));
    }

    auto json = nlohmann::json{
      {"id", session.id},
      {"user_id", session.user_id},
      {"target_id", session.target_id},
      {"created_at", session.created_at},
      {"updated_at", session.updated_at},
      {"images", images_array},
    };
    json["notes"] = session.notes.has_value()
      ? nlohmann::json(session.notes.value())
      : nlohmann::json(nullptr);
    return json;
  }

  ObservationSessionController::ObservationSessionController() {
    auto upload_base_path = std::string(
      std::getenv("UPLOAD_PATH")
        ? std::getenv("UPLOAD_PATH")
        : "uploads"
    );

    this->observation_service = std::make_shared<
      Nyx::Application::Observation::ObservationService
    >(
      std::make_shared<
        Nyx::Infrastructure::Persistence::
          PostgresObservationSessionRepository
      >(),
      std::make_shared<
        Nyx::Infrastructure::Persistence::
          PostgresObservationImageRepository
      >(),
      std::make_shared<
        Nyx::Infrastructure::Persistence::
          PostgresTargetRepository
      >(),
      std::make_shared<
        Nyx::Infrastructure::Imaging::Exiv2ExifParser
      >(),
      std::make_shared<
        Nyx::Infrastructure::Storage::LocalFileStorage
      >(upload_base_path),
      std::make_shared<
        Nyx::Infrastructure::Util::DrogonUuidGenerator
      >()
    );

    spdlog::debug(
      "ObservationSessionController initialized"
    );
  }

  auto ObservationSessionController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id =
      request->getAttributes()->get<std::string>(
        "user_id"
      );

    if (!logger) logger = spdlog::default_logger();

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(
        request->body()
      );
    } catch (const nlohmann::json::parse_error& e) {
      logger->warn(
        "Invalid JSON in create session request: {}",
        e.what()
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
      request_body, create_session_schema
    );
    if (!validated.has_value()) {
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto create_request = Nyx::Application::Observation::CreateSessionRequest{
      .target_id =
        request_body["target_id"].get<std::string>(),
      .notes = request_body.contains("notes")
        ? std::optional<std::string>(
            request_body["notes"].get<std::string>()
          )
        : std::nullopt,
    };

    auto result = this->observation_service->create_session(
      user_id, create_request, logger
    );
    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::created(
      session_to_json(result.value()), correlation_id
    ));
  }

  auto ObservationSessionController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id =
      request->getAttributes()->get<std::string>(
        "user_id"
      );

    if (!logger) logger = spdlog::default_logger();

    auto result = this->observation_service->list_sessions(
      user_id, logger
    );
    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto response_array = nlohmann::json::array();
    for (const auto& session : result.value()) {
      response_array.push_back(
        session_to_json(session)
      );
    }

    callback(ResponseHelper::success(
      response_array, correlation_id
    ));
  }

  auto ObservationSessionController::get_by_id(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id =
      request->getAttributes()->get<std::string>(
        "user_id"
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

    auto result = this->observation_service->get_session(
      user_id, id, logger
    );
    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::success(
      session_to_json(result.value()), correlation_id
    ));
  }

  auto ObservationSessionController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id =
      request->getAttributes()->get<std::string>(
        "user_id"
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

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(
        request->body()
      );
    } catch (const nlohmann::json::parse_error& e) {
      logger->warn(
        "Invalid JSON in update session request: {}",
        e.what()
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
      request_body, update_session_schema
    );
    if (!validated.has_value()) {
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto update_request = Nyx::Application::Observation::UpdateSessionRequest{
      .notes = request_body.contains("notes")
        ? std::optional<std::string>(
            request_body["notes"].get<std::string>()
          )
        : std::nullopt,
    };

    auto result = this->observation_service->update_session(
      user_id, id, update_request, logger
    );
    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::success(
      session_to_json(result.value()), correlation_id
    ));
  }

  auto ObservationSessionController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback,
    const std::string& id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id =
      request->getAttributes()->get<std::string>(
        "user_id"
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

    auto result = this->observation_service->delete_session(
      user_id, id, logger
    );
    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::no_content());
  }
} // namespace Nyx::Presentation::Http::Observation
