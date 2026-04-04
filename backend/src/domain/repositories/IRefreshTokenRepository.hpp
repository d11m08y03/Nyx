#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/RefreshToken.hpp"

#include <optional>
#include <string>

namespace Nyx::Domain {
  class IRefreshTokenRepository {
    public:
      virtual ~IRefreshTokenRepository() = default;

      virtual auto store(
        const RefreshToken& token
      ) -> Nyx::Core::Result<void> = 0;

      virtual auto find_by_token_hash(
        const std::string& hash
      ) -> Nyx::Core::Result<
        std::optional<RefreshToken>
      > = 0;

      virtual auto revoke(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;

      virtual auto revoke_family(
        const std::string& family_id
      ) -> Nyx::Core::Result<void> = 0;

      virtual auto revoke_all_for_user(
        const std::string& user_id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
