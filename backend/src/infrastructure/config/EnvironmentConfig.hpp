#pragma once

#include <cstdint>
#include <string>

namespace Nyx::Infrastructure::Config {
  class EnvironmentConfig {
    public:
      EnvironmentConfig();

      [[nodiscard]] auto server_port() const -> uint16_t {
        return this->port;
      }

      [[nodiscard]] auto server_threads() const -> uint32_t {
        return this->threads;
      }

      [[nodiscard]] auto database_host() const -> const std::string& {
        return this->db_host;
      }

      [[nodiscard]] auto database_port() const -> uint16_t {
        return this->db_port;
      }

      [[nodiscard]] auto database_name() const -> const std::string& {
        return this->db_name;
      }

      [[nodiscard]] auto database_user() const -> const std::string& {
        return this->db_user;
      }

      [[nodiscard]] auto database_password() const -> const std::string& {
        return this->db_password;
      }

      [[nodiscard]] auto database_sslmode() const -> const std::string& {
        return this->db_sslmode;
      }

      [[nodiscard]] auto jwt_secret() const -> const std::string& {
        return this->secret;
      }

      [[nodiscard]] auto jwt_access_token_expiry_seconds() const -> uint32_t {
        return this->access_token_expiry_seconds;
      }

      [[nodiscard]] auto jwt_refresh_token_expiry_seconds() const -> uint32_t {
        return this->refresh_token_expiry_seconds;
      }

      [[nodiscard]] auto log_level() const -> const std::string& {
        return this->level;
      }

      [[nodiscard]] auto nasa_mast_base_url() const -> const std::string& {
        return this->mast_base_url;
      }

      [[nodiscard]] auto nasa_exoplanet_archive_base_url() const -> const std::string& {
        return this->exoplanet_archive_base_url;
      }

    private:
      static auto load_env_file(const std::string& path) -> void;
      static auto require_env(const char* variable_name) -> std::string;
      static auto optional_env(
        const char* variable_name,
        const char* default_value
      ) -> std::string;

      uint16_t port;
      uint32_t threads;

      std::string db_host;
      uint16_t db_port;
      std::string db_name;
      std::string db_user;
      std::string db_password;
      std::string db_sslmode;

      std::string secret;
      uint32_t access_token_expiry_seconds;
      uint32_t refresh_token_expiry_seconds;

      std::string level;

      std::string mast_base_url;
      std::string exoplanet_archive_base_url;
  };
} // namespace Nyx::Infrastructure::Config
