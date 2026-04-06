#pragma once

#include "core/error/AppError.hpp"

#include <string>

namespace Nyx::Application::Auth {
  class IEmailSender {
    public:
      virtual ~IEmailSender() = default;

      virtual auto send_verification_email(
        const std::string& to_email,
        const std::string& verification_token
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Application::Auth
