#pragma once

#include "application/observation/Dtos.hpp"
#include "application/observation/IExifParser.hpp"
#include "application/observation/IFileStorage.hpp"
#include "core/error/AppError.hpp"
#include "core/util/UuidGenerator.hpp"
#include "domain/repositories/IObservationImageRepository.hpp"
#include "domain/repositories/IObservationSessionRepository.hpp"
#include "domain/repositories/ITargetRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace Nyx::Application::Observation {
  class ObservationService {
    public:
      ObservationService(
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
      );

      auto create_session(
        const std::string& user_id,
        const CreateSessionRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<SessionResponse>;

      auto list_sessions(
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<
        std::vector<SessionResponse>
      >;

      auto get_session(
        const std::string& user_id,
        const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<SessionResponse>;

      auto update_session(
        const std::string& user_id,
        const std::string& id,
        const UpdateSessionRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<SessionResponse>;

      auto delete_session(
        const std::string& user_id,
        const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      auto upload_image(
        const std::string& user_id,
        const std::string& session_id,
        const UploadedFileData& file_data,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<ImageResponse>;

      auto delete_image(
        const std::string& user_id,
        const std::string& session_id,
        const std::string& image_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

    private:
      auto verify_session_ownership(
        const std::string& session_id,
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<
        Nyx::Domain::ObservationSession
      >;

      auto to_image_response(
        const Nyx::Domain::ObservationImage& image
      ) -> ImageResponse;

      auto to_session_response(
        const Nyx::Domain::ObservationSession& session,
        const std::vector<Nyx::Domain::ObservationImage>&
          images
      ) -> SessionResponse;

      auto detect_mime_type(
        const std::string& filename
      ) -> std::string;

      std::shared_ptr<Nyx::Domain::IObservationSessionRepository>
        session_repository;
      std::shared_ptr<Nyx::Domain::IObservationImageRepository>
        image_repository;
      std::shared_ptr<Nyx::Domain::ITargetRepository>
        target_repository;
      std::shared_ptr<IExifParser> exif_parser;
      std::shared_ptr<IFileStorage> file_storage;
      std::shared_ptr<Nyx::Core::IUuidGenerator>
        uuid_generator;
  };
} // namespace Nyx::Application::Observation
