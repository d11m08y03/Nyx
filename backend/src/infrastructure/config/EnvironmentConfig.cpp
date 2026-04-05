#include "infrastructure/config/EnvironmentConfig.hpp"

#include <cstdlib>
#include <fstream>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Nyx::Infrastructure::Config {
  EnvironmentConfig::EnvironmentConfig() {
    load_env_file(".env");

    this->port = static_cast<uint16_t>(
      std::stoi(optional_env("SERVER_PORT", "8080"))
    );
    this->threads = static_cast<uint32_t>(
      std::stoi(optional_env("SERVER_THREADS", "4"))
    );
    this->db_host = require_env("PGHOST");
    this->db_port = static_cast<uint16_t>(
      std::stoi(optional_env("PGPORT", "5432"))
    );
    this->db_name = require_env("PGDATABASE");
    this->db_user = require_env("PGUSER");
    this->db_password = require_env("PGPASSWORD");
    this->db_sslmode =
      optional_env("PGSSLMODE", "prefer");
    this->secret = require_env("JWT_SECRET");
    this->access_token_expiry_seconds =
      static_cast<uint32_t>(std::stoi(optional_env(
        "JWT_ACCESS_TOKEN_EXPIRY_SECONDS", "900"
      )));
    this->refresh_token_expiry_seconds =
      static_cast<uint32_t>(std::stoi(optional_env(
        "JWT_REFRESH_TOKEN_EXPIRY_SECONDS", "604800"
      )));
    this->level = optional_env("LOG_LEVEL", "debug");
    this->mast_base_url = optional_env(
      "NASA_MAST_BASE_URL", "/api/v0"
    );
    this->exoplanet_archive_base_url = optional_env(
      "NASA_EXOPLANET_ARCHIVE_BASE_URL",
      "https://exoplanetarchive.ipac.caltech.edu"
    );

    spdlog::debug(
      "EnvironmentConfig loaded successfully"
    );
    spdlog::debug(
      "  server_port={}, server_threads={}",
      this->port, this->threads
    );
    spdlog::debug(
      "  database_host={}, database_port={}, "
      "database_name={}",
      this->db_host, this->db_port, this->db_name
    );
    spdlog::debug("  log_level={}", this->level);
  }

  auto EnvironmentConfig::load_env_file(
    const std::string& path
  ) -> void {
    auto file = std::ifstream{path};
    if (!file.is_open()) {
      spdlog::debug(
        "No .env file found at '{}'", path
      );
      return;
    }

    spdlog::debug(
      "Loading environment from '{}'", path
    );

    auto line = std::string{};
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#') {
        continue;
      }

      auto delimiter_position = line.find('=');
      if (delimiter_position == std::string::npos) {
        continue;
      }

      auto key =
        line.substr(0, delimiter_position);
      auto value =
        line.substr(delimiter_position + 1);

      if (value.size() >= 2) {
        auto front = value.front();
        auto back = value.back();
        if ((front == '"' && back == '"')
          || (front == '\'' && back == '\'')) {
          value =
            value.substr(1, value.size() - 2);
        }
      }

      setenv(key.c_str(), value.c_str(), 0);
    }
  }

  auto EnvironmentConfig::require_env(
    const char* variable_name
  ) -> std::string {
    const char* value = std::getenv(variable_name);
    if (value == nullptr || value[0] == '\0') {
      spdlog::critical(
        "Required environment variable '{}' "
        "is not set",
        variable_name
      );

      throw std::runtime_error(
        std::string(
          "Required environment variable "
          "is not set: "
        ) + variable_name
      );
    }

    return std::string(value);
  }

  auto EnvironmentConfig::optional_env(
    const char* variable_name,
    const char* default_value
  ) -> std::string {
    const char* value = std::getenv(variable_name);
    if (value == nullptr || value[0] == '\0') {
      spdlog::debug(
        "Environment variable '{}' not set, "
        "using default: '{}'",
        variable_name, default_value
      );

      return std::string(default_value);
    }

    return std::string(value);
  }
} // namespace Nyx::Infrastructure::Config
