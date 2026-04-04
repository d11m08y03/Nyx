#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/Mount.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class IMountRepository {
    public:
      virtual ~IMountRepository() = default;

      virtual auto create(
        const Mount& mount
      ) -> Nyx::Core::Result<Mount> = 0;

      virtual auto find_by_user_id(
        const std::string& user_id
      ) -> Nyx::Core::Result<std::vector<Mount>> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<Mount>> = 0;

      virtual auto update(
        const Mount& mount
      ) -> Nyx::Core::Result<Mount> = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
