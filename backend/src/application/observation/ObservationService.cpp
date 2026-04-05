#include "application/observation/ObservationService.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

namespace Nyx::Application::Observation {
  ObservationService::ObservationService(
    std::shared_ptr<Nyx::Domain::IObservationSessionRepository>
      session_repository,
    std::shared_ptr<Nyx::Domain::IObservationImageRepository>
      image_repository,
    std::shared_ptr<Nyx::Domain::ITargetRepository>
      target_repository,
    std::shared_ptr<Nyx::Domain::ITelescopeRepository>
      telescope_repository,
    std::shared_ptr<Nyx::Domain::ICameraRepository>
      camera_repository,
    std::shared_ptr<Nyx::Domain::IMountRepository>
      mount_repository,
    std::shared_ptr<Nyx::Domain::IObservingLocationRepository>
      location_repository,
    std::shared_ptr<Nyx::Domain::IFilterRepository>
      filter_repository,
    std::shared_ptr<IExifParser> exif_parser,
    std::shared_ptr<IFileStorage> file_storage,
    std::shared_ptr<IDngDecoder> dng_decoder,
    std::shared_ptr<IPhotometryProcessor>
      photometry_processor,
    std::shared_ptr<Nyx::Core::IUuidGenerator>
      uuid_generator
  )
    : session_repository(std::move(session_repository))
    , image_repository(std::move(image_repository))
    , target_repository(std::move(target_repository))
    , telescope_repository(std::move(telescope_repository))
    , camera_repository(std::move(camera_repository))
    , mount_repository(std::move(mount_repository))
    , location_repository(std::move(location_repository))
    , filter_repository(std::move(filter_repository))
    , exif_parser(std::move(exif_parser))
    , file_storage(std::move(file_storage))
    , dng_decoder(std::move(dng_decoder))
    , photometry_processor(std::move(photometry_processor))
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
      .target_x = image.target_x,
      .target_y = image.target_y,
      .raw_flux = image.raw_flux,
      .raw_flux_error = image.raw_flux_error,
      .relative_flux = image.relative_flux,
      .relative_flux_error = image.relative_flux_error,
      .photometry_status = image.photometry_status,
      .photometry_error_message =
        image.photometry_error_message,
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
      .telescope_id = session.telescope_id,
      .camera_id = session.camera_id,
      .mount_id = session.mount_id,
      .location_id = session.location_id,
      .filter_id = session.filter_id,
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

    auto equipment_check = this->verify_equipment_ownership(
      user_id, request, logger
    );
    if (!equipment_check.has_value()) {
      return std::unexpected(equipment_check.error());
    }

