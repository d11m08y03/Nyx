#include "application/target/TargetService.hpp"

#include <chrono>

namespace Nyx::Application::Target {
  TargetService::TargetService(
    std::shared_ptr<IMastClient> mast_client,
    std::shared_ptr<Nyx::Domain::ITargetRepository> target_repository,
    std::shared_ptr<Nyx::Domain::ITessObservationRepository>
      tess_observation_repository,
    std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator,
    std::shared_ptr<Nyx::Domain::ILightCurvePointRepository>
      light_curve_point_repository,
    std::shared_ptr<IFitsParser> fits_parser
  )
    : mast_client(std::move(mast_client))
    , target_repository(std::move(target_repository))
    , tess_observation_repository(std::move(tess_observation_repository))
    , uuid_generator(std::move(uuid_generator))
    , light_curve_point_repository(std::move(light_curve_point_repository))
    , fits_parser(std::move(fits_parser)) {
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

      constexpr auto max_single_sector_days = 30.0;

      auto single_sector_obs =
        std::vector<MastObservation>{};
      for (const auto& mast_obs : tess_result.value()) {
        auto span_days =
          mast_obs.end_time - mast_obs.start_time;
        if (span_days > max_single_sector_days) {
          logger->debug(
            "Skipping multi-sector observation obsid={} "
            "(span={:.1f} days)",
            mast_obs.obsid, span_days
          );
          continue;
        }
        single_sector_obs.push_back(mast_obs);
      }

      auto obsids = std::vector<std::string>{};
      obsids.reserve(single_sector_obs.size());
      for (const auto& obs : single_sector_obs) {
        obsids.push_back(obs.obsid);
      }

      auto existing_result =
        this->tess_observation_repository
          ->find_existing_obsids(obsids);

      if (!existing_result.has_value()) {
        logger->error(
          "Failed to check existing obsids for target "
          "id={}",
          created.id
        );
        return std::unexpected(existing_result.error());
      }

      auto& existing_obsids = existing_result.value();

      auto new_observations =
        std::vector<Nyx::Domain::TessObservation>{};
      for (const auto& mast_obs : single_sector_obs) {
        if (existing_obsids.contains(mast_obs.obsid)) {
          logger->debug(
            "TESS observation obsid={} already exists, "
            "skipping",
            mast_obs.obsid
          );
          continue;
        }

        new_observations.push_back(
          Nyx::Domain::TessObservation{
            .id = this->uuid_generator->generate(),
            .target_id = created.id,
            .obsid = mast_obs.obsid,
            .cadence_seconds = mast_obs.cadence_seconds,
            .start_time = mast_obs.start_time,
            .end_time = mast_obs.end_time,
            .data_uri = std::nullopt,
          }
        );
      }

      if (!new_observations.empty()) {
        auto bulk_result =
          this->tess_observation_repository->bulk_create(
            new_observations
          );

        if (!bulk_result.has_value()) {
          logger->error(
            "Failed to bulk create TESS observations "
            "for target id={}",
            created.id
          );
          return std::unexpected(bulk_result.error());
        }

        for (const auto& obs : bulk_result.value()) {
          tess_observations.push_back(
            TessObservationResponse{
              .id = obs.id,
              .obsid = obs.obsid,
              .cadence_seconds = obs.cadence_seconds,
              .start_time = obs.start_time,
              .end_time = obs.end_time,
              .data_uri = obs.data_uri,
            }
          );
        }
      }

      auto existing_obs_result =
        this->tess_observation_repository
          ->find_by_target_id(created.id);
      if (existing_obs_result.has_value()) {
        for (const auto& obs :
          existing_obs_result.value()) {
          if (!existing_obsids.contains(obs.obsid)) {
            continue;
          }
          tess_observations.push_back(
            TessObservationResponse{
              .id = obs.id,
              .obsid = obs.obsid,
              .cadence_seconds = obs.cadence_seconds,
              .start_time = obs.start_time,
              .end_time = obs.end_time,
              .data_uri = obs.data_uri,
            }
          );
        }
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

  auto TargetService::discover_products(
    const std::string& observation_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TessObservationResponse> {
    logger->debug(
      "Discovering products for observation_id={}",
      observation_id
    );

    auto obs_result =
      this->tess_observation_repository->find_by_id(observation_id);

    if (!obs_result.has_value()) {
      return std::unexpected(obs_result.error());
    }

    if (!obs_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "TESS observation not found"
        )
      );
    }

    auto observation = obs_result->value();

    if (observation.data_uri.has_value()) {
      logger->info(
        "Observation {} already has data_uri, skipping discovery",
        observation_id
      );
      return TessObservationResponse{
        .id = observation.id,
        .obsid = observation.obsid,
        .cadence_seconds = observation.cadence_seconds,
        .start_time = observation.start_time,
        .end_time = observation.end_time,
        .data_uri = observation.data_uri,
      };
    }

    auto products_result = this->mast_client->get_data_products(
      observation.obsid
    );

    if (!products_result.has_value()) {
      logger->error(
        "Failed to get data products for obsid={}",
        observation.obsid
      );
      return std::unexpected(products_result.error());
    }

    auto lc_data_uri = std::string{};

    for (const auto& product : products_result.value()) {
      if (product.description == "Light curves"
        && product.product_type == "SCIENCE"
        && product.filename.ends_with("_lc.fits")) {
        lc_data_uri = product.data_uri;
        break;
      }
    }

    if (lc_data_uri.empty()) {
      logger->warn(
        "No light curve FITS file found for obsid={}",
        observation.obsid
      );
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "No light curve FITS file found in data products"
        )
      );
    }

