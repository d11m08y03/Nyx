#include "infrastructure/persistence/PostgresTelescopeRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresTelescopeRepository::create(
    const Nyx::Domain::Telescope& telescope
  ) -> Nyx::Core::Result<Nyx::Domain::Telescope> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO telescopes (id, user_id, name, aperture_mm, focal_length_mm, "
        "optical_design, brand, model) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
        "RETURNING id, user_id, name, aperture_mm, focal_length_mm, "
        "optical_design, brand, model",
        telescope.id, telescope.user_id, telescope.name,
        telescope.aperture_mm, telescope.focal_length_mm,
        telescope.optical_design, telescope.brand, telescope.model
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::internal("Failed to create telescope"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Telescope{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .aperture_mm = row["aperture_mm"].as<int>(),
        .focal_length_mm = row["focal_length_mm"].as<int>(),
        .optical_design = row["optical_design"].as<std::string>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error creating telescope: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to create telescope"));
    }
  }

  auto PostgresTelescopeRepository::find_by_user_id(
    const std::string& user_id
  ) -> Nyx::Core::Result<std::vector<Nyx::Domain::Telescope>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, aperture_mm, focal_length_mm, "
        "optical_design, brand, model FROM telescopes "
        "WHERE user_id = $1 ORDER BY created_at DESC",
        user_id
      );

      auto telescopes = std::vector<Nyx::Domain::Telescope>{};
      for (const auto& row : result) {
        telescopes.push_back(Nyx::Domain::Telescope{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .aperture_mm = row["aperture_mm"].as<int>(),
          .focal_length_mm = row["focal_length_mm"].as<int>(),
          .optical_design = row["optical_design"].as<std::string>(),
          .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
          .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
        });
      }
      return telescopes;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error listing telescopes: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresTelescopeRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::Telescope>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, aperture_mm, focal_length_mm, "
        "optical_design, brand, model FROM telescopes WHERE id = $1",
        id
      );

      if (result.empty()) {
        return std::nullopt;
      }

      const auto& row = result[0];
      return Nyx::Domain::Telescope{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .aperture_mm = row["aperture_mm"].as<int>(),
        .focal_length_mm = row["focal_length_mm"].as<int>(),
        .optical_design = row["optical_design"].as<std::string>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error finding telescope: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresTelescopeRepository::update(
    const Nyx::Domain::Telescope& telescope
  ) -> Nyx::Core::Result<Nyx::Domain::Telescope> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE telescopes SET name = $1, aperture_mm = $2, focal_length_mm = $3, "
        "optical_design = $4, brand = $5, model = $6, updated_at = NOW() "
        "WHERE id = $7 "
        "RETURNING id, user_id, name, aperture_mm, focal_length_mm, "
        "optical_design, brand, model",
        telescope.name, telescope.aperture_mm, telescope.focal_length_mm,
        telescope.optical_design, telescope.brand, telescope.model, telescope.id
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::not_found("Telescope not found"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Telescope{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .aperture_mm = row["aperture_mm"].as<int>(),
        .focal_length_mm = row["focal_length_mm"].as<int>(),
        .optical_design = row["optical_design"].as<std::string>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error updating telescope: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to update telescope"));
    }
  }

  auto PostgresTelescopeRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync("DELETE FROM telescopes WHERE id = $1", id);

      if (result.affectedRows() == 0) {
        return std::unexpected(Nyx::Core::AppError::not_found("Telescope not found"));
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error deleting telescope: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to delete telescope"));
    }
  }
} // namespace Nyx::Infrastructure::Persistence
