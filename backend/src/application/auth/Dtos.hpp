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

  struct UserProfile {
    std::string id;
    std::string email;
  };
} // namespace Nyx::Application::Auth
