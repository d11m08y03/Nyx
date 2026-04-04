#pragma once

#include "domain/repositories/ICameraRepository.hpp"

namespace Nyx::Infrastructure::Persistence {
  class PostgresCameraRepository : public Nyx::Domain::ICameraRepository {
    public:
      auto create(const Nyx::Domain::Camera& camera)
        -> Nyx::Core::Result<Nyx::Domain::Camera> override;
      auto find_by_user_id(const std::string& user_id)
        -> Nyx::Core::Result<std::vector<Nyx::Domain::Camera>> override;
      auto find_by_id(const std::string& id)
        -> Nyx::Core::Result<std::optional<Nyx::Domain::Camera>> override;
      auto update(const Nyx::Domain::Camera& camera)
        -> Nyx::Core::Result<Nyx::Domain::Camera> override;
      auto remove(const std::string& id)
        -> Nyx::Core::Result<void> override;
  };
} // namespace Nyx::Infrastructure::Persistence
