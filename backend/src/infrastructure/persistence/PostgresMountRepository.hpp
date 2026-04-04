#pragma once

#include "domain/repositories/IMountRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresMountRepository : public Nyx::Domain::IMountRepository {
    public:
      auto create(const Nyx::Domain::Mount& mount)
        -> Nyx::Core::Result<Nyx::Domain::Mount> override;
      auto find_by_user_id(const std::string& user_id)
        -> Nyx::Core::Result<std::vector<Nyx::Domain::Mount>> override;
      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::Mount>> override;
      auto update(const Nyx::Domain::Mount& mount)
        -> Nyx::Core::Result<Nyx::Domain::Mount> override;
      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
