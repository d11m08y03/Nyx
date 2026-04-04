#pragma once

#include "domain/repositories/IRefreshTokenRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresRefreshTokenRepository
    : public Nyx::Domain::IRefreshTokenRepository {
    public:
      auto store(
        const Nyx::Domain::RefreshToken& token
      ) -> Nyx::Core::Result<void> override;

      auto find_by_token_hash(
        const std::string& hash
      ) -> Nyx::Core::Result<
        std::optional<Nyx::Domain::RefreshToken>
      > override;

      auto revoke(
        const std::string& id
      ) -> Nyx::Core::Result<void> override;

      auto revoke_family(
        const std::string& family_id
      ) -> Nyx::Core::Result<void> override;

      auto revoke_all_for_user(
        const std::string& user_id
      ) -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
