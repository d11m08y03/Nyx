#pragma once

#include "domain/repositories/ITessObservationRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresTessObservationRepository
    : public Nyx::Domain::ITessObservationRepository {
    public:
      auto create(const Nyx::Domain::TessObservation& observation)
        -> Nyx::Core::Result<Nyx::Domain::TessObservation> override;
      auto find_by_target_id(const std::string& target_id)
        -> Nyx::Core::Result<
          std::vector<Nyx::Domain::TessObservation>
        > override;
      auto find_by_obsid(const std::string& obsid)
        -> Nyx::Core::Result<
          std::optional<Nyx::Domain::TessObservation>
        > override;
  };
} // namespace Nyx::Infrastructure::Persistence
