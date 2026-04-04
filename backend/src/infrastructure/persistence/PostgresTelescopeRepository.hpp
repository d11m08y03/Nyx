#pragma once

#include "domain/repositories/ITelescopeRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresTelescopeRepository : public Nyx::Domain::ITelescopeRepository {
    public:
      auto create(const Nyx::Domain::Telescope& telescope)
        -> Nyx::Core::Result<Nyx::Domain::Telescope> override;
      auto find_by_user_id(const std::string& user_id)
        -> Nyx::Core::Result<std::vector<Nyx::Domain::Telescope>> override;
      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::Telescope>> override;
      auto update(const Nyx::Domain::Telescope& telescope)
        -> Nyx::Core::Result<Nyx::Domain::Telescope> override;
      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
