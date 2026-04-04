#include "infrastructure/persistence/PostgresFilterRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresFilterRepository::create(
    const Nyx::Domain::Filter& filter
  ) -> Nyx::Core::Result<Nyx::Domain::Filter> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO filters (id, user_id, name, band, bandwidth_nm, brand, model) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7) "
        "RETURNING id, user_id, name, band, bandwidth_nm, brand, model",
        filter.id, filter.user_id, filter.name, filter.band,
        filter.bandwidth_nm, filter.brand, filter.model
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::internal("Failed to create filter"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Filter{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .band = row["band"].as<std::string>(),
        .bandwidth_nm = row["bandwidth_nm"].isNull() ? 0.0 : row["bandwidth_nm"].as<double>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error creating filter: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to create filter"));
    }
  }

  auto PostgresFilterRepository::find_by_user_id(
    const std::string& user_id
  ) -> Nyx::Core::Result<std::vector<Nyx::Domain::Filter>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, band, bandwidth_nm, brand, model "
        "FROM filters WHERE user_id = $1 ORDER BY created_at DESC",
        user_id
      );

      auto filters = std::vector<Nyx::Domain::Filter>{};
      for (const auto& row : result) {
        filters.push_back(Nyx::Domain::Filter{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .band = row["band"].as<std::string>(),
          .bandwidth_nm = row["bandwidth_nm"].isNull() ? 0.0 : row["bandwidth_nm"].as<double>(),
          .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
          .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
        });
      }
      return filters;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error listing filters: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresFilterRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::Filter>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, band, bandwidth_nm, brand, model "
        "FROM filters WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      const auto& row = result[0];
      return Nyx::Domain::Filter{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .band = row["band"].as<std::string>(),
        .bandwidth_nm = row["bandwidth_nm"].isNull() ? 0.0 : row["bandwidth_nm"].as<double>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error finding filter: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresFilterRepository::update(
    const Nyx::Domain::Filter& filter
  ) -> Nyx::Core::Result<Nyx::Domain::Filter> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE filters SET name = $1, band = $2, bandwidth_nm = $3, brand = $4, "
        "model = $5, updated_at = NOW() WHERE id = $6 "
        "RETURNING id, user_id, name, band, bandwidth_nm, brand, model",
        filter.name, filter.band, filter.bandwidth_nm,
        filter.brand, filter.model, filter.id
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::not_found("Filter not found"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Filter{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .band = row["band"].as<std::string>(),
        .bandwidth_nm = row["bandwidth_nm"].isNull() ? 0.0 : row["bandwidth_nm"].as<double>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error updating filter: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to update filter"));
    }
  }

  auto PostgresFilterRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync("DELETE FROM filters WHERE id = $1", id);
      if (result.affectedRows() == 0) {
        return std::unexpected(Nyx::Core::AppError::not_found("Filter not found"));
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error deleting filter: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to delete filter"));
    }
  }
} // namespace Nyx::Infrastructure::Persistence
