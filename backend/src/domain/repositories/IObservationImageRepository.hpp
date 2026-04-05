#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/ObservationImage.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class IObservationImageRepository {
    public:
      virtual ~IObservationImageRepository() = default;

      virtual auto create(
        const ObservationImage& image
      ) -> Nyx::Core::Result<ObservationImage> = 0;

      virtual auto find_by_session_id(
        const std::string& session_id
      ) -> Nyx::Core::Result<
        std::vector<ObservationImage>
      > = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<
        std::optional<ObservationImage>
      > = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
