#pragma once

#include "domain/repositories/IVerificationTokenRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresVerificationTokenRepository
    : public Nyx::Domain::IVerificationTokenRepository {
    public:
      auto store(
        const Nyx::Domain::VerificationToken& token
      ) -> Nyx::Core::Result<void> override;

      auto find_by_token_hash(
        const std::string& token_hash
      ) -> Nyx::Core::Result<
        std::optional<Nyx::Domain::VerificationToken>
      > override;

      auto mark_used(
        const std::string& id
      ) -> Nyx::Core::Result<void> override;

      auto revoke_all_for_user(
        const std::string& user_id
      ) -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
