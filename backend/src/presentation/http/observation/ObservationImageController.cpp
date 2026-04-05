#include "presentation/http/observation/ObservationImageController.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/imaging/Exiv2ExifParser.hpp"
#include "infrastructure/persistence/PostgresObservationImageRepository.hpp"
#include "infrastructure/persistence/PostgresObservationSessionRepository.hpp"
#include "infrastructure/persistence/PostgresTargetRepository.hpp"
#include "infrastructure/storage/LocalFileStorage.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <drogon/MultiPart.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Observation {
  static auto image_response_to_json(
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

  ObservationImageController::ObservationImageController() {
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
      "ObservationImageController initialized"
    );
  }

  auto ObservationImageController::upload(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback,
    const std::string& session_id
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

    if (!Nyx::Core::RequestValidator::is_valid_uuid(
          session_id
        )) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"session_id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto parser = drogon::MultiPartParser{};
    if (parser.parse(request) != 0) {
      logger->warn(
        "Failed to parse multipart request for "
        "session_id={}",
        session_id
      );
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid multipart request",
          {{"file",
            "Request must be multipart/form-data "
            "with a file"}}
        ),
        correlation_id
      ));
      return;
    }

    auto& files = parser.getFiles();
    if (files.empty()) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "No file uploaded",
          {{"file",
            "At least one file must be provided"}}
        ),
        correlation_id
      ));
      return;
    }

    auto uploaded_images = nlohmann::json::array();
    for (const auto& file : files) {
      auto file_data = Nyx::Application::Observation::UploadedFileData{
        .original_filename = file.getFileName(),
        .data = file.fileContent().data(),
        .length = file.fileLength(),
        .mime_type = {},
      };

      auto result = this->observation_service->upload_image(
        user_id, session_id, file_data, logger
      );
      if (!result.has_value()) {
        callback(ResponseHelper::error(
          result.error(), correlation_id
        ));
        return;
      }

      uploaded_images.push_back(
        image_response_to_json(result.value())
      );
    }

    callback(ResponseHelper::created(
      uploaded_images, correlation_id
    ));
  }

  auto ObservationImageController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback,
    const std::string& session_id,
    const std::string& image_id
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

    if (!Nyx::Core::RequestValidator::is_valid_uuid(
          session_id
        )
        || !Nyx::Core::RequestValidator::is_valid_uuid(
             image_id
           )) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto result = this->observation_service->delete_image(
      user_id, session_id, image_id, logger
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
