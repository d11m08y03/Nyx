#pragma once

#include <optional>
#include <string>

namespace Nyx::Domain {
  struct User {
    std::string id;
    std::string email;
    std::string password_hash;
    std::optional<std::string> display_name;
  };
} // namespace Nyx::Domain
