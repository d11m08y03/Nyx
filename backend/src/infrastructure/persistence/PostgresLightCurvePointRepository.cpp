#include "infrastructure/persistence/PostgresLightCurvePointRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace Nyx::Infrastructure::Persistence {
  static constexpr auto BATCH_SIZE = 5000;
  static constexpr auto PARAMS_PER_ROW = 6;

  static auto optional_float_to_string(
    const std::optional<float>& value
  ) -> std::string {
    return value.has_value()
      ? std::to_string(value.value()) : "";
  }

  auto PostgresLightCurvePointRepository::bulk_create(
    const std::string& observation_id,
    const std::vector<Nyx::Domain::LightCurvePoint>& points
  ) -> Nyx::Core::Result<int> {
    if (points.empty()) return 0;

    try {
      auto db = drogon::app().getDbClient();
      auto transaction = db->newTransaction();
      auto total_inserted = 0;

      for (auto batch_start = size_t{0};
        batch_start < points.size();
        batch_start += BATCH_SIZE) {
        auto batch_end = std::min(
          batch_start + BATCH_SIZE, points.size()
        );
        auto batch_size =
          static_cast<int>(batch_end - batch_start);

        auto sql = std::ostringstream{};
        sql << "INSERT INTO light_curve_points "
            << "(tess_observation_id, time, pdcsap_flux, "
            << "pdcsap_flux_err, sap_flux, quality) VALUES ";

        for (auto i = 0; i < batch_size; ++i) {
          if (i > 0) sql << ", ";
          auto offset = i * PARAMS_PER_ROW;
          sql << "($" << (offset + 1)
              << ", $" << (offset + 2)
              << ", CAST(NULLIF($" << (offset + 3)
              << ", '') AS REAL)"
              << ", CAST(NULLIF($" << (offset + 4)
              << ", '') AS REAL)"
              << ", CAST(NULLIF($" << (offset + 5)
              << ", '') AS REAL)"
              << ", $" << (offset + 6) << ")";
        }

        auto query = sql.str();
        auto binder = *transaction << query;

        for (auto i = batch_start; i < batch_end; ++i) {
          const auto& point = points[i];
          binder << observation_id;
          binder << std::to_string(point.time);
          binder << optional_float_to_string(point.pdcsap_flux);
          binder << optional_float_to_string(
            point.pdcsap_flux_err
          );
          binder << optional_float_to_string(point.sap_flux);
          binder << std::to_string(point.quality);
        }

        binder << drogon::orm::Mode::Blocking;
        binder >> [](const drogon::orm::Result&) {};
        binder.exec();

        total_inserted += batch_size;

        spdlog::debug(
          "Inserted batch of {} light curve points "
          "(total: {}/{})",
          batch_size, total_inserted, points.size()
        );
      }

      return total_inserted;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error bulk inserting light curve points: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to insert light curve points"
        )
      );
    }
  }

  auto PostgresLightCurvePointRepository::find_by_observation_id(
    const std::string& observation_id,
    bool quality_filter
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Domain::LightCurvePoint>
  > {
    try {
      auto db = drogon::app().getDbClient();

      auto query = std::string{
        "SELECT time, pdcsap_flux, pdcsap_flux_err, "
        "sap_flux, quality "
        "FROM light_curve_points "
        "WHERE tess_observation_id = $1"
      };

      if (quality_filter) {
        query += " AND quality = 0";
      }

      query += " ORDER BY time ASC";

      auto result = db->execSqlSync(query, observation_id);

      auto points = std::vector<Nyx::Domain::LightCurvePoint>{};
      points.reserve(result.size());
      // Columns by index: 0=time, 1=pdcsap_flux,
      // 2=pdcsap_flux_err, 3=sap_flux, 4=quality
      for (const auto& row : result) {
        points.push_back(Nyx::Domain::LightCurvePoint{
          .id = 0,
          .tess_observation_id = {},
          .time = row[0].as<double>(),
          .pdcsap_flux = row[1].isNull()
            ? std::nullopt
            : std::optional<float>(row[1].as<float>()),
          .pdcsap_flux_err = row[2].isNull()
            ? std::nullopt
            : std::optional<float>(row[2].as<float>()),
          .sap_flux = row[3].isNull()
            ? std::nullopt
            : std::optional<float>(row[3].as<float>()),
          .quality = row[4].as<int>(),
        });
      }
      return points;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error fetching light curve points: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresLightCurvePointRepository::find_by_observation_id_as_json(
    const std::string& observation_id,
    bool quality_filter
  ) -> Nyx::Core::Result<std::string> {
    try {
      auto db = drogon::app().getDbClient();

      auto query = std::string{
        "SELECT COALESCE(json_agg("
        "json_build_object("
        "'time', time, "
        "'pdcsap_flux', pdcsap_flux, "
        "'pdcsap_flux_err', pdcsap_flux_err, "
        "'sap_flux', sap_flux, "
        "'quality', quality"
        ") ORDER BY time ASC), '[]'::json)::text "
        "FROM light_curve_points "
        "WHERE tess_observation_id = $1"
      };

      if (quality_filter) {
        query += " AND quality = 0";
      }

      auto result = db->execSqlSync(
        query, observation_id
      );

      return result[0][0].as<std::string>();
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error fetching light curve JSON: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresLightCurvePointRepository::count_by_observation_id(
    const std::string& observation_id
  ) -> Nyx::Core::Result<int> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT COUNT(*)::int AS count "
        "FROM light_curve_points "
        "WHERE tess_observation_id = $1",
        observation_id
      );

      if (result.empty()) return 0;

      return result[0]["count"].as<int>();
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error counting light curve points: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresLightCurvePointRepository::delete_by_observation_id(
    const std::string& observation_id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      db->execSqlSync(
        "DELETE FROM light_curve_points "
        "WHERE tess_observation_id = $1",
        observation_id
      );
      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error deleting light curve points: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
