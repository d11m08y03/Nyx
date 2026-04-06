#pragma once

#include "domain/repositories/IUserRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresUserRepository
    : public Nyx::Domain::IUserRepository {
    public:
      auto create(const Nyx::Domain::User& user)
        -> Nyx::Core::Result<Nyx::Domain::User> override;

      auto find_by_email(const std::string& email)
        -> Nyx::Core::Result<
          std::optional<Nyx::Domain::User>
        > override;

      auto update_display_name(
        const std::string& user_id,
        const std::string& display_name
      ) -> Nyx::Core::Result<void> override;

      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<
          std::optional<Nyx::Domain::User>
        > override;

      auto verify_email(const std::string& user_id)
        -> Nyx::Core::Result<void> override;

      auto find_by_google_id(
        const std::string& google_id
      ) -> Nyx::Core::Result<
        std::optional<Nyx::Domain::User>
      > override;

  };
} // namespace Nyx::Infrastructure::Persistence
