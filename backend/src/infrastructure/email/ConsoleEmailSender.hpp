#pragma once

#include "application/auth/IEmailSender.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"

#include <memory>

namespace Nyx::Infrastructure::Email {
  class ConsoleEmailSender
    : public Nyx::Application::Auth::IEmailSender {
    public:
      explicit ConsoleEmailSender(
        std::shared_ptr<
          Nyx::Infrastructure::Config::EnvironmentConfig
        > config
      );

      auto send_verification_email(
        const std::string& to_email,
        const std::string& verification_token
      ) -> Nyx::Core::Result<void> override;

    private:
      std::shared_ptr<
        Nyx::Infrastructure::Config::EnvironmentConfig
      > config;
  };
} // namespace Nyx::Infrastructure::Email
