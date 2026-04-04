#include "infrastructure/security/JwtTokenService.hpp"

#include <chrono>
#include <iomanip>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <sodium.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace Nyx::Infrastructure::Security {
  JwtTokenService::JwtTokenService(
    std::shared_ptr<
      Nyx::Infrastructure::Config::EnvironmentConfig
    > config
  ) : config(std::move(config)) {}

  auto JwtTokenService::generate_token_pair(
    const std::string& user_id,
    const std::string& email,
    const std::string& token_id,
    const std::optional<std::string>& family_id
  ) -> Nyx::Application::Auth::TokenPair {
    auto now = std::chrono::system_clock::now();

    auto refresh_expiry = now + std::chrono::seconds(
      this->config->jwt_refresh_token_expiry_seconds()
    );

    auto resolved_family_id = family_id.value_or(token_id);

    auto access_token = jwt::create()
      .set_issuer("nyx")
      .set_type("access")
      .set_issued_at(now)
      .set_expires_at(now + std::chrono::seconds(
        this->config->jwt_access_token_expiry_seconds()
      ))
      .set_payload_claim("sub", jwt::claim(user_id))
      .set_payload_claim("email", jwt::claim(email))
      .sign(jwt::algorithm::hs256{this->config->jwt_secret()});

    auto refresh_token = jwt::create()
      .set_issuer("nyx")
      .set_type("refresh")
      .set_id(token_id)
      .set_issued_at(now)
      .set_expires_at(refresh_expiry)
      .set_payload_claim("sub", jwt::claim(user_id))
      .set_payload_claim("email", jwt::claim(email))
      .set_payload_claim("fid", jwt::claim(resolved_family_id))
      .sign(jwt::algorithm::hs256{this->config->jwt_secret()});

    auto expiry_time = std::chrono::system_clock::to_time_t(refresh_expiry);
    auto expiry_stream = std::ostringstream{};
    expiry_stream << std::put_time(
      std::gmtime(&expiry_time), "%Y-%m-%dT%H:%M:%SZ"
    );

    return Nyx::Application::Auth::TokenPair{
      .access_token = std::move(access_token),
      .refresh_token = std::move(refresh_token),
      .refresh_token_id = token_id,
      .refresh_token_family_id = resolved_family_id,
      .refresh_token_expires_at = expiry_stream.str(),
    };
  }

  auto JwtTokenService::verify_refresh_token(
    const std::string& token
  ) -> Nyx::Core::Result<Nyx::Application::Auth::TokenClaims> {
    try {
      auto decoded_token = jwt::decode(token);

      auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{this->config->jwt_secret()})
        .with_issuer("nyx")
        .with_type("refresh");

      verifier.verify(decoded_token);

      return Nyx::Application::Auth::TokenClaims{
        .user_id = decoded_token.get_payload_claim("sub").as_string(),
        .email = decoded_token.get_payload_claim("email").as_string(),
      };
    } catch (const jwt::error::token_verification_exception& exception) {
      return std::unexpected(
        Nyx::Core::AppError::invalid_token("Refresh token is invalid")
      );
    } catch (const std::exception& exception) {
      spdlog::error(
        "Unexpected error verifying refresh token: {}", exception.what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Token verification failed")
      );
    }
  }

  auto JwtTokenService::verify_access_token(
    const std::string& token
  ) -> Nyx::Core::Result<Nyx::Application::Auth::TokenClaims> {
    try {
      auto decoded_token = jwt::decode(token);

      auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{this->config->jwt_secret()})
        .with_issuer("nyx");

      verifier.verify(decoded_token);

      return Nyx::Application::Auth::TokenClaims{
        .user_id = decoded_token.get_payload_claim("sub").as_string(),
        .email = decoded_token.get_payload_claim("email").as_string(),
      };
    } catch (const jwt::error::token_verification_exception& exception) {
      return std::unexpected(
        Nyx::Core::AppError::invalid_token("Access token is invalid")
      );
    } catch (const std::exception& exception) {
      spdlog::error(
        "Unexpected error verifying access token: {}", exception.what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal("Token verification failed")
      );
    }
  }

  auto JwtTokenService::hash_token(
    const std::string& token
  ) -> std::string {
    unsigned char hash[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(
      hash,
      reinterpret_cast<const unsigned char*>(token.c_str()),
      token.size()
    );

    char hex[crypto_hash_sha256_BYTES * 2 + 1];
    sodium_bin2hex(hex, sizeof(hex), hash, crypto_hash_sha256_BYTES);

    return std::string(hex);
  }
} // namespace Nyx::Infrastructure::Security
