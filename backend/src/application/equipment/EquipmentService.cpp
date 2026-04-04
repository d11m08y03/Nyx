#include "application/equipment/EquipmentService.hpp"

namespace Nyx::Application::Equipment {
  EquipmentService::EquipmentService(
    std::shared_ptr<Nyx::Domain::ITelescopeRepository> telescope_repository,
    std::shared_ptr<Nyx::Domain::ICameraRepository> camera_repository,
    std::shared_ptr<Nyx::Domain::IMountRepository> mount_repository,
    std::shared_ptr<Nyx::Domain::IFilterRepository> filter_repository,
    std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
  )
    : telescope_repository(std::move(telescope_repository))
    , camera_repository(std::move(camera_repository))
    , mount_repository(std::move(mount_repository))
    , filter_repository(std::move(filter_repository))
    , uuid_generator(std::move(uuid_generator)) {
    spdlog::debug("EquipmentService initialized");
  }

  auto EquipmentService::verify_ownership(
    const std::string& resource_user_id,
    const std::string& requesting_user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    if (resource_user_id != requesting_user_id) {
      logger->warn(
        "Ownership check failed: resource belongs to user_id={}, "
        "requested by user_id={}",
        resource_user_id, requesting_user_id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }
    return {};
  }

  // --- Telescopes ---

  auto EquipmentService::create_telescope(
    const std::string& user_id,
    const CreateTelescopeRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TelescopeResponse> {
    logger->debug("Creating telescope for user_id={}", user_id);

    auto telescope = Nyx::Domain::Telescope{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .name = request.name,
      .aperture_mm = request.aperture_mm,
      .focal_length_mm = request.focal_length_mm,
      .optical_design = request.optical_design,
      .brand = request.brand,
      .model = request.model,
    };

    auto result = this->telescope_repository->create(telescope);

    if (!result.has_value()) {
      logger->error("Failed to create telescope for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto created = result.value();
    logger->info("Telescope created id={}, user_id={}", created.id, user_id);

    return TelescopeResponse{
      .id = created.id, .user_id = created.user_id,
      .name = created.name, .aperture_mm = created.aperture_mm,
      .focal_length_mm = created.focal_length_mm,
      .optical_design = created.optical_design,
      .brand = created.brand, .model = created.model,
    };
  }

  auto EquipmentService::list_telescopes(
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<TelescopeResponse>> {
    logger->debug("Listing telescopes for user_id={}", user_id);

    auto result = this->telescope_repository->find_by_user_id(user_id);

    if (!result.has_value()) {
      logger->error("Failed to list telescopes for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto responses = std::vector<TelescopeResponse>{};
    for (const auto& telescope : result.value()) {
      responses.push_back(TelescopeResponse{
        .id = telescope.id, .user_id = telescope.user_id,
        .name = telescope.name, .aperture_mm = telescope.aperture_mm,
        .focal_length_mm = telescope.focal_length_mm,
        .optical_design = telescope.optical_design,
        .brand = telescope.brand, .model = telescope.model,
      });
    }
    return responses;
  }

  auto EquipmentService::get_telescope(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TelescopeResponse> {
    logger->debug("Getting telescope id={} for user_id={}", id, user_id);

    auto result = this->telescope_repository->find_by_id(id);

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Telescope not found")
      );
    }

    auto telescope = result->value();
    auto ownership = this->verify_ownership(telescope.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    return TelescopeResponse{
      .id = telescope.id, .user_id = telescope.user_id,
      .name = telescope.name, .aperture_mm = telescope.aperture_mm,
      .focal_length_mm = telescope.focal_length_mm,
      .optical_design = telescope.optical_design,
      .brand = telescope.brand, .model = telescope.model,
    };
  }

  auto EquipmentService::update_telescope(
    const std::string& user_id, const std::string& id,
    const UpdateTelescopeRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TelescopeResponse> {
    logger->debug("Updating telescope id={} for user_id={}", id, user_id);

    auto existing_result = this->telescope_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Telescope not found")
      );
    }

    auto existing = existing_result->value();
    auto ownership = this->verify_ownership(existing.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto updated_telescope = Nyx::Domain::Telescope{
      .id = id, .user_id = user_id,
      .name = request.name, .aperture_mm = request.aperture_mm,
      .focal_length_mm = request.focal_length_mm,
      .optical_design = request.optical_design,
      .brand = request.brand, .model = request.model,
    };

    auto result = this->telescope_repository->update(updated_telescope);

    if (!result.has_value()) {
      logger->error("Failed to update telescope id={}", id);
      return std::unexpected(result.error());
    }

    auto updated = result.value();
    logger->info("Telescope updated id={}, user_id={}", id, user_id);

    return TelescopeResponse{
      .id = updated.id, .user_id = updated.user_id,
      .name = updated.name, .aperture_mm = updated.aperture_mm,
      .focal_length_mm = updated.focal_length_mm,
      .optical_design = updated.optical_design,
      .brand = updated.brand, .model = updated.model,
    };
  }

  auto EquipmentService::delete_telescope(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Deleting telescope id={} for user_id={}", id, user_id);

    auto existing_result = this->telescope_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Telescope not found")
      );
    }

    auto ownership = this->verify_ownership(
      existing_result->value().user_id, user_id, logger
    );

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto result = this->telescope_repository->remove(id);

    if (!result.has_value()) {
      logger->error("Failed to delete telescope id={}", id);
      return std::unexpected(result.error());
    }

    logger->info("Telescope deleted id={}, user_id={}", id, user_id);
    return {};
  }

  // --- Cameras ---

  auto EquipmentService::create_camera(
    const std::string& user_id,
    const CreateCameraRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<CameraResponse> {
    logger->debug("Creating camera for user_id={}", user_id);

    auto camera = Nyx::Domain::Camera{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .name = request.name,
      .sensor_type = request.sensor_type,
      .brand = request.brand,
      .model = request.model,
      .pixel_size_um = request.pixel_size_um,
      .resolution = request.resolution,
    };

    auto result = this->camera_repository->create(camera);

    if (!result.has_value()) {
      logger->error("Failed to create camera for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto created = result.value();
    logger->info("Camera created id={}, user_id={}", created.id, user_id);

    return CameraResponse{
      .id = created.id, .user_id = created.user_id,
      .name = created.name, .sensor_type = created.sensor_type,
      .brand = created.brand, .model = created.model,
      .pixel_size_um = created.pixel_size_um,
      .resolution = created.resolution,
    };
  }

  auto EquipmentService::list_cameras(
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<CameraResponse>> {
    logger->debug("Listing cameras for user_id={}", user_id);

    auto result = this->camera_repository->find_by_user_id(user_id);

    if (!result.has_value()) {
      logger->error("Failed to list cameras for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto responses = std::vector<CameraResponse>{};
    for (const auto& camera : result.value()) {
      responses.push_back(CameraResponse{
        .id = camera.id, .user_id = camera.user_id,
        .name = camera.name, .sensor_type = camera.sensor_type,
        .brand = camera.brand, .model = camera.model,
        .pixel_size_um = camera.pixel_size_um,
        .resolution = camera.resolution,
      });
    }
    return responses;
  }

  auto EquipmentService::get_camera(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<CameraResponse> {
    logger->debug("Getting camera id={} for user_id={}", id, user_id);

    auto result = this->camera_repository->find_by_id(id);

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Camera not found")
      );
    }

    auto camera = result->value();
    auto ownership = this->verify_ownership(camera.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    return CameraResponse{
      .id = camera.id, .user_id = camera.user_id,
      .name = camera.name, .sensor_type = camera.sensor_type,
      .brand = camera.brand, .model = camera.model,
      .pixel_size_um = camera.pixel_size_um,
      .resolution = camera.resolution,
    };
  }

  auto EquipmentService::update_camera(
    const std::string& user_id, const std::string& id,
    const UpdateCameraRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<CameraResponse> {
    logger->debug("Updating camera id={} for user_id={}", id, user_id);

    auto existing_result = this->camera_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Camera not found")
      );
    }

    auto existing = existing_result->value();
    auto ownership = this->verify_ownership(existing.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto updated_camera = Nyx::Domain::Camera{
      .id = id, .user_id = user_id,
      .name = request.name, .sensor_type = request.sensor_type,
      .brand = request.brand, .model = request.model,
      .pixel_size_um = request.pixel_size_um,
      .resolution = request.resolution,
    };

    auto result = this->camera_repository->update(updated_camera);

    if (!result.has_value()) {
      logger->error("Failed to update camera id={}", id);
      return std::unexpected(result.error());
    }

    auto updated = result.value();
    logger->info("Camera updated id={}, user_id={}", id, user_id);

    return CameraResponse{
      .id = updated.id, .user_id = updated.user_id,
      .name = updated.name, .sensor_type = updated.sensor_type,
      .brand = updated.brand, .model = updated.model,
      .pixel_size_um = updated.pixel_size_um,
      .resolution = updated.resolution,
    };
  }

  auto EquipmentService::delete_camera(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Deleting camera id={} for user_id={}", id, user_id);

    auto existing_result = this->camera_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Camera not found")
      );
    }

    auto ownership = this->verify_ownership(
      existing_result->value().user_id, user_id, logger
    );

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto result = this->camera_repository->remove(id);

    if (!result.has_value()) {
      logger->error("Failed to delete camera id={}", id);
      return std::unexpected(result.error());
    }

    logger->info("Camera deleted id={}, user_id={}", id, user_id);
    return {};
  }

  // --- Mounts ---

  auto EquipmentService::create_mount(
    const std::string& user_id,
    const CreateMountRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<MountResponse> {
    logger->debug("Creating mount for user_id={}", user_id);

    auto mount = Nyx::Domain::Mount{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .name = request.name,
      .mount_type = request.mount_type,
      .is_tracked = request.is_tracked,
      .has_goto = request.has_goto,
      .brand = request.brand,
      .model = request.model,
    };

    auto result = this->mount_repository->create(mount);

    if (!result.has_value()) {
      logger->error("Failed to create mount for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto created = result.value();
    logger->info("Mount created id={}, user_id={}", created.id, user_id);

    return MountResponse{
      .id = created.id, .user_id = created.user_id,
      .name = created.name, .mount_type = created.mount_type,
      .is_tracked = created.is_tracked, .has_goto = created.has_goto,
      .brand = created.brand, .model = created.model,
    };
  }

  auto EquipmentService::list_mounts(
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<MountResponse>> {
    logger->debug("Listing mounts for user_id={}", user_id);

    auto result = this->mount_repository->find_by_user_id(user_id);

    if (!result.has_value()) {
      logger->error("Failed to list mounts for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto responses = std::vector<MountResponse>{};
    for (const auto& mount : result.value()) {
      responses.push_back(MountResponse{
        .id = mount.id, .user_id = mount.user_id,
        .name = mount.name, .mount_type = mount.mount_type,
        .is_tracked = mount.is_tracked, .has_goto = mount.has_goto,
        .brand = mount.brand, .model = mount.model,
      });
    }
    return responses;
  }

  auto EquipmentService::get_mount(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<MountResponse> {
    logger->debug("Getting mount id={} for user_id={}", id, user_id);

    auto result = this->mount_repository->find_by_id(id);

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Mount not found")
      );
    }

    auto mount = result->value();
    auto ownership = this->verify_ownership(mount.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    return MountResponse{
      .id = mount.id, .user_id = mount.user_id,
      .name = mount.name, .mount_type = mount.mount_type,
      .is_tracked = mount.is_tracked, .has_goto = mount.has_goto,
      .brand = mount.brand, .model = mount.model,
    };
  }

  auto EquipmentService::update_mount(
    const std::string& user_id, const std::string& id,
    const UpdateMountRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<MountResponse> {
    logger->debug("Updating mount id={} for user_id={}", id, user_id);

    auto existing_result = this->mount_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Mount not found")
      );
    }

    auto existing = existing_result->value();
    auto ownership = this->verify_ownership(existing.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto updated_mount = Nyx::Domain::Mount{
      .id = id, .user_id = user_id,
      .name = request.name, .mount_type = request.mount_type,
      .is_tracked = request.is_tracked, .has_goto = request.has_goto,
      .brand = request.brand, .model = request.model,
    };

    auto result = this->mount_repository->update(updated_mount);

    if (!result.has_value()) {
      logger->error("Failed to update mount id={}", id);
      return std::unexpected(result.error());
    }

    auto updated = result.value();
    logger->info("Mount updated id={}, user_id={}", id, user_id);

    return MountResponse{
      .id = updated.id, .user_id = updated.user_id,
      .name = updated.name, .mount_type = updated.mount_type,
      .is_tracked = updated.is_tracked, .has_goto = updated.has_goto,
      .brand = updated.brand, .model = updated.model,
    };
  }

  auto EquipmentService::delete_mount(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Deleting mount id={} for user_id={}", id, user_id);

    auto existing_result = this->mount_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Mount not found")
      );
    }

    auto ownership = this->verify_ownership(
      existing_result->value().user_id, user_id, logger
    );

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto result = this->mount_repository->remove(id);

    if (!result.has_value()) {
      logger->error("Failed to delete mount id={}", id);
      return std::unexpected(result.error());
    }

    logger->info("Mount deleted id={}, user_id={}", id, user_id);
    return {};
  }

  // --- Filters ---

  auto EquipmentService::create_filter(
    const std::string& user_id,
    const CreateFilterRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<FilterResponse> {
    logger->debug("Creating filter for user_id={}", user_id);

    auto filter = Nyx::Domain::Filter{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .name = request.name,
      .band = request.band,
      .bandwidth_nm = request.bandwidth_nm,
      .brand = request.brand,
      .model = request.model,
    };

    auto result = this->filter_repository->create(filter);

    if (!result.has_value()) {
      logger->error("Failed to create filter for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto created = result.value();
    logger->info("Filter created id={}, user_id={}", created.id, user_id);

    return FilterResponse{
      .id = created.id, .user_id = created.user_id,
      .name = created.name, .band = created.band,
      .bandwidth_nm = created.bandwidth_nm,
      .brand = created.brand, .model = created.model,
    };
  }

  auto EquipmentService::list_filters(
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<FilterResponse>> {
    logger->debug("Listing filters for user_id={}", user_id);

    auto result = this->filter_repository->find_by_user_id(user_id);

    if (!result.has_value()) {
      logger->error("Failed to list filters for user_id={}", user_id);
      return std::unexpected(result.error());
    }

    auto responses = std::vector<FilterResponse>{};
    for (const auto& filter : result.value()) {
      responses.push_back(FilterResponse{
        .id = filter.id, .user_id = filter.user_id,
        .name = filter.name, .band = filter.band,
        .bandwidth_nm = filter.bandwidth_nm,
        .brand = filter.brand, .model = filter.model,
      });
    }
    return responses;
  }

  auto EquipmentService::get_filter(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<FilterResponse> {
    logger->debug("Getting filter id={} for user_id={}", id, user_id);

    auto result = this->filter_repository->find_by_id(id);

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Filter not found")
      );
    }

    auto filter = result->value();
    auto ownership = this->verify_ownership(filter.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    return FilterResponse{
      .id = filter.id, .user_id = filter.user_id,
      .name = filter.name, .band = filter.band,
      .bandwidth_nm = filter.bandwidth_nm,
      .brand = filter.brand, .model = filter.model,
    };
  }

  auto EquipmentService::update_filter(
    const std::string& user_id, const std::string& id,
    const UpdateFilterRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<FilterResponse> {
    logger->debug("Updating filter id={} for user_id={}", id, user_id);

    auto existing_result = this->filter_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Filter not found")
      );
    }

    auto existing = existing_result->value();
    auto ownership = this->verify_ownership(existing.user_id, user_id, logger);

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto updated_filter = Nyx::Domain::Filter{
      .id = id, .user_id = user_id,
      .name = request.name, .band = request.band,
      .bandwidth_nm = request.bandwidth_nm,
      .brand = request.brand, .model = request.model,
    };

    auto result = this->filter_repository->update(updated_filter);

    if (!result.has_value()) {
      logger->error("Failed to update filter id={}", id);
      return std::unexpected(result.error());
    }

    auto updated = result.value();
    logger->info("Filter updated id={}, user_id={}", id, user_id);

    return FilterResponse{
      .id = updated.id, .user_id = updated.user_id,
      .name = updated.name, .band = updated.band,
      .bandwidth_nm = updated.bandwidth_nm,
      .brand = updated.brand, .model = updated.model,
    };
  }

  auto EquipmentService::delete_filter(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Deleting filter id={} for user_id={}", id, user_id);

    auto existing_result = this->filter_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Filter not found")
      );
    }

    auto ownership = this->verify_ownership(
      existing_result->value().user_id, user_id, logger
    );

    if (!ownership.has_value()) {
      return std::unexpected(ownership.error());
    }

    auto result = this->filter_repository->remove(id);

    if (!result.has_value()) {
      logger->error("Failed to delete filter id={}", id);
      return std::unexpected(result.error());
    }

    logger->info("Filter deleted id={}, user_id={}", id, user_id);
    return {};
  }
} // namespace Nyx::Application::Equipment
