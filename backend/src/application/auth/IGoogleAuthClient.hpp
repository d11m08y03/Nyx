#pragma once

#include "core/error/AppError.hpp"

#include <optional>
#include <string>

namespace Nyx::Application::Auth {
  struct GoogleUserInfo {
    std::string google_id;
    std::string email;
    bool email_verified;
    std::optional<std::string> name;
  };

  class IGoogleAuthClient {
    public:
      virtual ~IGoogleAuthClient() = default;

      virtual auto exchange_code(
        const std::string& code,
        const std::string& redirect_uri
      ) -> Nyx::Core::Result<GoogleUserInfo> = 0;
  };
} // namespace Nyx::Application::Auth
