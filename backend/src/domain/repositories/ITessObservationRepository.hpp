#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/TessObservation.hpp"

#include <optional>
#include <string>
#include <unordered_set>
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

      virtual auto find_existing_obsids(
        const std::vector<std::string>& obsids
      ) -> Nyx::Core::Result<
        std::unordered_set<std::string>
      > = 0;

      virtual auto bulk_create(
        const std::vector<TessObservation>& observations
      ) -> Nyx::Core::Result<std::vector<TessObservation>> = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<std::optional<TessObservation>> = 0;

      virtual auto update_data_uri(
        const std::string& id, const std::string& data_uri
      ) -> Nyx::Core::Result<TessObservation> = 0;
  };
} // namespace Nyx::Domain
