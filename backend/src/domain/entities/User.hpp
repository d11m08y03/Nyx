#pragma once

#include <string>

namespace Nyx::Domain {
  struct User {
    std::string id;
    std::string email;
    std::string password_hash;
  };
} // namespace Nyx::Domain
