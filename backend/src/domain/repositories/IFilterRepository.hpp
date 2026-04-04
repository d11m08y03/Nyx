#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/Filter.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class IFilterRepository {
    public:
      virtual ~IFilterRepository() = default;

      virtual auto create(
        const Filter& filter
      ) -> Nyx::Core::Result<Filter> = 0;

      virtual auto find_by_user_id(
        const std::string& user_id
      ) -> Nyx::Core::Result<std::vector<Filter>> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<Filter>> = 0;

      virtual auto update(
        const Filter& filter
      ) -> Nyx::Core::Result<Filter> = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
