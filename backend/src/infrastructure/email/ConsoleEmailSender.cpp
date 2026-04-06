#include "infrastructure/email/ConsoleEmailSender.hpp"

#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Email {
  ConsoleEmailSender::ConsoleEmailSender(
    std::shared_ptr<
      Nyx::Infrastructure::Config::EnvironmentConfig
    > config
  ) : config(std::move(config)) {}

  auto ConsoleEmailSender::send_verification_email(
    const std::string& to_email,
    const std::string& verification_token
  ) -> Nyx::Core::Result<void> {
    auto verification_url =
      this->config->frontend_url()
      + "/verify-email?token=" + verification_token;

    spdlog::info(
      "[EMAIL] Verification email for {}: {}",
      to_email, verification_url
    );

    return {};
  }
} // namespace Nyx::Infrastructure::Email
