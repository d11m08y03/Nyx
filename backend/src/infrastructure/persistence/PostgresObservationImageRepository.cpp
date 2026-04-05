#include "infrastructure/persistence/PostgresObservationImageRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  static auto row_to_image(
    const drogon::orm::Row& row
  ) -> Nyx::Domain::ObservationImage {
    return Nyx::Domain::ObservationImage{
      .id = row["id"].as<std::string>(),
      .session_id = row["session_id"].as<std::string>(),
      .filename = row["filename"].as<std::string>(),
      .original_filename =
        row["original_filename"].as<std::string>(),
      .file_path = row["file_path"].as<std::string>(),
      .file_size_bytes =
        row["file_size_bytes"].as<int64_t>(),
      .mime_type = row["mime_type"].as<std::string>(),
      .captured_at = row["captured_at"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["captured_at"].as<std::string>()
          ),
      .camera_model = row["camera_model"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["camera_model"].as<std::string>()
          ),
      .exposure_time_seconds =
        row["exposure_time_seconds"].isNull()
          ? std::nullopt
          : std::optional<double>(
              row["exposure_time_seconds"].as<double>()
            ),
      .iso_speed = row["iso_speed"].isNull()
        ? std::nullopt
        : std::optional<int>(
            row["iso_speed"].as<int>()
          ),
      .gps_latitude = row["gps_latitude"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["gps_latitude"].as<double>()
          ),
      .gps_longitude = row["gps_longitude"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["gps_longitude"].as<double>()
          ),
      .image_width = row["image_width"].isNull()
        ? std::nullopt
        : std::optional<int>(
            row["image_width"].as<int>()
          ),
      .image_height = row["image_height"].isNull()
        ? std::nullopt
        : std::optional<int>(
            row["image_height"].as<int>()
          ),
      .target_x = row["target_x"].isNull()
        ? std::nullopt
        : std::optional<int>(
            row["target_x"].as<int>()
          ),
      .target_y = row["target_y"].isNull()
        ? std::nullopt
        : std::optional<int>(
            row["target_y"].as<int>()
          ),
      .raw_flux = row["raw_flux"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["raw_flux"].as<double>()
          ),
      .raw_flux_error = row["raw_flux_error"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["raw_flux_error"].as<double>()
          ),
      .relative_flux = row["relative_flux"].isNull()
        ? std::nullopt
        : std::optional<double>(
            row["relative_flux"].as<double>()
          ),
      .relative_flux_error =
        row["relative_flux_error"].isNull()
          ? std::nullopt
          : std::optional<double>(
              row["relative_flux_error"].as<double>()
            ),
      .photometry_status =
        row["photometry_status"].isNull()
          ? std::nullopt
          : std::optional<std::string>(
              row["photometry_status"].as<std::string>()
            ),
      .photometry_error_message =
        row["photometry_error_message"].isNull()
          ? std::nullopt
          : std::optional<std::string>(
              row["photometry_error_message"]
                .as<std::string>()
            ),
      .created_at = row["created_at"].as<std::string>(),
    };
  }

  auto PostgresObservationImageRepository::create(
    const Nyx::Domain::ObservationImage& image
  ) -> Nyx::Core::Result<Nyx::Domain::ObservationImage> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO observation_images "
        "(id, session_id, filename, original_filename, "
        "file_path, file_size_bytes, mime_type, "
        "captured_at, camera_model, "
        "exposure_time_seconds, iso_speed, "
        "gps_latitude, gps_longitude, "
        "image_width, image_height) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, "
        "NULLIF($8, '')::timestamptz, NULLIF($9, ''), "
        "NULLIF($10, '')::double precision, "
        "NULLIF($11, '')::integer, "
        "NULLIF($12, '')::double precision, "
        "NULLIF($13, '')::double precision, "
        "NULLIF($14, '')::integer, "
        "NULLIF($15, '')::integer) "
        "RETURNING *",
        image.id, image.session_id,
        image.filename, image.original_filename,
        image.file_path, image.file_size_bytes,
        image.mime_type,
        image.captured_at.value_or(""),
        image.camera_model.value_or(""),
        image.exposure_time_seconds.has_value()
          ? std::to_string(
              image.exposure_time_seconds.value()
            )
          : "",
        image.iso_speed.has_value()
          ? std::to_string(image.iso_speed.value())
          : "",
        image.gps_latitude.has_value()
          ? std::to_string(image.gps_latitude.value())
          : "",
        image.gps_longitude.has_value()
          ? std::to_string(image.gps_longitude.value())
          : "",
        image.image_width.has_value()
          ? std::to_string(image.image_width.value())
          : "",
        image.image_height.has_value()
          ? std::to_string(image.image_height.value())
          : ""
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal(
            "Failed to create observation image"
          )
        );
      }

      return row_to_image(result[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error creating observation image: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create observation image"
        )
      );
    }
  }

  auto PostgresObservationImageRepository::find_by_session_id(
    const std::string& session_id
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Domain::ObservationImage>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT * FROM observation_images "
        "WHERE session_id = $1 "
        "ORDER BY created_at DESC",
        session_id
      );

      auto images =
        std::vector<Nyx::Domain::ObservationImage>{};
      for (const auto& row : result) {
        images.push_back(row_to_image(row));
      }
      return images;
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error listing observation images: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresObservationImageRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::ObservationImage>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT * FROM observation_images WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      return row_to_image(result[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error finding observation image: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresObservationImageRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "DELETE FROM observation_images WHERE id = $1",
        id
      );

      if (result.affectedRows() == 0) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "Observation image not found"
          )
        );
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error deleting observation image: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to delete observation image"
        )
      );
    }
  }
  auto PostgresObservationImageRepository::
    update_photometry(
      const Nyx::Domain::ObservationImage& image
    ) -> Nyx::Core::Result<
      Nyx::Domain::ObservationImage
    > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE observation_images SET "
        "target_x = NULLIF($1, '')::integer, "
        "target_y = NULLIF($2, '')::integer, "
        "raw_flux = NULLIF($3, '')::double precision, "
        "raw_flux_error = "
        "NULLIF($4, '')::double precision, "
        "relative_flux = "
        "NULLIF($5, '')::double precision, "
        "relative_flux_error = "
        "NULLIF($6, '')::double precision, "
        "photometry_status = NULLIF($7, ''), "
        "photometry_error_message = NULLIF($8, '') "
        "WHERE id = $9 RETURNING *",
        image.target_x.has_value()
          ? std::to_string(image.target_x.value())
          : "",
        image.target_y.has_value()
          ? std::to_string(image.target_y.value())
          : "",
        image.raw_flux.has_value()
          ? std::to_string(image.raw_flux.value())
          : "",
        image.raw_flux_error.has_value()
          ? std::to_string(
              image.raw_flux_error.value()
            )
          : "",
        image.relative_flux.has_value()
          ? std::to_string(
              image.relative_flux.value()
            )
          : "",
        image.relative_flux_error.has_value()
          ? std::to_string(
              image.relative_flux_error.value()
            )
          : "",
        image.photometry_status.value_or(""),
        image.photometry_error_message.value_or("")
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "Observation image not found"
          )
        );
      }

      return row_to_image(result[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error updating photometry: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to update photometry"
        )
      );
    }
  }

  auto PostgresObservationImageRepository::
    update_photometry_status_batch(
    const std::string& session_id,
    const std::string& status
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      db->execSqlSync(
        "UPDATE observation_images "
        "SET photometry_status = $1 "
        "WHERE session_id = $2",
        status, session_id
      );
      return {};
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error batch updating photometry "
        "status: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to update photometry status"
        )
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
