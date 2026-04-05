#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/TessObservation.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class ITessObservationRepository {
    public:
      virtual ~ITessObservationRepository() = default;

      virtual auto create(
        const TessObservation& observation
      ) -> Nyx::Core::Result<TessObservation> = 0;

      virtual auto find_by_target_id(
        const std::string& target_id
      ) -> Nyx::Core::Result<std::vector<TessObservation>> = 0;

      virtual auto find_by_obsid(
        const std::string& obsid
      ) -> Nyx::Core::Result<std::optional<TessObservation>> = 0;
  };
} // namespace Nyx::Domain
