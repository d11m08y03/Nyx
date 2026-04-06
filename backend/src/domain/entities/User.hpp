#pragma once

#include <optional>
#include <string>

namespace Nyx::Domain {
  struct User {
    std::string id;
    std::string email;
    std::optional<std::string> password_hash;
    std::optional<std::string> display_name;
    bool email_verified = false;
    std::string auth_provider = "local";
    std::optional<std::string> google_id;
  };
} // namespace Nyx::Domain
