#pragma once

#include "domain/repositories/IObservingLocationRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresObservingLocationRepository
    : public Nyx::Domain::IObservingLocationRepository {
    public:
      auto create(const Nyx::Domain::ObservingLocation& location)
        -> Nyx::Core::Result<Nyx::Domain::ObservingLocation> override;
      auto find_by_user_id(const std::string& user_id)
        -> Nyx::Core::Result<std::vector<Nyx::Domain::ObservingLocation>> override;
      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>> override;
      auto find_by_user_id_and_name(
        const std::string& user_id, const std::string& name
      ) -> Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>> override;
      auto update(const Nyx::Domain::ObservingLocation& location)
        -> Nyx::Core::Result<Nyx::Domain::ObservingLocation> override;
      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
