#pragma once

#include <string>

namespace Nyx::Application::Auth {
  struct RegisterRequest {
    std::string email;
    std::string password;
  };

  struct LoginRequest {
    std::string email;
    std::string password;
  };

  struct GoogleLoginRequest {
    std::string code;
    std::string redirect_uri;
  };

  struct UserProfile {
    std::string id;
    std::string email;
    bool email_verified = false;
  };
} // namespace Nyx::Application::Auth
