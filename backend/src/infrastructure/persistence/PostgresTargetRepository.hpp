#pragma once

#include "domain/repositories/ITargetRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresTargetRepository
    : public Nyx::Domain::ITargetRepository {
    public:
      auto create(const Nyx::Domain::Target& target)
        -> Nyx::Core::Result<Nyx::Domain::Target> override;
      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::Target>> override;
      auto find_by_canonical_name(const std::string& canonical_name)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::Target>> override;
  };
} // namespace Nyx::Infrastructure::Persistence
