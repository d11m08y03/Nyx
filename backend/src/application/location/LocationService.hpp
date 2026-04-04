#pragma once

#include "application/location/Dtos.hpp"
#include "core/error/AppError.hpp"
#include "core/util/UuidGenerator.hpp"
#include "domain/repositories/IObservingLocationRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace Nyx::Application::Location {
  class LocationService {
    public:
      LocationService(
        std::shared_ptr<Nyx::Domain::IObservingLocationRepository> location_repository,
        std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
      );

      auto create_location(
        const std::string& user_id,
        const CreateLocationRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<LocationResponse>;

      auto list_locations(
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<LocationResponse>>;

      auto get_location(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<LocationResponse>;

      auto update_location(
        const std::string& user_id, const std::string& id,
        const UpdateLocationRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<LocationResponse>;

      auto delete_location(
        const std::string& user_id, const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

    private:
      std::shared_ptr<Nyx::Domain::IObservingLocationRepository> location_repository;
      std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator;
  };
} // namespace Nyx::Application::Location
