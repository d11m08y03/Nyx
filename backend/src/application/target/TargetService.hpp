#pragma once

#include "application/target/Dtos.hpp"
#include "application/target/IMastClient.hpp"
#include "core/error/AppError.hpp"
#include "core/util/UuidGenerator.hpp"
#include "domain/repositories/ITargetRepository.hpp"
#include "domain/repositories/ITessObservationRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace Nyx::Application::Target {
  class TargetService {
    public:
      TargetService(
        std::shared_ptr<IMastClient> mast_client,
        std::shared_ptr<Nyx::Domain::ITargetRepository> target_repository,
        std::shared_ptr<Nyx::Domain::ITessObservationRepository>
          tess_observation_repository,
        std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
      );

      auto resolve_target(
        const ResolveTargetRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TargetResponse>;

      auto get_target(
        const std::string& id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TargetResponse>;

      auto list_tess_observations(
        const std::string& target_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<TessObservationResponse>>;

    private:
      auto build_observation_responses(
        const std::string& target_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<std::vector<TessObservationResponse>>;

      std::shared_ptr<IMastClient> mast_client;
      std::shared_ptr<Nyx::Domain::ITargetRepository> target_repository;
      std::shared_ptr<Nyx::Domain::ITessObservationRepository>
        tess_observation_repository;
      std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator;
  };
} // namespace Nyx::Application::Target
