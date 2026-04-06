#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/VerificationToken.hpp"

#include <optional>
#include <string>

namespace Nyx::Domain {
  class IVerificationTokenRepository {
    public:
      virtual ~IVerificationTokenRepository() = default;

      virtual auto store(
        const VerificationToken& token
      ) -> Nyx::Core::Result<void> = 0;

      virtual auto find_by_token_hash(
        const std::string& token_hash
      ) -> Nyx::Core::Result<
        std::optional<VerificationToken>
      > = 0;

      virtual auto mark_used(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;

      virtual auto revoke_all_for_user(
        const std::string& user_id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