    auto session = Nyx::Domain::ObservationSession{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .target_id = request.target_id,
      .telescope_id = request.telescope_id,
      .camera_id = request.camera_id,
      .mount_id = request.mount_id,
      .location_id = request.location_id,
      .filter_id = request.filter_id,
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
    if (request.filter_id.has_value()) {
      session.filter_id = request.filter_id;
    }
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
      .target_x = std::nullopt,
      .target_y = std::nullopt,
      .raw_flux = std::nullopt,
      .raw_flux_error = std::nullopt,
      .relative_flux = std::nullopt,
      .relative_flux_error = std::nullopt,
      .photometry_status = std::nullopt,
      .photometry_error_message = std::nullopt,
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
  auto ObservationService::verify_equipment_ownership(
    const std::string& user_id,
    const CreateSessionRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    auto telescope = this->telescope_repository->find_by_id(
      request.telescope_id
    );
    if (!telescope.has_value()) {
      return std::unexpected(telescope.error());
    }
    if (!telescope->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Telescope not found"
        )
      );
    }
    if (telescope->value().user_id != user_id) {
      logger->warn(
        "Equipment ownership failed: telescope {} "
        "belongs to another user",
        request.telescope_id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    auto camera = this->camera_repository->find_by_id(
      request.camera_id
    );
    if (!camera.has_value()) {
      return std::unexpected(camera.error());
    }
    if (!camera->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Camera not found"
        )
      );
    }
    if (camera->value().user_id != user_id) {
      logger->warn(
        "Equipment ownership failed: camera {} "
        "belongs to another user",
        request.camera_id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    auto mount = this->mount_repository->find_by_id(
      request.mount_id
    );
    if (!mount.has_value()) {
      return std::unexpected(mount.error());
    }
    if (!mount->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Mount not found"
        )
      );
    }
    if (mount->value().user_id != user_id) {
      logger->warn(
        "Equipment ownership failed: mount {} "
        "belongs to another user",
        request.mount_id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    auto location = this->location_repository->find_by_id(
      request.location_id
    );
    if (!location.has_value()) {
      return std::unexpected(location.error());
    }
    if (!location->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Observing location not found"
        )
      );
    }
    if (location->value().user_id != user_id) {
      logger->warn(
        "Equipment ownership failed: location {} "
        "belongs to another user",
        request.location_id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    if (request.filter_id.has_value()) {
      auto filter = this->filter_repository->find_by_id(
        request.filter_id.value()
      );
      if (!filter.has_value()) {
        return std::unexpected(filter.error());
      }
      if (!filter->has_value()) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "Filter not found"
          )
        );
      }
      if (filter->value().user_id != user_id) {
        logger->warn(
          "Equipment ownership failed: filter {} "
          "belongs to another user",
          request.filter_id.value()
        );
        return std::unexpected(
          Nyx::Core::AppError::forbidden(
            "Access denied"
          )
        );
      }
    }

    return {};
  }

  auto ObservationService::run_photometry(
    const std::string& user_id,
    const std::string& session_id,
    const RunPhotometryRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<PhotometryStatusResponse> {
    logger->debug(
      "Running photometry for session_id={}, "
      "target=({},{})",
      session_id, request.target_x, request.target_y
    );

    auto session_result = this->verify_session_ownership(
      session_id, user_id, logger
    );
    if (!session_result.has_value()) {
      return std::unexpected(session_result.error());
    }

    auto session = session_result.value();

    auto telescope_result =
      this->telescope_repository->find_by_id(
        session.telescope_id
      );
    if (!telescope_result.has_value()
        || !telescope_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to load telescope"
        )
      );
    }
    auto telescope = telescope_result->value();

    auto camera_result =
      this->camera_repository->find_by_id(
        session.camera_id
      );
    if (!camera_result.has_value()
        || !camera_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to load camera"
        )
      );
    }
    auto camera = camera_result->value();

    auto images_result =
      this->image_repository->find_by_session_id(
        session_id
      );
    if (!images_result.has_value()) {
      return std::unexpected(images_result.error());
    }

    auto images = images_result.value();
    if (images.empty()) {
      return std::unexpected(
        Nyx::Core::AppError::validation(
          "No images in session",
          {{"session", "Upload images first"}}
        )
      );
    }

    for (const auto& img : images) {
      if (img.photometry_status.has_value()
          && img.photometry_status.value()
               == "processing") {
        return std::unexpected(
          Nyx::Core::AppError::conflict(
            "Photometry already in progress"
          )
        );
      }
    }

    auto plate_scale =
      (camera.pixel_size_um
       / static_cast<double>(telescope.focal_length_mm))
      * 206.265;
    auto aperture_radius = std::max(
      5.0, 2.0 / plate_scale
    );
    auto annulus_inner = 2.0 * aperture_radius;
    auto annulus_outer = 3.0 * aperture_radius;

    auto batch_result =
      this->image_repository
        ->update_photometry_status_batch(
          session_id, "processing"
        );
    if (!batch_result.has_value()) {
      return std::unexpected(batch_result.error());
    }

    auto target_x = request.target_x;
    auto target_y = request.target_y;
    auto image_repo = this->image_repository;
    auto decoder = this->dng_decoder;
    auto photometer = this->photometry_processor;

    std::thread([images = std::move(images),
                 image_repo, decoder, photometer,
                 target_x, target_y,
                 aperture_radius, annulus_inner,
                 annulus_outer, session_id]() {
      spdlog::info(
        "Photometry background job started for "
        "session_id={}, {} images",
        session_id, images.size()
      );

      for (auto image : images) {
        try {
          auto decoded = decoder->decode(
            image.file_path
          );
          if (!decoded.has_value()) {
            image.photometry_status = "failed";
            image.photometry_error_message =
              "Failed to decode DNG";
            (void)image_repo->update_photometry(image);
            continue;
          }

          auto target_result =
            photometer->measure_aperture(
              decoded.value(), target_x, target_y,
              aperture_radius, annulus_inner,
              annulus_outer
            );
          if (!target_result.has_value()) {
            image.photometry_status = "failed";
            image.photometry_error_message =
              target_result.error().message;
            (void)image_repo->update_photometry(image);
            continue;
          }

          auto sources = photometer->detect_sources(
            decoded.value(), 5.0
          );

          auto reference_fluxes = std::vector<double>{};
          for (const auto& source : sources) {
            auto dx = source.x - target_x;
            auto dy = source.y - target_y;
            auto dist = std::sqrt(
              static_cast<double>(dx * dx + dy * dy)
            );
            if (dist < 2.0 * aperture_radius) continue;

            auto ref_result =
              photometer->measure_aperture(
                decoded.value(), source.x, source.y,
                aperture_radius, annulus_inner,
                annulus_outer
              );
            if (ref_result.has_value()
                && ref_result->raw_flux > 0) {
              reference_fluxes.push_back(
                ref_result->raw_flux
              );
            }
            if (reference_fluxes.size() >= 5) break;
          }

          if (reference_fluxes.empty()) {
            image.photometry_status = "failed";
            image.photometry_error_message =
              "No reference stars found";
            (void)image_repo->update_photometry(image);
            continue;
          }

          auto mean_ref = 0.0;
          for (auto f : reference_fluxes) mean_ref += f;
          mean_ref /= static_cast<double>(
            reference_fluxes.size()
          );

          auto relative_flux =
            target_result->raw_flux / mean_ref;
          auto relative_flux_error = relative_flux
            * (target_result->raw_flux_error
               / target_result->raw_flux);

          image.target_x = target_x;
          image.target_y = target_y;
          image.raw_flux = target_result->raw_flux;
          image.raw_flux_error =
            target_result->raw_flux_error;
          image.relative_flux = relative_flux;
          image.relative_flux_error = relative_flux_error;
          image.photometry_status = "completed";
          image.photometry_error_message = std::nullopt;
          (void)image_repo->update_photometry(image);
        } catch (const std::exception& e) {
          spdlog::error(
            "Photometry failed for image {}: {}",
            image.id, e.what()
          );
          image.photometry_status = "failed";
          image.photometry_error_message = e.what();
          (void)image_repo->update_photometry(image);
        }
      }

      spdlog::info(
        "Photometry background job completed for "
        "session_id={}",
        session_id
      );
    }).detach();

    logger->info(
      "Photometry started for session_id={}, "
      "{} images queued",
      session_id, images_result->size()
    );

    return PhotometryStatusResponse{
      .session_id = session_id,
      .status = "processing",
      .message = "Photometry processing started for "
        + std::to_string(images_result->size())
        + " images",
    };
  }
} // namespace Nyx::Application::Observation
