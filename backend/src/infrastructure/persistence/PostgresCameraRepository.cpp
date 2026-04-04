#include "infrastructure/persistence/PostgresCameraRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresCameraRepository::create(
    const Nyx::Domain::Camera& camera
  ) -> Nyx::Core::Result<Nyx::Domain::Camera> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO cameras (id, user_id, name, sensor_type, brand, model, "
        "pixel_size_um, resolution) VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
        "RETURNING id, user_id, name, sensor_type, brand, model, pixel_size_um, resolution",
        camera.id, camera.user_id, camera.name, camera.sensor_type,
        camera.brand, camera.model, camera.pixel_size_um, camera.resolution
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::internal("Failed to create camera"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Camera{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .sensor_type = row["sensor_type"].as<std::string>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
        .pixel_size_um = row["pixel_size_um"].isNull() ? 0.0 : row["pixel_size_um"].as<double>(),
        .resolution = row["resolution"].isNull() ? "" : row["resolution"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error creating camera: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to create camera"));
    }
  }

  auto PostgresCameraRepository::find_by_user_id(
    const std::string& user_id
  ) -> Nyx::Core::Result<std::vector<Nyx::Domain::Camera>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, sensor_type, brand, model, pixel_size_um, resolution "
        "FROM cameras WHERE user_id = $1 ORDER BY created_at DESC",
        user_id
      );

      auto cameras = std::vector<Nyx::Domain::Camera>{};
      for (const auto& row : result) {
        cameras.push_back(Nyx::Domain::Camera{
          .id = row["id"].as<std::string>(),
          .user_id = row["user_id"].as<std::string>(),
          .name = row["name"].as<std::string>(),
          .sensor_type = row["sensor_type"].as<std::string>(),
          .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
          .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
          .pixel_size_um = row["pixel_size_um"].isNull() ? 0.0 : row["pixel_size_um"].as<double>(),
          .resolution = row["resolution"].isNull() ? "" : row["resolution"].as<std::string>(),
        });
      }
      return cameras;
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error listing cameras: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresCameraRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::Camera>> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT id, user_id, name, sensor_type, brand, model, pixel_size_um, resolution "
        "FROM cameras WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      const auto& row = result[0];
      return Nyx::Domain::Camera{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .sensor_type = row["sensor_type"].as<std::string>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
        .pixel_size_um = row["pixel_size_um"].isNull() ? 0.0 : row["pixel_size_um"].as<double>(),
        .resolution = row["resolution"].isNull() ? "" : row["resolution"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error finding camera: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresCameraRepository::update(
    const Nyx::Domain::Camera& camera
  ) -> Nyx::Core::Result<Nyx::Domain::Camera> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE cameras SET name = $1, sensor_type = $2, brand = $3, model = $4, "
        "pixel_size_um = $5, resolution = $6, updated_at = NOW() WHERE id = $7 "
        "RETURNING id, user_id, name, sensor_type, brand, model, pixel_size_um, resolution",
        camera.name, camera.sensor_type, camera.brand, camera.model,
        camera.pixel_size_um, camera.resolution, camera.id
      );

      if (result.empty()) {
        return std::unexpected(Nyx::Core::AppError::not_found("Camera not found"));
      }

      const auto& row = result[0];
      return Nyx::Domain::Camera{
        .id = row["id"].as<std::string>(),
        .user_id = row["user_id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .sensor_type = row["sensor_type"].as<std::string>(),
        .brand = row["brand"].isNull() ? "" : row["brand"].as<std::string>(),
        .model = row["model"].isNull() ? "" : row["model"].as<std::string>(),
        .pixel_size_um = row["pixel_size_um"].isNull() ? 0.0 : row["pixel_size_um"].as<double>(),
        .resolution = row["resolution"].isNull() ? "" : row["resolution"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error updating camera: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to update camera"));
    }
  }

  auto PostgresCameraRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync("DELETE FROM cameras WHERE id = $1", id);
      if (result.affectedRows() == 0) {
        return std::unexpected(Nyx::Core::AppError::not_found("Camera not found"));
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error deleting camera: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Failed to delete camera"));
    }
  }
} // namespace Nyx::Infrastructure::Persistence
