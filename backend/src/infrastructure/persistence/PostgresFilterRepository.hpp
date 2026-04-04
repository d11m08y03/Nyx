#pragma once

#include "domain/repositories/IFilterRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresFilterRepository : public Nyx::Domain::IFilterRepository {
    public:
      auto create(const Nyx::Domain::Filter& filter)
        -> Nyx::Core::Result<Nyx::Domain::Filter> override;
      auto find_by_user_id(const std::string& user_id)
        -> Nyx::Core::Result<std::vector<Nyx::Domain::Filter>> override;
      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::Filter>> override;
      auto update(const Nyx::Domain::Filter& filter)
        -> Nyx::Core::Result<Nyx::Domain::Filter> override;
      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
