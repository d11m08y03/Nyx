#include "application/location/LocationService.hpp"

namespace Nyx::Application::Location {
  LocationService::LocationService(
    std::shared_ptr<Nyx::Domain::IObservingLocationRepository> location_repository,
    std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
  )
    : location_repository(std::move(location_repository))
    , uuid_generator(std::move(uuid_generator)) {
    spdlog::debug("LocationService initialized");
  }

  auto LocationService::create_location(
    const std::string& user_id,
    const CreateLocationRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<LocationResponse> {
    logger->debug("Creating observing location for user_id={}", user_id);

    auto existing = this->location_repository->find_by_user_id_and_name(
      user_id, request.name
    );

    if (!existing.has_value()) {
      return std::unexpected(existing.error());
    }

    if (existing->has_value()) {
      logger->warn(
        "Duplicate location name='{}' for user_id={}", request.name, user_id
      );
      return std::unexpected(Nyx::Core::AppError::conflict(
        "An observing location with this name already exists"
      ));
    }

    auto location = Nyx::Domain::ObservingLocation{
      .id = this->uuid_generator->generate(),
      .user_id = user_id,
      .name = request.name,
      .latitude = request.latitude,
      .longitude = request.longitude,
      .bortle_class = request.bortle_class,
    };

    auto result = this->location_repository->create(location);

    if (!result.has_value()) {
      logger->error(
        "Failed to create observing location for user_id={}", user_id
      );
      return std::unexpected(result.error());
    }

    auto created = result.value();
    logger->info(
      "Observing location created id={}, user_id={}", created.id, user_id
    );

    return LocationResponse{
      .id = created.id, .user_id = created.user_id,
      .name = created.name, .latitude = created.latitude,
      .longitude = created.longitude,
      .bortle_class = created.bortle_class,
    };
  }

  auto LocationService::list_locations(
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<LocationResponse>> {
    logger->debug("Listing observing locations for user_id={}", user_id);

    auto result = this->location_repository->find_by_user_id(user_id);

    if (!result.has_value()) {
      logger->error(
        "Failed to list observing locations for user_id={}", user_id
      );
      return std::unexpected(result.error());
    }

    auto responses = std::vector<LocationResponse>{};
    for (const auto& location : result.value()) {
      responses.push_back(LocationResponse{
        .id = location.id, .user_id = location.user_id,
        .name = location.name, .latitude = location.latitude,
        .longitude = location.longitude,
        .bortle_class = location.bortle_class,
      });
    }
    return responses;
  }

  auto LocationService::get_location(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<LocationResponse> {
    logger->debug(
      "Getting observing location id={} for user_id={}", id, user_id
    );

    auto result = this->location_repository->find_by_id(id);

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Observing location not found")
      );
    }

    auto location = result->value();

    if (location.user_id != user_id) {
      logger->warn(
        "Ownership check failed for observing location id={}", id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    return LocationResponse{
      .id = location.id, .user_id = location.user_id,
      .name = location.name, .latitude = location.latitude,
      .longitude = location.longitude,
      .bortle_class = location.bortle_class,
    };
  }

  auto LocationService::update_location(
    const std::string& user_id, const std::string& id,
    const UpdateLocationRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<LocationResponse> {
    logger->debug(
      "Updating observing location id={} for user_id={}", id, user_id
    );

    auto existing_result = this->location_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Observing location not found")
      );
    }

    auto existing = existing_result->value();

    if (existing.user_id != user_id) {
      logger->warn(
        "Ownership check failed for observing location id={}", id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    if (existing.name != request.name) {
      auto name_check =
        this->location_repository->find_by_user_id_and_name(
          user_id, request.name
        );

      if (!name_check.has_value()) {
        return std::unexpected(name_check.error());
      }

      if (name_check->has_value()) {
        logger->warn(
          "Duplicate location name='{}' for user_id={}",
          request.name, user_id
        );
        return std::unexpected(Nyx::Core::AppError::conflict(
          "An observing location with this name already exists"
        ));
      }
    }

    auto updated_location = Nyx::Domain::ObservingLocation{
      .id = id, .user_id = user_id,
      .name = request.name, .latitude = request.latitude,
      .longitude = request.longitude,
      .bortle_class = request.bortle_class,
    };

    auto result = this->location_repository->update(updated_location);

    if (!result.has_value()) {
      logger->error("Failed to update observing location id={}", id);
      return std::unexpected(result.error());
    }

    auto updated = result.value();
    logger->info(
      "Observing location updated id={}, user_id={}", id, user_id
    );

    return LocationResponse{
      .id = updated.id, .user_id = updated.user_id,
      .name = updated.name, .latitude = updated.latitude,
      .longitude = updated.longitude,
      .bortle_class = updated.bortle_class,
    };
  }

  auto LocationService::delete_location(
    const std::string& user_id, const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug(
      "Deleting observing location id={} for user_id={}", id, user_id
    );

    auto existing_result = this->location_repository->find_by_id(id);

    if (!existing_result.has_value()) {
      return std::unexpected(existing_result.error());
    }

    if (!existing_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Observing location not found")
      );
    }

    if (existing_result->value().user_id != user_id) {
      logger->warn(
        "Ownership check failed for observing location id={}", id
      );
      return std::unexpected(
        Nyx::Core::AppError::forbidden("Access denied")
      );
    }

    auto result = this->location_repository->remove(id);

    if (!result.has_value()) {
      logger->error("Failed to delete observing location id={}", id);
      return std::unexpected(result.error());
    }

    logger->info(
      "Observing location deleted id={}, user_id={}", id, user_id
    );
    return {};
  }
} // namespace Nyx::Application::Location
