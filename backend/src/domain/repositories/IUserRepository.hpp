#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/User.hpp"

#include <optional>
#include <string>

namespace Nyx::Domain {
  class IUserRepository {
    public:
      virtual ~IUserRepository() = default;

      virtual auto create(
        const User& user
      ) -> Nyx::Core::Result<User> = 0;

      virtual auto find_by_email(
        const std::string& email
      ) -> Nyx::Core::Result<
        std::optional<User>
      > = 0;

      virtual auto update_display_name(
        const std::string& user_id,
        const std::string& display_name
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
