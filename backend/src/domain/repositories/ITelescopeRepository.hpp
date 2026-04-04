#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/Telescope.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class ITelescopeRepository {
    public:
      virtual ~ITelescopeRepository() = default;

      virtual auto create(
        const Telescope& telescope
      ) -> Nyx::Core::Result<Telescope> = 0;

      virtual auto find_by_user_id(
        const std::string& user_id
      ) -> Nyx::Core::Result<std::vector<Telescope>> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<Telescope>> = 0;

      virtual auto update(
        const Telescope& telescope
      ) -> Nyx::Core::Result<Telescope> = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
