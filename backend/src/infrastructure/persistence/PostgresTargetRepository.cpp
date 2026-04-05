#include "infrastructure/persistence/PostgresTargetRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  static auto row_to_target(
    const drogon::orm::Row& row
  ) -> Nyx::Domain::Target {
    return Nyx::Domain::Target{
      .id = row["id"].as<std::string>(),
      .canonical_name = row["canonical_name"].as<std::string>(),
      .target_type = row["target_type"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["target_type"].as<std::string>()
          ),
      .right_ascension = row["right_ascension"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["right_ascension"].as<double>()
          ),
      .declination = row["declination"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["declination"].as<double>()
          ),
    };
  }

  auto PostgresTargetRepository::create(
    const Nyx::Domain::Target& target
  ) -> Nyx::Core::Result<Nyx::Domain::Target> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO targets "
        "(id, canonical_name, target_type, right_ascension, declination) "
        "VALUES ($1, $2, NULLIF($3, ''), "
        "CAST(NULLIF($4, '') AS DOUBLE PRECISION), "
        "CAST(NULLIF($5, '') AS DOUBLE PRECISION)) "
        "RETURNING id, canonical_name, target_type, "
        "right_ascension, declination",
        target.id, target.canonical_name,
        target.target_type.value_or(""),
        target.right_ascension.has_value()
          ? std::to_string(target.right_ascension.value()) : "",
        target.declination.has_value()
          ? std::to_string(target.declination.value()) : ""
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal("Failed to create target")
        );
      }

      return row_to_target(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error creating target: {}", exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Failed to create target")
      );
    }
  }

  auto PostgresTargetRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::Target>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, canonical_name, target_type, "
        "right_ascension, declination "
        "FROM targets WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      return row_to_target(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding target: {}", exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresTargetRepository::find_by_canonical_name(
    const std::string& canonical_name
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::Target>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, canonical_name, target_type, "
        "right_ascension, declination "
        "FROM targets WHERE canonical_name = $1",
        canonical_name
      );

      if (result.empty()) return std::nullopt;

      return row_to_target(result[0]);
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding target by name: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
