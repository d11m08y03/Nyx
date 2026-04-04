#pragma once

#include "application/equipment/Dtos.hpp"
#include "core/error/AppError.hpp"
#include "core/util/UuidGenerator.hpp"
#include "domain/repositories/ICameraRepository.hpp"
#include "domain/repositories/IFilterRepository.hpp"
#include "domain/repositories/IMountRepository.hpp"
#include "domain/repositories/ITelescopeRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace Nyx::Application::Equipment {
  class EquipmentService {
    public:
      EquipmentService(
        std::shared_ptr<Nyx::Domain::ITelescopeRepository> telescope_repository,
        std::shared_ptr<Nyx::Domain::ICameraRepository> camera_repository,
        std::shared_ptr<Nyx::Domain::IMountRepository> mount_repository,
        std::shared_ptr<Nyx::Domain::IFilterRepository> filter_repository,
        std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
      );

      // Telescopes
      auto create_telescope(
        const std::string& user_id,
        const CreateTelescopeRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TelescopeResponse>;

      auto list_telescopes(
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<TelescopeResponse>>;

      auto get_telescope(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TelescopeResponse>;

      auto update_telescope(
        const std::string& user_id, const std::string& id,
        const UpdateTelescopeRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TelescopeResponse>;

      auto delete_telescope(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      // Cameras
      auto create_camera(
        const std::string& user_id,
        const CreateCameraRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<CameraResponse>;

      auto list_cameras(
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<CameraResponse>>;

      auto get_camera(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<CameraResponse>;

      auto update_camera(
        const std::string& user_id, const std::string& id,
        const UpdateCameraRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<CameraResponse>;

      auto delete_camera(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      // Mounts
      auto create_mount(
        const std::string& user_id,
        const CreateMountRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<MountResponse>;

      auto list_mounts(
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<MountResponse>>;

      auto get_mount(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<MountResponse>;

      auto update_mount(
        const std::string& user_id, const std::string& id,
        const UpdateMountRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<MountResponse>;

      auto delete_mount(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      // Filters
      auto create_filter(
        const std::string& user_id,
        const CreateFilterRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<FilterResponse>;

      auto list_filters(
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<FilterResponse>>;

      auto get_filter(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<FilterResponse>;

      auto update_filter(
        const std::string& user_id, const std::string& id,
        const UpdateFilterRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<FilterResponse>;

      auto delete_filter(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

    private:
      auto verify_ownership(
        const std::string& resource_user_id,
        const std::string& requesting_user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      std::shared_ptr<Nyx::Domain::ITelescopeRepository> telescope_repository;
      std::shared_ptr<Nyx::Domain::ICameraRepository> camera_repository;
      std::shared_ptr<Nyx::Domain::IMountRepository> mount_repository;
      std::shared_ptr<Nyx::Domain::IFilterRepository> filter_repository;
      std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator;
  };
} // namespace Nyx::Application::Equipment
