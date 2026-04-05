#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/LightCurvePoint.hpp"

#include <string>
#include <vector>

namespace Nyx::Domain {
  class ILightCurvePointRepository {
    public:
      virtual ~ILightCurvePointRepository() = default;

      virtual auto bulk_create(
        const std::string& observation_id,
        const std::vector<LightCurvePoint>& points
      ) -> Nyx::Core::Result<int> = 0;

      virtual auto find_by_observation_id(
        const std::string& observation_id,
        bool quality_filter
      ) -> Nyx::Core::Result<std::vector<LightCurvePoint>> = 0;

      virtual auto find_by_observation_id_as_json(
        const std::string& observation_id,
        bool quality_filter
      ) -> Nyx::Core::Result<std::string> = 0;

      virtual auto count_by_observation_id(
        const std::string& observation_id
      ) -> Nyx::Core::Result<int> = 0;

      virtual auto delete_by_observation_id(
        const std::string& observation_id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
