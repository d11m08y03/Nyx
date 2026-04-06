#pragma once

#include <optional>
#include <string>

namespace Nyx::Domain {
  struct VerificationToken {
    std::string id;
    std::string user_id;
    std::string token_hash;
    std::string expires_at;
    std::optional<std::string> used_at;
  };
} // namespace Nyx::Domain
