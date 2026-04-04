#pragma once

#include "application/auth/ITokenService.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"

#include <memory>
#include <optional>

namespace Nyx::Infrastructure::Security {
  class JwtTokenService
    : public Nyx::Application::Auth::ITokenService {
    public:
      explicit JwtTokenService(
        std::shared_ptr<
          Nyx::Infrastructure::Config::EnvironmentConfig
        > config
      );

      auto generate_token_pair(
        const std::string& user_id,
        const std::string& email,
        const std::string& token_id,
        const std::optional<std::string>& family_id
      ) -> Nyx::Application::Auth::TokenPair override;

      auto verify_refresh_token(
        const std::string& token
      ) -> Nyx::Core::Result<
        Nyx::Application::Auth::TokenClaims
      > override;

      auto verify_access_token(
        const std::string& token
      ) -> Nyx::Core::Result<
        Nyx::Application::Auth::TokenClaims
      > override;

      auto hash_token(
        const std::string& token
      ) -> std::string override;

    private:
      std::shared_ptr<
        Nyx::Infrastructure::Config::EnvironmentConfig
      > config;
  };
} // namespace Nyx::Infrastructure::Security
