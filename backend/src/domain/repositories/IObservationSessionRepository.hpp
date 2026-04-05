#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/ObservationSession.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Domain {
  class IObservationSessionRepository {
    public:
      virtual ~IObservationSessionRepository() = default;

      virtual auto create(
        const ObservationSession& session
      ) -> Nyx::Core::Result<ObservationSession> = 0;

      virtual auto find_by_user_id(
        const std::string& user_id
      ) -> Nyx::Core::Result<
        std::vector<ObservationSession>
      > = 0;

      virtual auto find_by_id(
        const std::string& id
      ) -> Nyx::Core::Result<
        std::optional<ObservationSession>
      > = 0;

      virtual auto update(
        const ObservationSession& session
      ) -> Nyx::Core::Result<ObservationSession> = 0;

      virtual auto remove(
        const std::string& id
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Domain
