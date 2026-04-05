#include "infrastructure/persistence/PostgresTessObservationRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <sstream>

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
  auto PostgresTessObservationRepository::find_existing_obsids(
    const std::vector<std::string>& obsids
  ) -> Nyx::Core::Result<
    std::unordered_set<std::string>
  > {
    if (obsids.empty()) return std::unordered_set<std::string>{};

    try {
      auto db = drogon::app().getDbClient();

      auto sql = std::ostringstream{};
      sql << "SELECT obsid FROM tess_observations "
          << "WHERE obsid = ANY(ARRAY[";
      for (auto i = size_t{0}; i < obsids.size(); ++i) {
        if (i > 0) sql << ',';
        sql << '$' << (i + 1);
      }
      sql << "]::text[])";

      auto query = sql.str();
      auto binder = *db << query;
      for (const auto& obsid : obsids) {
        binder << obsid;
      }

      binder << drogon::orm::Mode::Blocking;
      auto result = std::optional<drogon::orm::Result>{};
      binder >> [&result](const drogon::orm::Result& r) {
        result = r;
      };
      binder.exec();

      auto existing = std::unordered_set<std::string>{};
      for (const auto& row : *result) {
        existing.insert(row["obsid"].as<std::string>());
      }
      return existing;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding existing obsids: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresTessObservationRepository::bulk_create(
    const std::vector<Nyx::Domain::TessObservation>& observations
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Domain::TessObservation>
  > {
    if (observations.empty()) {
      return std::vector<Nyx::Domain::TessObservation>{};
    }

    try {
      auto db = drogon::app().getDbClient();
      constexpr auto params_per_row = 7;

      auto sql = std::ostringstream{};
      sql << "INSERT INTO tess_observations "
          << "(id, target_id, obsid, cadence_seconds, "
          << "start_time, end_time, data_uri) VALUES ";

      for (auto i = size_t{0}; i < observations.size();
        ++i) {
        if (i > 0) sql << ", ";
        auto offset =
          static_cast<int>(i) * params_per_row;
        sql << "($" << (offset + 1)
            << ", $" << (offset + 2)
            << ", $" << (offset + 3)
            << ", $" << (offset + 4)
            << ", $" << (offset + 5)
            << ", $" << (offset + 6)
            << ", NULLIF($" << (offset + 7) << ", ''))";
      }

      sql << " RETURNING id, target_id, obsid, "
          << "cadence_seconds, start_time, end_time, "
          << "data_uri";

      auto query = sql.str();
      auto binder = *db << query;

      for (const auto& obs : observations) {
        binder << obs.id;
        binder << obs.target_id;
        binder << obs.obsid;
        binder << obs.cadence_seconds;
        binder << obs.start_time;
        binder << obs.end_time;
        binder << obs.data_uri.value_or("");
      }

      binder << drogon::orm::Mode::Blocking;
      auto result = std::optional<drogon::orm::Result>{};
      binder >> [&result](const drogon::orm::Result& r) {
        result = r;
      };
      binder.exec();

      auto created =
        std::vector<Nyx::Domain::TessObservation>{};
      created.reserve(result->size());
      for (const auto& row : *result) {
        created.push_back(row_to_tess_observation(row));
      }
      return created;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error bulk creating TESS observations: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to bulk create TESS observations"
        )
      );
    }
  }

  auto PostgresTessObservationRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::TessObservation>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, target_id, obsid, cadence_seconds, "
        "start_time, end_time, data_uri "
        "FROM tess_observations WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      return row_to_tess_observation(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding TESS observation by id: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresTessObservationRepository::update_data_uri(
    const std::string& id, const std::string& data_uri
  ) -> Nyx::Core::Result<Nyx::Domain::TessObservation> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE tess_observations SET data_uri = $1, "
        "updated_at = NOW() WHERE id = $2 "
        "RETURNING id, target_id, obsid, cadence_seconds, "
        "start_time, end_time, data_uri",
        data_uri, id
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "TESS observation not found"
          )
        );
      }

      return row_to_tess_observation(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error updating TESS observation data_uri: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
