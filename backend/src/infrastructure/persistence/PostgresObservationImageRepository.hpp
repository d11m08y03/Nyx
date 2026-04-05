#pragma once

#include "domain/repositories/IObservationImageRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresObservationImageRepository
    : public Nyx::Domain::IObservationImageRepository {
    public:
      auto create(
        const Nyx::Domain::ObservationImage& image
      ) -> Nyx::Core::Result<
        Nyx::Domain::ObservationImage
      > override;

      auto find_by_session_id(
        const std::string& session_id
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Domain::ObservationImage>
      > override;

      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<
          std::optional<Nyx::Domain::ObservationImage>
        > override;

      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;

      auto update_photometry(
        const Nyx::Domain::ObservationImage& image
      ) -> Nyx::Core::Result<
        Nyx::Domain::ObservationImage
      > override;

      auto update_photometry_status_batch(
        const std::string& session_id,
        const std::string& status
      ) -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
