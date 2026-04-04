#include "infrastructure/persistence/PostgresUserRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresUserRepository::create(
    const Nyx::Domain::User& user
  ) -> Nyx::Core::Result<Nyx::Domain::User> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "INSERT INTO users (id, email, password_hash) "
        "VALUES ($1, $2, $3) "
        "RETURNING id, email, password_hash, display_name",
        user.id, user.email, user.password_hash
      );

      if (result.empty()) {
        return std::unexpected(
          Nyx::Core::AppError::internal("Failed to create user")
        );
      }

      return Nyx::Domain::User{
        .id = result[0]["id"].as<std::string>(),
        .email = result[0]["email"].as<std::string>(),
        .password_hash = result[0]["password_hash"].as<std::string>(),
        .display_name = result[0]["display_name"].isNull()
          ? std::nullopt
          : std::optional<std::string>(
              result[0]["display_name"].as<std::string>()
            ),
      };
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
        "SELECT id, email, password_hash, display_name "
        "FROM users WHERE email = $1",
        email
      );

      if (result.empty()) {
        return std::nullopt;
      }

      return Nyx::Domain::User{
        .id = result[0]["id"].as<std::string>(),
        .email = result[0]["email"].as<std::string>(),
        .password_hash = result[0]["password_hash"].as<std::string>(),
        .display_name = result[0]["display_name"].isNull()
          ? std::nullopt
          : std::optional<std::string>(
              result[0]["display_name"].as<std::string>()
            ),
      };
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error finding user by email: {}",
        exception.base().what()
      );

      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Database error"
        )
      );
    }
  }

  auto PostgresUserRepository::update_display_name(
    const std::string& user_id,
    const std::string& display_name
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client = drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "UPDATE users SET display_name = $1, updated_at = NOW() WHERE id = $2",
        display_name, user_id
      );

      if (result.affectedRows() == 0) {
        return std::unexpected(Nyx::Core::AppError::not_found("User not found"));
      }

      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error("Database error updating display name: {}", exception.base().what());
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }
} // namespace Nyx::Infrastructure::Persistence
