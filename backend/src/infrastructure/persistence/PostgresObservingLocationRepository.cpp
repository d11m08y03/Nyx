#include "infrastructure/persistence/PostgresObservingLocationRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresObservingLocationRepository::create(
    const Nyx::Domain::ObservingLocation& location
  ) -> Nyx::Core::Result<Nyx::Domain::ObservingLocation> {
    try {
      auto db = drogon::app().getDbClient();

      if (location.bortle_class.has_value()) {
        auto result = db->execSqlSync(
          "INSERT INTO observing_locations (id, user_id, name, latitude, longitude, "
          "bortle_class) VALUES ($1, $2, $3, $4, $5, $6) "
          "RETURNING id, user_id, name, latitude, longitude, bortle_class",
          location.id, location.user_id, location.name,
          location.latitude, location.longitude, location.bortle_class.value()
        );

        if (result.empty()) {
          return std::unexpected(
            Nyx::Core::AppError::internal("Failed to create observing location")
          );
        }

        const auto& row = result[0];
        return Nyx::Domain::ObservingLocation{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .latitude = row["latitude"].as<double>(),
          .longitude = row["longitude"].as<double>(),
          .bortle_class = row["bortle_class"].isNull()
            ? std::nullopt
            : std::optional<int16_t>(row["bortle_class"].as<int16_t>()),
        };
      }

      auto result = db->execSqlSync(
        "INSERT INTO observing_locations (id, user_id, name, latitude, longitude) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id, user_id, name, latitude, longitude, bortle_class",
        location.id, location.user_id, location.name,
        location.latitude, location.longitude
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal("Failed to create observing location")
        );
      }

      const auto& row = result[0];
      return Nyx::Domain::ObservingLocation{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .latitude = row["latitude"].as<double>(),
        .longitude = row["longitude"].as<double>(),
        .bortle_class = std::nullopt,
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error creating observing location: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create observing location"
        )
      );
    }
  }

  auto PostgresObservingLocationRepository::find_by_user_id(
    const std::string& user_id
  ) -> Nyx::Core::Result<std::vector<Nyx::Domain::ObservingLocation>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, latitude, longitude, bortle_class "
        "FROM observing_locations WHERE user_id = $1 ORDER BY created_at DESC",
        user_id
      );

      auto locations = std::vector<Nyx::Domain::ObservingLocation>{};
      for (const auto& row : result) {
        locations.push_back(Nyx::Domain::ObservingLocation{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .latitude = row["latitude"].as<double>(),
          .longitude = row["longitude"].as<double>(),
          .bortle_class = row["bortle_class"].isNull()
            ? std::nullopt
            : std::optional<int16_t>(row["bortle_class"].as<int16_t>()),
        });
      }
      return locations;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error listing observing locations: {}", exception.base().what()
      );
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresObservingLocationRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, latitude, longitude, bortle_class "
        "FROM observing_locations WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      const auto& row = result[0];
      return Nyx::Domain::ObservingLocation{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .latitude = row["latitude"].as<double>(),
        .longitude = row["longitude"].as<double>(),
        .bortle_class = row["bortle_class"].isNull()
          ? std::nullopt
          : std::optional<int16_t>(row["bortle_class"].as<int16_t>()),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding observing location: {}", exception.base().what()
      );
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresObservingLocationRepository::find_by_user_id_and_name(
    const std::string& user_id, const std::string& name
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, latitude, longitude, bortle_class "
        "FROM observing_locations WHERE user_id = $1 AND name = $2",
        user_id, name
      );

      if (result.empty()) return std::nullopt;

      const auto& row = result[0];
      return Nyx::Domain::ObservingLocation{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .latitude = row["latitude"].as<double>(),
        .longitude = row["longitude"].as<double>(),
        .bortle_class = row["bortle_class"].isNull()
          ? std::nullopt
          : std::optional<int16_t>(row["bortle_class"].as<int16_t>()),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding observing location by name: {}",
        exception.base().what()
      );
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresObservingLocationRepository::update(
    const Nyx::Domain::ObservingLocation& location
  ) -> Nyx::Core::Result<Nyx::Domain::ObservingLocation> {
    try {
      auto db = drogon::app().getDbClient();

      if (location.bortle_class.has_value()) {
        auto result = db->execSqlSync(
          "UPDATE observing_locations SET name = $1, latitude = $2, longitude = $3, "
          "bortle_class = $4, updated_at = NOW() WHERE id = $5 "
          "RETURNING id, user_id, name, latitude, longitude, bortle_class",
          location.name, location.latitude, location.longitude,
          location.bortle_class.value(), location.id
        );

        if (result.empty()) {
          return std::unexpected(
            Nyx::Core::AppError::not_found("Observing location not found")
          );
        }

        const auto& row = result[0];
        return Nyx::Domain::ObservingLocation{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .latitude = row["latitude"].as<double>(),
          .longitude = row["longitude"].as<double>(),
          .bortle_class = row["bortle_class"].isNull()
            ? std::nullopt
            : std::optional<int16_t>(row["bortle_class"].as<int16_t>()),
        };
      }

      auto result = db->execSqlSync(
        "UPDATE observing_locations SET name = $1, latitude = $2, longitude = $3, "
        "bortle_class = NULL, updated_at = NOW() WHERE id = $4 "
        "RETURNING id, user_id, name, latitude, longitude, bortle_class",
        location.name, location.latitude, location.longitude, location.id
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::not_found("Observing location not found")
        );
      }

      const auto& row = result[0];
      return Nyx::Domain::ObservingLocation{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .latitude = row["latitude"].as<double>(),
        .longitude = row["longitude"].as<double>(),
        .bortle_class = std::nullopt,
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error updating observing location: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to update observing location"
        )
      );
    }
  }

  auto PostgresObservingLocationRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync("DELETE FROM observing_locations WHERE id = $1", id);
      if (result.affectedRows() == 0) {
        return std::unexpected(
          Nyx::Core::AppError::not_found("Observing location not found")
        );
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error deleting observing location: {}", exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Failed to delete observing location")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
