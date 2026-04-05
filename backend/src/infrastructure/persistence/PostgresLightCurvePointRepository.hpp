#pragma once

#include "domain/repositories/ILightCurvePointRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresLightCurvePointRepository
    : public Nyx::Domain::ILightCurvePointRepository {
    public:
      auto bulk_create(
        const std::string& observation_id,
        const std::vector<Nyx::Domain::LightCurvePoint>& points
      ) -> Nyx::Core::Result<int> override;

      auto find_by_observation_id(
        const std::string& observation_id,
        bool quality_filter
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Domain::LightCurvePoint>
      > override;

      auto count_by_observation_id(
        const std::string& observation_id
      ) -> Nyx::Core::Result<int> override;

      auto delete_by_observation_id(
        const std::string& observation_id
      ) -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