    logger->info(
      "Found light curve data_uri='{}' for obsid={}",
      lc_data_uri, observation.obsid
    );

    auto update_result =
      this->tess_observation_repository->update_data_uri(
        observation_id, lc_data_uri
      );

    if (!update_result.has_value()) {
      return std::unexpected(update_result.error());
    }

    auto updated = update_result.value();

    return TessObservationResponse{
      .id = updated.id,
      .obsid = updated.obsid,
      .cadence_seconds = updated.cadence_seconds,
      .start_time = updated.start_time,
      .end_time = updated.end_time,
      .data_uri = updated.data_uri,
    };
  }

  auto TargetService::fetch_light_curve(
    const std::string& observation_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<LightCurveMetadataResponse> {
    logger->debug(
      "Fetching light curve for observation_id={}",
      observation_id
    );

    auto obs_result =
      this->tess_observation_repository->find_by_id(observation_id);

    if (!obs_result.has_value()) {
      return std::unexpected(obs_result.error());
    }

    if (!obs_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "TESS observation not found"
        )
      );
    }

    auto observation = obs_result->value();

    if (!observation.data_uri.has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::validation(
          "Observation has no data_uri. "
          "Call discover-products first."
        )
      );
    }

    auto count_result =
      this->light_curve_point_repository->count_by_observation_id(
        observation_id
      );

    if (!count_result.has_value()) {
      return std::unexpected(count_result.error());
    }

    if (count_result.value() > 0) {
      logger->info(
        "Light curve already fetched for observation_id={} "
        "({} points)",
        observation_id, count_result.value()
      );

      auto points_result =
        this->light_curve_point_repository->find_by_observation_id(
          observation_id, false
        );

      if (!points_result.has_value()) {
        return std::unexpected(points_result.error());
      }

      auto time_min = 0.0;
      auto time_max = 0.0;
      if (!points_result->empty()) {
        time_min = points_result->front().time;
        time_max = points_result->back().time;
      }

      return LightCurveMetadataResponse{
        .tess_observation_id = observation_id,
        .obsid = observation.obsid,
        .point_count = count_result.value(),
        .time_min = time_min,
        .time_max = time_max,
      };
    }

    logger->info(
      "Downloading FITS file from '{}'",
      observation.data_uri.value()
    );

    auto download_result = this->mast_client->download_fits(
      observation.data_uri.value()
    );

    if (!download_result.has_value()) {
      logger->error(
        "Failed to download FITS file for observation_id={}",
        observation_id
      );
      return std::unexpected(download_result.error());
    }

    logger->info(
      "Parsing FITS data ({} bytes)",
      download_result->size()
    );

    auto parse_result = this->fits_parser->parse_light_curve(
      download_result.value()
    );

    if (!parse_result.has_value()) {
      logger->error(
        "Failed to parse FITS file for observation_id={}",
        observation_id
      );
      return std::unexpected(parse_result.error());
    }

    auto& points = parse_result.value();

    logger->info(
      "Inserting {} light curve points for observation_id={}",
      points.size(), observation_id
    );

    auto insert_result =
      this->light_curve_point_repository->bulk_create(
        observation_id, points
      );

    if (!insert_result.has_value()) {
      return std::unexpected(insert_result.error());
    }

    auto time_min = 0.0;
    auto time_max = 0.0;
    if (!points.empty()) {
      time_min = points.front().time;
      time_max = points.back().time;
    }

    return LightCurveMetadataResponse{
      .tess_observation_id = observation_id,
      .obsid = observation.obsid,
      .point_count = insert_result.value(),
      .time_min = time_min,
      .time_max = time_max,
    };
  }

  auto TargetService::get_light_curve(
    const std::string& observation_id,
    bool quality_filter,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<LightCurveResponse> {
    logger->debug(
      "Getting light curve for observation_id={}, "
      "quality_filter={}",
      observation_id, quality_filter
    );

    auto obs_result =
      this->tess_observation_repository->find_by_id(observation_id);

    if (!obs_result.has_value()) {
      return std::unexpected(obs_result.error());
    }

    if (!obs_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "TESS observation not found"
        )
      );
    }

    auto observation = obs_result->value();

    auto db_start = std::chrono::steady_clock::now();

    auto points_result =
      this->light_curve_point_repository->find_by_observation_id(
        observation_id, quality_filter
      );

    auto db_elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - db_start
      ).count();
    logger->debug(
      "Light curve DB fetch took {}ms ({} points)",
      db_elapsed,
      points_result.has_value() ? points_result->size() : 0
    );

    if (!points_result.has_value()) {
      return std::unexpected(points_result.error());
    }

    return LightCurveResponse{
      .tess_observation_id = observation_id,
      .obsid = observation.obsid,
      .point_count =
        static_cast<int>(points_result->size()),
      .points = std::move(points_result.value()),
    };
  }
  auto TargetService::get_light_curve_json(
    const std::string& observation_id,
    bool quality_filter,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<LightCurveJsonResponse> {
    logger->debug(
      "Getting light curve JSON for observation_id={}, "
      "quality_filter={}",
      observation_id, quality_filter
    );

    auto obs_result =
      this->tess_observation_repository->find_by_id(
        observation_id
      );

    if (!obs_result.has_value()) {
      return std::unexpected(obs_result.error());
    }

    if (!obs_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "TESS observation not found"
        )
      );
    }

    auto observation = obs_result->value();

    auto count_result =
      this->light_curve_point_repository
        ->count_by_observation_id(observation_id);

    if (!count_result.has_value()) {
      return std::unexpected(count_result.error());
    }

    auto db_start = std::chrono::steady_clock::now();

    auto json_result =
      this->light_curve_point_repository
        ->find_by_observation_id_as_json(
          observation_id, quality_filter
        );

    auto db_elapsed =
      std::chrono::duration_cast<
        std::chrono::milliseconds
      >(
        std::chrono::steady_clock::now() - db_start
      ).count();
    logger->debug(
      "Light curve JSON DB fetch took {}ms",
      db_elapsed
    );

    if (!json_result.has_value()) {
      return std::unexpected(json_result.error());
    }

    return LightCurveJsonResponse{
      .tess_observation_id = observation_id,
      .obsid = observation.obsid,
      .point_count = count_result.value(),
      .points_json = std::move(json_result.value()),
    };
  }
} // namespace Nyx::Application::Target
