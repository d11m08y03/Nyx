#include "infrastructure/persistence/PostgresUserRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {
  auto map_user(
    const drogon::orm::Row& row
  ) -> Nyx::Domain::User {
    return Nyx::Domain::User{
      .id = row["id"].as<std::string>(),
      .email = row["email"].as<std::string>(),
      .password_hash = row["password_hash"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["password_hash"].as<std::string>()
          ),
      .display_name = row["display_name"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["display_name"].as<std::string>()
          ),
      .email_verified =
        row["email_verified"].as<bool>(),
      .auth_provider =
        row["auth_provider"].as<std::string>(),
      .google_id = row["google_id"].isNull()
        ? std::nullopt
        : std::optional<std::string>(
            row["google_id"].as<std::string>()
          ),
    };
  }
} // anonymous namespace

namespace Nyx::Infrastructure::Persistence {
  auto PostgresUserRepository::create(
    const Nyx::Domain::User& user
  ) -> Nyx::Core::Result<Nyx::Domain::User> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "INSERT INTO users "
        "(id, email, password_hash, email_verified, "
        "auth_provider, google_id) "
        "VALUES ($1, $2, NULLIF($3, ''), $4, $5, "
        "NULLIF($6, '')) "
        "RETURNING id, email, password_hash, "
        "display_name, email_verified, "
        "auth_provider, google_id",
        user.id, user.email,
        user.password_hash.value_or(""),
        user.email_verified, user.auth_provider,
        user.google_id.value_or("")
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal(
            "Failed to create user"
          )
        );
      }

      return map_user(result[0]);
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      auto error_message =
        std::string(exception.base().what());

      if (error_message.find("unique")
            != std::string::npos
          || error_message.find("duplicate")
               != std::string::npos) {
        return std::unexpected(
          Nyx::Core::AppError::conflict(
            "A user with this email already exists"
          )
        );
      }

      spdlog::error(
        "Database error creating user: {}",
        error_message
      );

      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create user"
        )
      );
    }
  }

  auto PostgresUserRepository::find_by_email(
    const std::string& email
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::User>
  > {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "SELECT id, email, password_hash, "
        "display_name, email_verified, "
        "auth_provider, google_id "
        "FROM users WHERE email = $1",
        email
      );

      if (result.empty()) {
        return std::nullopt;
      }

      return map_user(result[0]);
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error finding user by email: {}",
        exception.base().what()
      );

      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresUserRepository::find_by_id(
    const std::string& id
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::User>
  > {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "SELECT id, email, password_hash, "
        "display_name, email_verified, "
        "auth_provider, google_id "
        "FROM users WHERE id = $1",
        id
      );

      if (result.empty()) {
        return std::nullopt;
      }

      return map_user(result[0]);
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error finding user by id: {}",
        exception.base().what()
      );

      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresUserRepository::update_display_name(
    const std::string& user_id,
    const std::string& display_name
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "UPDATE users SET display_name = $1, "
        "updated_at = NOW() WHERE id = $2",
        display_name, user_id
      );

      if (result.affectedRows() == 0) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "User not found"
          )
        );
      }

      return {};
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error updating display name: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresUserRepository::verify_email(
    const std::string& user_id
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "UPDATE users SET email_verified = TRUE, "
        "updated_at = NOW() WHERE id = $1",
        user_id
      );

      if (result.affectedRows() == 0) {
        return std::unexpected(
          Nyx::Core::AppError::not_found(
            "User not found"
          )
        );
      }

      return {};
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error verifying email: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresUserRepository::find_by_google_id(
    const std::string& google_id
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::User>
  > {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "SELECT id, email, password_hash, "
        "display_name, email_verified, "
        "auth_provider, google_id "
        "FROM users WHERE google_id = $1",
        google_id
      );

      if (result.empty()) {
        return std::nullopt;
      }

      return map_user(result[0]);
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error finding user by google_id: {}",
        exception.base().what()
      );

      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
