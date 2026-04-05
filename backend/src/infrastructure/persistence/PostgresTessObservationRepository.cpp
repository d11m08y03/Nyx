#include "infrastructure/persistence/PostgresTessObservationRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  static auto row_to_tess_observation(
    const drogon::orm::Row& row
  ) -> Nyx::Domain::TessObservation {
    return Nyx::Domain::TessObservation{
      .id = row["id"].as<std::string>(),
      .target_id = row["target_id"].as<std::string>(),
      .obsid = row["obsid"].as<std::string>(),
      .cadence_seconds = row["cadence_seconds"].as<int>(),
      .start_time = row["start_time"].as<double>(),
      .end_time = row["end_time"].as<double>(),
      .data_uri = row["data_uri"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["data_uri"].as<std::string>()
          ),
    };
  }

  auto PostgresTessObservationRepository::create(
    const Nyx::Domain::TessObservation& observation
  ) -> Nyx::Core::Result<Nyx::Domain::TessObservation> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO tess_observations "
        "(id, target_id, obsid, cadence_seconds, "
        "start_time, end_time, data_uri) "
        "VALUES ($1, $2, $3, $4, $5, $6, NULLIF($7, '')) "
        "RETURNING id, target_id, obsid, cadence_seconds, "
        "start_time, end_time, data_uri",
        observation.id, observation.target_id,
        observation.obsid, observation.cadence_seconds,
        observation.start_time, observation.end_time,
        observation.data_uri.value_or("")
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal(
            "Failed to create TESS observation"
          )
        );
      }

      return row_to_tess_observation(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error creating TESS observation: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create TESS observation"
        )
      );
    }
  }

  auto PostgresTessObservationRepository::find_by_target_id(
    const std::string& target_id
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Domain::TessObservation>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, target_id, obsid, cadence_seconds, "
        "start_time, end_time, data_uri "
        "FROM tess_observations WHERE target_id = $1 "
        "ORDER BY start_time ASC",
        target_id
      );

      auto observations =
        std::vector<Nyx::Domain::TessObservation>{};
      for (const auto& row : result) {
        observations.push_back(row_to_tess_observation(row));
      }
      return observations;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error listing TESS observations: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresTessObservationRepository::find_by_obsid(
    const std::string& obsid
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::TessObservation>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, target_id, obsid, cadence_seconds, "
        "start_time, end_time, data_uri "
        "FROM tess_observations WHERE obsid = $1",
        obsid
      );

      if (result.empty()) return std::nullopt;

      return row_to_tess_observation(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding TESS observation: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
