#include "application/target/LightCurveComparisonService.hpp"
#include "core/util/TimeConversion.hpp"

#include <algorithm>

namespace Nyx::Application::Target {
  LightCurveComparisonService::
    LightCurveComparisonService(
      std::shared_ptr<Nyx::Domain::ITargetRepository>
        target_repository,
      std::shared_ptr<
        Nyx::Domain::ITessObservationRepository
      > tess_observation_repository,
      std::shared_ptr<
        Nyx::Domain::ILightCurvePointRepository
      > light_curve_point_repository,
      std::shared_ptr<
        Nyx::Domain::IObservationSessionRepository
      > session_repository,
      std::shared_ptr<
        Nyx::Domain::IObservationImageRepository
      > image_repository
    )
    : target_repository(std::move(target_repository))
    , tess_observation_repository(
        std::move(tess_observation_repository)
      )
    , light_curve_point_repository(
        std::move(light_curve_point_repository)
      )
    , session_repository(
        std::move(session_repository)
      )
    , image_repository(
        std::move(image_repository)
      ) {
    spdlog::debug(
      "LightCurveComparisonService initialized"
    );
  }

  auto LightCurveComparisonService::get_comparison(
    const std::string& user_id,
    const std::string& target_id,
    const std::string& tess_observation_id,
    bool quality_filter,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<
    LightCurveComparisonResponse
  > {
    logger->debug(
      "Getting light curve comparison for "
      "target_id={}, tess_observation_id={}, "
      "user_id={}",
      target_id, tess_observation_id, user_id
    );

    auto target_result =
      this->target_repository->find_by_id(target_id);
    if (!target_result.has_value()) {
      return std::unexpected(target_result.error());
    }
    if (!target_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "Target not found"
        )
      );
    }
    auto target = target_result->value();

    auto tess_obs_result =
      this->tess_observation_repository->find_by_id(
        tess_observation_id
      );
    if (!tess_obs_result.has_value()) {
      return std::unexpected(tess_obs_result.error());
    }
    if (!tess_obs_result->has_value()) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "TESS observation not found"
        )
      );
    }
    auto tess_observation = tess_obs_result->value();

    if (tess_observation.target_id != target_id) {
      return std::unexpected(
        Nyx::Core::AppError::not_found(
          "TESS observation does not belong to "
          "this target"
        )
      );
    }

    auto points_result =
      this->light_curve_point_repository
        ->find_by_observation_id(
          tess_observation_id, quality_filter
        );
    if (!points_result.has_value()) {
      return std::unexpected(points_result.error());
    }
    auto tess_points = std::move(points_result.value());

    for (auto& point : tess_points) {
      point.time += Nyx::Core::TimeConversion::btjd_offset;
    }

    auto sessions_result =
      this->session_repository
        ->find_by_user_id_and_target_id(
          user_id, target_id
        );
    auto sessions = sessions_result.has_value()
      ? std::move(sessions_result.value())
      : std::vector<Nyx::Domain::ObservationSession>{};

    auto user_points = std::vector<UserObservationPoint>{};
    auto session_count = 0;

    for (const auto& session : sessions) {
      auto images_result =
        this->image_repository->find_by_session_id(
          session.id
        );
      if (!images_result.has_value()) continue;

      auto session_had_points = false;
      for (const auto& image : images_result.value()) {
        if (!image.photometry_status.has_value()
            || image.photometry_status.value()
                 != "completed") {
          continue;
        }
        if (!image.relative_flux.has_value()
            || !image.captured_at.has_value()) {
          continue;
        }

        auto jd = Nyx::Core::TimeConversion::iso8601_to_jd(
          image.captured_at.value()
        );
        if (!jd.has_value()) {
          logger->warn(
            "Failed to convert timestamp '{}' for "
            "image {}",
            image.captured_at.value(), image.id
          );
          continue;
        }

        user_points.push_back(UserObservationPoint{
          .time = jd.value(),
          .relative_flux =
            image.relative_flux.value(),
          .relative_flux_error =
            image.relative_flux_error.value_or(0.0),
          .session_id = session.id,
          .captured_at = image.captured_at.value(),
        });
        session_had_points = true;
      }

      if (session_had_points) session_count++;
    }

    std::sort(
      user_points.begin(), user_points.end(),
      [](const auto& a, const auto& b) {
        return a.time < b.time;
      }
    );

    auto target_response = TargetResponse{
      .id = target.id,
      .canonical_name = target.canonical_name,
      .target_type = target.target_type,
      .right_ascension = target.right_ascension,
      .declination = target.declination,
      .tess_observations = {},
    };

    logger->info(
      "Light curve comparison for target_id={}: "
      "{} TESS points, {} user points from {} sessions",
      target_id,
      tess_points.size(), user_points.size(),
      session_count
    );

    return LightCurveComparisonResponse{
      .target = std::move(target_response),
      .time_system = "bjd_tdb",
      .time_system_note =
        "TESS times converted from BTJD (+2457000). "
        "User times converted from UTC to JD "
        "(barycentric correction not applied, "
        "max error ~8 min).",
      .tess = TessComparisonData{
        .tess_observation_id = tess_observation_id,
        .obsid = tess_observation.obsid,
        .cadence_seconds =
          tess_observation.cadence_seconds,
        .point_count =
          static_cast<int>(tess_points.size()),
        .points = std::move(tess_points),
      },
      .user_observations = UserObservationData{
        .session_count = session_count,
        .point_count =
          static_cast<int>(user_points.size()),
        .points = std::move(user_points),
      },
    };
  }
} // namespace Nyx::Application::Target
