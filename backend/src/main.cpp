#include "core/logging/Logger.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"
#include "infrastructure/database/DatabaseManager.hpp"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

auto main() -> int {
  try {
    auto config = Nyx::Infrastructure::Config::EnvironmentConfig();

    Nyx::Core::Logger::initialize(
      config.log_level()
    );

    spdlog::info("Starting Nyx backend v0.1.0");
    spdlog::info(
      "Configuring server on port {} "
      "with {} threads",
      config.server_port(),
      config.server_threads()
    );

    trantor::Logger::setLogLevel(
      trantor::Logger::kFatal
    );

    Nyx::Infrastructure::Database::DatabaseManager::initialize(config);

    drogon::app()
      .setLogLevel(trantor::Logger::kFatal)
      .addListener(
        "0.0.0.0", config.server_port()
      )
      .setThreadNum(config.server_threads())
      .registerBeginningAdvice([] {
        spdlog::info("Nyx backend is running");
      })
      .run();
  } catch (const std::exception& exception) {
    spdlog::critical(
      "Fatal error during startup: {}",
      exception.what()
    );
    return 1;
  }

  return 0;
}
