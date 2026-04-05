#pragma once

#include "domain/repositories/IObservationSessionRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresObservationSessionRepository
    : public Nyx::Domain::IObservationSessionRepository {
    public:
      auto create(
        const Nyx::Domain::ObservationSession& session
      ) -> Nyx::Core::Result<
        Nyx::Domain::ObservationSession
      > override;

      auto find_by_user_id(const std::string& user_id)
        -> Nyx::Core::Result<
          std::vector<Nyx::Domain::ObservationSession>
        > override;

      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<
          std::optional<Nyx::Domain::ObservationSession>
        > override;

      auto update(
        const Nyx::Domain::ObservationSession& session
      ) -> Nyx::Core::Result<
        Nyx::Domain::ObservationSession
      > override;

      auto find_by_user_id_and_target_id(
        const std::string& user_id,
        const std::string& target_id
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Domain::ObservationSession>
      > override;

      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
