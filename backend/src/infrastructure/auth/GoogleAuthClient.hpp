#pragma once

#include "application/auth/IGoogleAuthClient.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"

#include <memory>

namespace Nyx::Infrastructure::Auth {
  class GoogleAuthClient
    : public Nyx::Application::Auth::IGoogleAuthClient {
    public:
      explicit GoogleAuthClient(
        std::shared_ptr<
          Nyx::Infrastructure::Config::EnvironmentConfig
        > config
      );

      auto exchange_code(
        const std::string& code,
        const std::string& redirect_uri
      ) -> Nyx::Core::Result<
        Nyx::Application::Auth::GoogleUserInfo
      > override;

    private:
      std::shared_ptr<
        Nyx::Infrastructure::Config::EnvironmentConfig
      > config;
  };
} // namespace Nyx::Infrastructure::Auth
