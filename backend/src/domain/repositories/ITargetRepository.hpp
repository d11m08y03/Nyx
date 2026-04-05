#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/Target.hpp"

#include <optional>
#include <string>

namespace Nyx::Domain {
  class ITargetRepository {
    public:
      virtual ~ITargetRepository() = default;

      virtual auto create(
        const Target& target
      ) -> Nyx::Core::Result<Target> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<Target>> = 0;

      virtual auto find_by_canonical_name(
        const std::string& canonical_name
      ) -> Nyx::Core::Result<std::optional<Target>> = 0;
  };
} // namespace Nyx::Domain
