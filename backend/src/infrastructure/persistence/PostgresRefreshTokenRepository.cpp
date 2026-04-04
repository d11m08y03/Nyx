#include "infrastructure/persistence/PostgresRefreshTokenRepository.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Persistence {
  auto PostgresRefreshTokenRepository::store(
    const Nyx::Domain::RefreshToken& token
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client = drogon::app().getDbClient();

      database_client->execSqlSync(
        "INSERT INTO refresh_tokens "
        "(id, user_id, token_hash, family_id, is_revoked, expires_at) "
        "VALUES ($1, $2, $3, $4, $5, $6)",
        token.id, token.user_id, token.token_hash,
        token.family_id, token.is_revoked, token.expires_at
      );

      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error storing refresh token: {}", exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Failed to store refresh token")
      );
    }
  }

  auto PostgresRefreshTokenRepository::find_by_token_hash(
    const std::string& hash
  ) -> Nyx::Core::Result<std::optional<Nyx::Domain::RefreshToken>> {
    try {
      auto database_client = drogon::app().getDbClient();

      auto result = database_client->execSqlSync(
        "SELECT id, user_id, token_hash, family_id, is_revoked, expires_at "
        "FROM refresh_tokens WHERE token_hash = $1",
        hash
      );

      if (result.empty()) {
        return std::nullopt;
      }

      return Nyx::Domain::RefreshToken{
        .id = result[0]["id"].as<std::string>(),
        .user_id = result[0]["user_id"].as<std::string>(),
        .token_hash = result[0]["token_hash"].as<std::string>(),
        .family_id = result[0]["family_id"].as<std::string>(),
        .is_revoked = result[0]["is_revoked"].as<bool>(),
        .expires_at = result[0]["expires_at"].as<std::string>(),
      };
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error finding refresh token: {}", exception.base().what()
      );
      return std::unexpected(Nyx::Core::AppError::internal("Database error"));
    }
  }

  auto PostgresRefreshTokenRepository::revoke(
    const std::string& id
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client = drogon::app().getDbClient();

      database_client->execSqlSync(
        "UPDATE refresh_tokens SET is_revoked = TRUE WHERE id = $1", id
      );

      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error revoking refresh token: {}", exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Failed to revoke refresh token")
      );
    }
  }

  auto PostgresRefreshTokenRepository::revoke_family(
    const std::string& family_id
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client = drogon::app().getDbClient();

      database_client->execSqlSync(
        "UPDATE refresh_tokens SET is_revoked = TRUE WHERE family_id = $1",
        family_id
      );

      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error revoking token family: {}", exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Failed to revoke token family")
      );
    }
  }

  auto PostgresRefreshTokenRepository::revoke_all_for_user(
    const std::string& user_id
  ) -> Nyx::Core::Result<void> {
    try {
      auto database_client = drogon::app().getDbClient();

      database_client->execSqlSync(
        "UPDATE refresh_tokens SET is_revoked = TRUE WHERE user_id = $1",
        user_id
      );

      return {};
    } catch (const drogon::orm::DrogonDbException& exception) {
      spdlog::error(
        "Database error revoking all tokens for user: {}",
        exception.base().what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Failed to revoke user tokens")
      );
    }
  }
} // namespace Nyx::Infrastructure::Persistence
