#include "infrastructure/persistence/PostgresObservationSessionRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  static auto row_to_session(
    const drogon::orm::Row& row
  ) -> Nyx::Domain::ObservationSession {
    return Nyx::Domain::ObservationSession{
      .id = row["id"].as<std::string>(),
      .user_id = row["user_id"].as<std::string>(),
      .target_id = row["target_id"].as<std::string>(),
      .telescope_id =
        row["telescope_id"].as<std::string>(),
      .camera_id = row["camera_id"].as<std::string>(),
      .mount_id = row["mount_id"].as<std::string>(),
      .location_id =
        row["location_id"].as<std::string>(),
      .filter_id = row["filter_id"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["filter_id"].as<std::string>()
          ),
      .notes = row["notes"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["notes"].as<std::string>()
          ),
      .created_at = row["created_at"].as<std::string>(),
      .updated_at = row["updated_at"].as<std::string>(),
    };
  }

  auto PostgresObservationSessionRepository::create(
    const Nyx::Domain::ObservationSession& session
  ) -> Nyx::Core::Result<Nyx::Domain::ObservationSession> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "INSERT INTO observation_sessions "
        "(id, user_id, target_id, telescope_id, "
        "camera_id, mount_id, location_id, "
        "filter_id, notes) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, "
        "NULLIF($8, '')::uuid, NULLIF($9, '')) "
        "RETURNING *",
        session.id, session.user_id,
        session.target_id, session.telescope_id,
        session.camera_id, session.mount_id,
        session.location_id,
        session.filter_id.value_or(""),
        session.notes.value_or("")
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal(
            "Failed to create observation session"
          )
        );
      }

      return row_to_session(result[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error creating observation session: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create observation session"
        )
      );
    }
  }

  auto PostgresObservationSessionRepository::find_by_user_id(
    const std::string& user_id
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Domain::ObservationSession>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT * FROM observation_sessions "
        "WHERE user_id = $1 ORDER BY created_at DESC",
        user_id
      );

      auto sessions =
        std::vector<Nyx::Domain::ObservationSession>{};
      for (const auto& row : result) {
        sessions.push_back(row_to_session(row));
      }
      return sessions;
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error listing observation sessions: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresObservationSessionRepository::
    find_by_user_id_and_target_id(
      const std::string& user_id,
      const std::string& target_id
    ) -> Nyx::Core::Result<
      std::vector<Nyx::Domain::ObservationSession>
    > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT * FROM observation_sessions "
        "WHERE user_id = $1 AND target_id = $2 "
        "ORDER BY created_at DESC",
        user_id, target_id
      );

      auto sessions =
        std::vector<Nyx::Domain::ObservationSession>{};
      for (const auto& row : result) {
        sessions.push_back(row_to_session(row));
      }
      return sessions;
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error finding sessions by user "
        "and target: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresObservationSessionRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::ObservationSession>
  > {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "SELECT * FROM observation_sessions "
        "WHERE id = $1",
        id
      );

      if (result.empty()) return std::nullopt;

      return row_to_session(result[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error finding observation session: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresObservationSessionRepository::update(
    const Nyx::Domain::ObservationSession& session
  ) -> Nyx::Core::Result<Nyx::Domain::ObservationSession> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "UPDATE observation_sessions "
        "SET notes = NULLIF($1, ''), "
        "filter_id = NULLIF($2, '')::uuid, "
        "updated_at = NOW() "
        "WHERE id = $3 "
        "RETURNING *",
        session.notes.value_or(""),
        session.filter_id.value_or(""),
        session.id
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "Observation session not found"
          )
        );
      }

      return row_to_session(result[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error updating observation session: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to update observation session"
        )
      );
    }
  }

  auto PostgresObservationSessionRepository::remove(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto db = drogon::app().getDbClient();
      auto result = db->execSqlSync(
        "DELETE FROM observation_sessions WHERE id = $1",
        id
      );

      if (result.affectedRows() == 0) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "Observation session not found"
          )
        );
      }
      return {};
    } catch (const drogon::orm::DrogonDbException& e) {
      spdlog::error(
        "Database error deleting observation session: {}",
        e.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to delete observation session"
        )
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
