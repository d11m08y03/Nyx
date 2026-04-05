#include "application/target/TargetService.hpp"

namespace Nyx::Application::Target {
  TargetService::TargetService(
    std::shared_ptr<IMastClient> mast_client,
    std::shared_ptr<Nyx::Domain::ITargetRepository> target_repository,
    std::shared_ptr<Nyx::Domain::ITessObservationRepository>
      tess_observation_repository,
    std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
  )
    : mast_client(std::move(mast_client))
    , target_repository(std::move(target_repository))
    , tess_observation_repository(std::move(tess_observation_repository))
    , uuid_generator(std::move(uuid_generator)) {
    spdlog::debug("TargetService initialized");
  }

  auto TargetService::build_observation_responses(
    const std::string& target_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<TessObservationResponse>> {
    auto obs_result =
      this->tess_observation_repository->find_by_target_id(target_id);

    if (!obs_result.has_value()) {
      logger->error(
        "Failed to fetch TESS observations for target_id={}",
        target_id
      );
      return std::unexpected(obs_result.error());
    }

    auto responses = std::vector<TessObservationResponse>{};
    for (const auto& obs : obs_result.value()) {
      responses.push_back(TessObservationResponse{
        .id = obs.id,
        .obsid = obs.obsid,
        .cadence_seconds = obs.cadence_seconds,
        .start_time = obs.start_time,
        .end_time = obs.end_time,
        .data_uri = obs.data_uri,
      });
    }
    return responses;
  }

  auto TargetService::resolve_target(
    const ResolveTargetRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TargetResponse> {
    logger->debug(
      "Resolving target name='{}'", request.target_name
    );

    auto resolved = this->mast_client->resolve_target(
      request.target_name
    );

    if (!resolved.has_value()) {
      logger->warn(
        "MAST name resolution failed for '{}'",
        request.target_name
      );
      return std::unexpected(resolved.error());
    }

    auto cached = this->target_repository->find_by_canonical_name(
      resolved->canonical_name
    );

    if (!cached.has_value()) {
      return std::unexpected(cached.error());
    }

    if (cached->has_value()) {
      auto existing = cached->value();
      logger->info(
        "Target '{}' already cached as id={}",
        existing.canonical_name, existing.id
      );

      auto observations = this->build_observation_responses(
        existing.id, logger
      );
      if (!observations.has_value()) {
        return std::unexpected(observations.error());
      }

      return TargetResponse{
        .id = existing.id,
        .canonical_name = existing.canonical_name,
        .target_type = existing.target_type,
        .right_ascension = existing.right_ascension,
        .declination = existing.declination,
        .tess_observations = observations.value(),
      };
    }

    auto target = Nyx::Domain::Target{
      .id = this->uuid_generator->generate(),
      .canonical_name = resolved->canonical_name,
      .target_type = resolved->target_type,
      .right_ascension = resolved->right_ascension,
      .declination = resolved->declination,
    };

    auto create_result = this->target_repository->create(target);
    if (!create_result.has_value()) {
      logger->error(
        "Failed to persist target '{}'", target.canonical_name
      );
      return std::unexpected(create_result.error());
    }

    auto created = create_result.value();
    logger->info(
      "Target created id={}, canonical_name='{}'",
      created.id, created.canonical_name
    );

    auto tess_observations = std::vector<TessObservationResponse>{};

    if (created.right_ascension.has_value()
      && created.declination.has_value()) {
      auto tess_result = this->mast_client->search_tess_timeseries(
        created.right_ascension.value(),
        created.declination.value()
      );

      if (!tess_result.has_value()) {
        logger->error(
          "TESS search failed for target id={}", created.id
        );
        return std::unexpected(tess_result.error());
      }

      for (const auto& mast_obs : tess_result.value()) {
        auto existing_obs =
          this->tess_observation_repository->find_by_obsid(
            mast_obs.obsid
          );

        if (existing_obs.has_value()
          && existing_obs->has_value()) {
          logger->debug(
            "TESS observation obsid={} already exists, skipping",
            mast_obs.obsid
          );
          auto obs = existing_obs->value();
          tess_observations.push_back(TessObservationResponse{
            .id = obs.id,
            .obsid = obs.obsid,
            .cadence_seconds = obs.cadence_seconds,
            .start_time = obs.start_time,
            .end_time = obs.end_time,
            .data_uri = obs.data_uri,
          });
          continue;
        }

        auto observation = Nyx::Domain::TessObservation{
          .id = this->uuid_generator->generate(),
          .target_id = created.id,
          .obsid = mast_obs.obsid,
          .cadence_seconds = mast_obs.cadence_seconds,
          .start_time = mast_obs.start_time,
          .end_time = mast_obs.end_time,
          .data_uri = std::nullopt,
        };

        auto obs_create_result =
          this->tess_observation_repository->create(observation);

        if (!obs_create_result.has_value()) {
          logger->warn(
            "Failed to persist TESS observation obsid={}",
            mast_obs.obsid
          );
          continue;
        }

        auto created_obs = obs_create_result.value();
        tess_observations.push_back(TessObservationResponse{
          .id = created_obs.id,
          .obsid = created_obs.obsid,
          .cadence_seconds = created_obs.cadence_seconds,
          .start_time = created_obs.start_time,
          .end_time = created_obs.end_time,
          .data_uri = created_obs.data_uri,
        });
      }

      logger->info(
        "Stored {} TESS observations for target id={}",
        tess_observations.size(), created.id
      );
    }

    return TargetResponse{
      .id = created.id,
      .canonical_name = created.canonical_name,
      .target_type = created.target_type,
      .right_ascension = created.right_ascension,
      .declination = created.declination,
      .tess_observations = tess_observations,
    };
  }

  auto TargetService::get_target(
    const std::string& id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TargetResponse> {
    logger->debug("Getting target id={}", id);

    auto result = this->target_repository->find_by_id(id);

    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    if (!result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Target not found")
      );
    }

    auto target = result->value();

    auto observations = this->build_observation_responses(
      target.id, logger
    );
    if (!observations.has_value()) {
      return std::unexpected(observations.error());
    }

    return TargetResponse{
      .id = target.id,
      .canonical_name = target.canonical_name,
      .target_type = target.target_type,
      .right_ascension = target.right_ascension,
      .declination = target.declination,
      .tess_observations = observations.value(),
    };
  }

  auto TargetService::list_tess_observations(
    const std::string& target_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<std::vector<TessObservationResponse>> {
    logger->debug(
      "Listing TESS observations for target_id={}", target_id
    );

    auto target_result =
      this->target_repository->find_by_id(target_id);

    if (!target_result.has_value()) {
      return std::unexpected(target_result.error());
    }

    if (!target_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found("Target not found")
      );
    }

    return this->build_observation_responses(target_id, logger);
  }
} // namespace Nyx::Application::Target
