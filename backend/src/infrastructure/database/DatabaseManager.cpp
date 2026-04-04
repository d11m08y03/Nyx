#include "infrastructure/database/DatabaseManager.hpp"

#include <drogon/drogon.h>
#include <drogon/orm/DbConfig.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Database {
  auto DatabaseManager::initialize(
    const Nyx::Infrastructure::Config::
      EnvironmentConfig& config
  ) -> void {
    spdlog::info(
      "Initializing database connection to "
      "{}:{}/{}",
      config.database_host(),
      config.database_port(),
      config.database_name()
    );

    auto postgres_config =
      drogon::orm::PostgresConfig{
        .host = config.database_host(),
        .port = config.database_port(),
        .databaseName = config.database_name(),
        .username = config.database_user(),
        .password = config.database_password(),
        .connectionNumber = 4,
        .name = "default",
        .isFast = false,
        .characterSet = "",
        .timeout = -1.0,
        .autoBatch = false,
        .connectOptions = {},
      };

    drogon::app().addDbClient(postgres_config);

    spdlog::debug(
      "Database client configured for {}:{}/{}",
      config.database_host(),
      config.database_port(),
      config.database_name()
    );
  }

  auto DatabaseManager::check_health() -> void {
    try {
      auto database_client =
        drogon::app().getDbClient();
      database_client->execSqlSync("SELECT 1");
      spdlog::info("Database health check passed");
    } catch (const std::exception& exception) {
      spdlog::error(
        "Database health check failed: {}",
        exception.what()
      );
      throw;
    }
  }
} // namespace Nyx::Infrastructure::Database
