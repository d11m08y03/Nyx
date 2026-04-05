#include "application/observation/ObservationService.hpp"

#include <algorithm>

namespace Nyx::Application::Observation {
  ObservationService::ObservationService(
    std::shared_ptr<Nyx::Domain::IObservationSessionRepository>
      session_repository,
    std::shared_ptr<Nyx::Domain::IObservationImageRepository>
      image_repository,
    std::shared_ptr<Nyx::Domain::ITargetRepository>
      target_repository,
    std::shared_ptr<IExifParser> exif_parser,
    std::shared_ptr<IFileStorage> file_storage,
    std::shared_ptr<Nyx::Core::IUuidGenerator>
      uuid_generator
  )
    : session_repository(std::move(session_repository))
    , image_repository(std::move(image_repository))
    , target_repository(std::move(target_repository))
    , exif_parser(std::move(exif_parser))
    , file_storage(std::move(file_storage))
    , uuid_generator(std::move(uuid_generator)) {
    spdlog::debug("ObservationService initialized");
  }

  auto ObservationService::verify_session_ownership(
    const std::string& session_id,
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<
    Nyx::Domain::ObservationSession
  > {
    auto result = this->session_repository->find_by_id(
      session_id
    );

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Observation session not found"
        )
      );
    }

    auto session = result->value();
    if (session.user_id != user_id) {
      logger->warn(
        "Ownership check failed: session {} belongs to "
        "user_id={}, requested by user_id={}",
        session_id, session.user_id, user_id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    return session;
  }

  auto ObservationService::to_image_response(
    const Nyx::Domain::ObservationImage& image
  ) -> ImageResponse {
    return ImageResponse{
      .id = image.id,
      .session_id = image.session_id,
      .original_filename = image.original_filename,
      .file_size_bytes = image.file_size_bytes,
      .mime_type = image.mime_type,
      .captured_at = image.captured_at,
      .camera_model = image.camera_model,
      .exposure_time_seconds =
        image.exposure_time_seconds,
      .iso_speed = image.iso_speed,
      .gps_latitude = image.gps_latitude,
      .gps_longitude = image.gps_longitude,
      .image_width = image.image_width,
      .image_height = image.image_height,
      .created_at = image.created_at,
    };
  }

  auto ObservationService::to_session_response(
    const Nyx::Domain::ObservationSession& session,
    const std::vector<Nyx::Domain::ObservationImage>&
      images
  ) -> SessionResponse {
    auto image_responses = std::vector<ImageResponse>{};
    image_responses.reserve(images.size());
    for (const auto& image : images) {
      image_responses.push_back(
        this->to_image_response(image)
      );
    }

    return SessionResponse{
      .id = session.id,
      .user_id = session.user_id,
      .target_id = session.target_id,
      .notes = session.notes,
      .created_at = session.created_at,
      .updated_at = session.updated_at,
      .images = std::move(image_responses),
    };
  }

  auto ObservationService::detect_mime_type(
    const std::string& filename
  ) -> std::string {
    auto lower = filename;
    std::transform(
      lower.begin(), lower.end(), lower.begin(),
      ::tolower
    );

    if (lower.ends_with(".jpg")
        || lower.ends_with(".jpeg")) {
      return "image/jpeg";
    }
    if (lower.ends_with(".png")) {
      return "image/png";
    }
    if (lower.ends_with(".tiff")
        || lower.ends_with(".tif")) {
      return "image/tiff";
    }
    if (lower.ends_with(".dng")) {
      return "image/x-adobe-dng";
    }
    return "application/octet-stream";
  }

  auto ObservationService::create_session(
    const std::string& user_id,
    const CreateSessionRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<SessionResponse> {
    logger->debug(
      "Creating observation session for user_id={}, "
      "target_id={}",
      user_id, request.target_id
    );

    auto target_result = this->target_repository->find_by_id(
      request.target_id
    );
    if (!target_result.has_value()) {
      return std::unexpected(target_result.error());
    }
    if (!target_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Target not found"
        )
      );
    }

    auto session = Nyx::Domain::ObservationSession{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .target_id = request.target_id,
      .notes = request.notes,
      .created_at = {},
      .updated_at = {},
    };

    auto result = this->session_repository->create(
      session
    );
    if (!result.has_value()) {
      logger->error(
        "Failed to create observation session for "
        "user_id={}",
        user_id
      );
      return std::unexpected(result.error());
    }

    auto created = result.value();
    logger->info(
      "Observation session created id={}, user_id={}",
      created.id, user_id
    );

    return this->to_session_response(created, {});
  }

  auto ObservationService::list_sessions(
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<
    std::vector<SessionResponse>
  > {
    logger->debug(
      "Listing observation sessions for user_id={}",
      user_id
    );

    auto result = this->session_repository->find_by_user_id(
      user_id
    );
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    auto responses = std::vector<SessionResponse>{};
    for (const auto& session : result.value()) {
      auto images_result =
        this->image_repository->find_by_session_id(
          session.id
        );
      auto images = images_result.has_value()
        ? images_result.value()
        : std::vector<Nyx::Domain::ObservationImage>{};

      responses.push_back(
        this->to_session_response(session, images)
      );
    }
    return responses;
  }

  auto ObservationService::get_session(
    const std::string& user_id,
    const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<SessionResponse> {
    logger->debug(
      "Getting observation session id={} for user_id={}",
      id, user_id
    );

    auto session_result = this->verify_session_ownership(
      id, user_id, logger
    );
    if (!session_result.has_value()) {
      return std::unexpected(session_result.error());
    }

    auto images_result =
      this->image_repository->find_by_session_id(id);
    auto images = images_result.has_value()
      ? images_result.value()
      : std::vector<Nyx::Domain::ObservationImage>{};

    return this->to_session_response(
      session_result.value(), images
    );
  }

  auto ObservationService::update_session(
    const std::string& user_id,
    const std::string& id,
    const UpdateSessionRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<SessionResponse> {
    logger->debug(
      "Updating observation session id={} for "
      "user_id={}",
      id, user_id
    );

    auto session_result = this->verify_session_ownership(
      id, user_id, logger
    );
    if (!session_result.has_value()) {
      return std::unexpected(session_result.error());
    }

    auto session = session_result.value();
    session.notes = request.notes;

    auto result = this->session_repository->update(
      session
    );
    if (!result.has_value()) {
      logger->error(
        "Failed to update observation session id={}", id
      );
      return std::unexpected(result.error());
    }

    auto images_result =
      this->image_repository->find_by_session_id(id);
    auto images = images_result.has_value()
      ? images_result.value()
      : std::vector<Nyx::Domain::ObservationImage>{};

    logger->info(
      "Observation session updated id={}, user_id={}",
      id, user_id
    );

    return this->to_session_response(
      result.value(), images
    );
  }

  auto ObservationService::delete_session(
    const std::string& user_id,
    const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug(
      "Deleting observation session id={} for "
      "user_id={}",
      id, user_id
    );

    auto session_result = this->verify_session_ownership(
      id, user_id, logger
    );
    if (!session_result.has_value()) {
      return std::unexpected(session_result.error());
    }

    auto result = this->session_repository->remove(id);
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    auto directory = user_id + "/" + id;
    auto remove_result = this->file_storage->remove_directory(
      directory
    );
    if (!remove_result.has_value()) {
      logger->warn(
        "Failed to remove files for session id={}: {}",
        id, remove_result.error().message
      );
    }

    logger->info(
      "Observation session deleted id={}, user_id={}",
      id, user_id
    );
    return {};
  }

  auto ObservationService::upload_image(
    const std::string& user_id,
    const std::string& session_id,
    const UploadedFileData& file_data,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<ImageResponse> {
    logger->debug(
      "Uploading image to session_id={} for user_id={}",
      session_id, user_id
    );

    auto session_result = this->verify_session_ownership(
      session_id, user_id, logger
    );
    if (!session_result.has_value()) {
      return std::unexpected(session_result.error());
    }

    auto mime_type = this->detect_mime_type(
      file_data.original_filename
    );

    constexpr auto max_file_size_bytes =
      static_cast<std::size_t>(50 * 1024 * 1024);
    if (file_data.length > max_file_size_bytes) {
      return std::unexpected(
        Nyx::Core::AppError::validation(
          "File too large",
          {{"file", "Maximum file size is 50MB"}}
        )
      );
    }

    auto allowed_types = std::vector<std::string>{
      "image/jpeg", "image/png", "image/tiff",
      "image/x-adobe-dng",
    };
    auto type_allowed = std::find(
      allowed_types.begin(), allowed_types.end(),
      mime_type
    ) != allowed_types.end();
    if (!type_allowed) {
      return std::unexpected(
        Nyx::Core::AppError::validation(
          "Unsupported file type",
          {{"file",
            "Allowed types: JPEG, PNG, TIFF, DNG"}}
        )
      );
    }

    auto image_id = this->uuid_generator->generate();
    auto extension = file_data.original_filename.substr(
      file_data.original_filename.find_last_of('.')
    );
    auto stored_filename = image_id + extension;
    auto directory = user_id + "/" + session_id;

    auto save_result = this->file_storage->save_file(
      directory, stored_filename,
      file_data.data, file_data.length
    );
    if (!save_result.has_value()) {
      return std::unexpected(save_result.error());
    }

    auto exif_result = this->exif_parser->parse(
      save_result.value()
    );
    auto exif_data = exif_result.has_value()
      ? exif_result.value()
      : ExifData{};

    auto image = Nyx::Domain::ObservationImage{
      .id = image_id,
      .session_id = session_id,
      .filename = stored_filename,
      .original_filename = file_data.original_filename,
      .file_path = save_result.value(),
      .file_size_bytes =
        static_cast<int64_t>(file_data.length),
      .mime_type = mime_type,
      .captured_at = exif_data.captured_at,
      .camera_model = exif_data.camera_model,
      .exposure_time_seconds =
        exif_data.exposure_time_seconds,
      .iso_speed = exif_data.iso_speed,
      .gps_latitude = exif_data.gps_latitude,
      .gps_longitude = exif_data.gps_longitude,
      .image_width = exif_data.image_width,
      .image_height = exif_data.image_height,
      .created_at = {},
    };

    auto create_result = this->image_repository->create(
      image
    );
    if (!create_result.has_value()) {
      (void)this->file_storage->remove_file(
        save_result.value()
      );
      return std::unexpected(create_result.error());
    }

    logger->info(
      "Image uploaded id={}, session_id={}, "
      "original_filename={}",
      image_id, session_id,
      file_data.original_filename
    );

    return this->to_image_response(
      create_result.value()
    );
  }

  auto ObservationService::delete_image(
    const std::string& user_id,
    const std::string& session_id,
    const std::string& image_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug(
      "Deleting image id={} from session_id={} for "
      "user_id={}",
      image_id, session_id, user_id
    );

    auto session_result = this->verify_session_ownership(
      session_id, user_id, logger
    );
    if (!session_result.has_value()) {
      return std::unexpected(session_result.error());
    }

    auto image_result = this->image_repository->find_by_id(
      image_id
    );
    if (!image_result.has_value()) {
      return std::unexpected(image_result.error());
    }
    if (!image_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Image not found"
        )
      );
    }

    auto image = image_result->value();
    if (image.session_id != session_id) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Image not found in this session"
        )
      );
    }

    auto remove_result = this->image_repository->remove(
      image_id
    );
    if (!remove_result.has_value()) {
      return std::unexpected(remove_result.error());
    }

    auto file_remove_result = this->file_storage->remove_file(
      image.file_path
    );
    if (!file_remove_result.has_value()) {
      logger->warn(
        "Failed to remove image file {}: {}",
        image.file_path,
        file_remove_result.error().message
      );
    }

    logger->info(
      "Image deleted id={}, session_id={}", image_id,
      session_id
    );
    return {};
  }
} // namespace Nyx::Application::Observation
