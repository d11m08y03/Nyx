#pragma once

#include <string>

namespace Nyx::Domain {
  struct RefreshToken {
    std::string id;
    std::string user_id;
    std::string token_hash;
    std::string family_id;
    bool is_revoked;
    std::string expires_at;
  };
} // namespace Nyx::Domain
