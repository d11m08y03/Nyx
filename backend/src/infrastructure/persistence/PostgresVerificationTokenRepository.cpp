#include "infrastructure/persistence/PostgresVerificationTokenRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresVerificationTokenRepository::store(
    const Nyx::Domain::VerificationToken& token
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      database_client->execSqlSync(
        "INSERT INTO verification_tokens "
        "(id, user_id, token_hash, expires_at) "
        "VALUES ($1, $2, $3, $4)",
        token.id, token.user_id,
        token.token_hash, token.expires_at
      );

      return {};
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error storing verification token: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to store verification token"
        )
      );
    }
  }

  auto PostgresVerificationTokenRepository::find_by_token_hash(
    const std::string& token_hash
  ) -> Nyx::Core::Result<
    std::optional<Nyx::Domain::VerificationToken>
  > {
    try {
      auto database_client =
        drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "SELECT id, user_id, token_hash, "
        "expires_at, used_at "
        "FROM verification_tokens "
        "WHERE token_hash = $1",
        token_hash
      );

      if (result.empty()) {
        return std::nullopt;
      }

      return Nyx::Domain::VerificationToken{
        .id = result[0]["id"].as<std::string>(),
        .user_id =
          result[0]["user_id"].as<std::string>(),
        .token_hash =
          result[0]["token_hash"].as<std::string>(),
        .expires_at =
          result[0]["expires_at"].as<std::string>(),
        .used_at = result[0]["used_at"].isNull()
          ? std::nullopt
          : std::optional<std::string>(
              result[0]["used_at"].as<std::string>()
            ),
      };
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error finding verification token: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Database error")
      );
    }
  }

  auto PostgresVerificationTokenRepository::mark_used(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      database_client->execSqlSync(
        "UPDATE verification_tokens "
        "SET used_at = NOW() WHERE id = $1",
        id
      );

      return {};
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error marking verification "
        "token as used: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to mark token as used"
        )
      );
    }
  }

  auto PostgresVerificationTokenRepository::revoke_all_for_user(
    const std::string& user_id
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client =
        drogon::app().getDbClient();

      database_client->execSqlSync(
        "DELETE FROM verification_tokens "
        "WHERE user_id = $1",
        user_id
      );

      return {};
    } catch (
      const drogon::orm::DrogonDbException& exception
    ) {
      spdlog::error(
        "Database error revoking verification "
        "tokens for user: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to revoke verification tokens"
        )
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
