#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/ObservingLocation.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class IObservingLocationRepository {
    public:
      virtual ~IObservingLocationRepository() = default;

      virtual auto create(
        const ObservingLocation& location
      ) -> Nyx::Core::Result<ObservingLocation> = 0;

      virtual auto find_by_user_id(
        const std::string& user_id
      ) -> Nyx::Core::Result<std::vector<ObservingLocation>> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<ObservingLocation>> = 0;

      virtual auto find_by_user_id_and_name(
        const std::string& user_id,
        const std::string& name
      ) -> Nyx::Core::Result<std::optional<ObservingLocation>> = 0;

      virtual auto update(
        const ObservingLocation& location
      ) -> Nyx::Core::Result<ObservingLocation> = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
