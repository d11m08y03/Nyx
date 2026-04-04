#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/Camera.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class ICameraRepository {
    public:
      virtual ~ICameraRepository() = default;

      virtual auto create(
        const Camera& camera
      ) -> Nyx::Core::Result<Camera> = 0;

      virtual auto find_by_user_id(
        const std::string& user_id
      ) -> Nyx::Core::Result<std::vector<Camera>> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<Camera>> = 0;

      virtual auto update(
        const Camera& camera
      ) -> Nyx::Core::Result<Camera> = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
