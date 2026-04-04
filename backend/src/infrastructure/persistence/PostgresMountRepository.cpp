#include "infrastructure/persistence/PostgresMountRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresMountRepository::create(
    const Nyx::Domain::Mount& mount
  ) -> Nyx::Core::Result<Nyx::Domain::Mount> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO mounts (id, user_id, name, mount_type, is_tracked, has_goto, "
        "brand, model) VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
        "RETURNING id, user_id, name, mount_type, is_tracked, has_goto, brand, model",
        mount.id, mount.user_id, mount.name, mount.mount_type,
        mount.is_tracked, mount.has_goto, mount.brand, mount.model
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::internal("Failed to create mount"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Mount{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .mount_type = row["mount_type"].as<std::string>(),
        .is_tracked = row["is_tracked"].as<bool>(),
        .has_goto = row["has_goto"].as<bool>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error creating mount: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to create mount"));
    }
  }

  auto PostgresMountRepository::find_by_user_id(
    const std::string& user_id
  ) -> Nyx::Core::Result<std::vector<Nyx::Domain::Mount>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, mount_type, is_tracked, has_goto, brand, model "
        "FROM mounts WHERE user_id = $1 ORDER BY created_at DESC",
        user_id
      );

      auto mounts = std::vector<Nyx::Domain::Mount>{};
      for (const auto& row : result) {
        mounts.push_back(Nyx::Domain::Mount{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .mount_type = row["mount_type"].as<std::string>(),
          .is_tracked = row["is_tracked"].as<bool>(),
          .has_goto = row["has_goto"].as<bool>(),
          .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
          .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
        });
      }
      return mounts;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error listing mounts: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresMountRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::Mount>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, mount_type, is_tracked, has_goto, brand, model "
        "FROM mounts WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      const auto& row = result[0];
      return Nyx::Domain::Mount{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .mount_type = row["mount_type"].as<std::string>(),
        .is_tracked = row["is_tracked"].as<bool>(),
        .has_goto = row["has_goto"].as<bool>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error finding mount: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresMountRepository::update(
    const Nyx::Domain::Mount& mount
  ) -> Nyx::Core::Result<Nyx::Domain::Mount> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE mounts SET name = $1, mount_type = $2, is_tracked = $3, has_goto = $4, "
        "brand = $5, model = $6, updated_at = NOW() WHERE id = $7 "
        "RETURNING id, user_id, name, mount_type, is_tracked, has_goto, brand, model",
        mount.name, mount.mount_type, mount.is_tracked, mount.has_goto,
        mount.brand, mount.model, mount.id
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::not_found("Mount not found"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Mount{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .mount_type = row["mount_type"].as<std::string>(),
        .is_tracked = row["is_tracked"].as<bool>(),
        .has_goto = row["has_goto"].as<bool>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error updating mount: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to update mount"));
    }
  }

  auto PostgresMountRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync("DELETE FROM mounts WHERE id = $1", id);
      if (result.affectedRows() == 0) {
        return std::unexpected(Nyx::Core::AppError::not_found("Mount not found"));
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error deleting mount: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to delete mount"));
    }
  }
} // namespace Nyx::Infrastructure::Persistence
