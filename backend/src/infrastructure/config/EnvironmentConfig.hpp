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

      [[nodiscard]] auto cookie_secure() const -> bool {
        return this->is_cookie_secure;
      }

      [[nodiscard]] auto frontend_url() const -> const std::string& {
        return this->front_url;
      }

      [[nodiscard]] auto verification_token_expiry_seconds() const -> uint32_t {
        return this->verification_expiry_seconds;
      }

      [[nodiscard]] auto smtp_host() const -> const std::string& {
        return this->mail_smtp_host;
      }

      [[nodiscard]] auto smtp_port() const -> uint16_t {
        return this->mail_smtp_port;
      }

      [[nodiscard]] auto smtp_username() const -> const std::string& {
        return this->mail_smtp_username;
      }

      [[nodiscard]] auto smtp_password() const -> const std::string& {
        return this->mail_smtp_password;
      }

      [[nodiscard]] auto smtp_from_email() const -> const std::string& {
        return this->mail_smtp_from_email;
      }

      [[nodiscard]] auto smtp_use_tls() const -> bool {
        return this->mail_smtp_use_tls;
      }

      [[nodiscard]] auto google_client_id() const -> const std::string& {
        return this->oauth_google_client_id;
      }

      [[nodiscard]] auto google_client_secret() const -> const std::string& {
        return this->oauth_google_client_secret;
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

      bool is_cookie_secure;
      std::string front_url;
      uint32_t verification_expiry_seconds;

      std::string mail_smtp_host;
      uint16_t mail_smtp_port;
      std::string mail_smtp_username;
      std::string mail_smtp_password;
      std::string mail_smtp_from_email;
      bool mail_smtp_use_tls;

      std::string oauth_google_client_id;
      std::string oauth_google_client_secret;
  };
} // namespace Nyx::Infrastructure::Config
