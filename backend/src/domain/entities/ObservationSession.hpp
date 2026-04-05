#pragma once

#include <optional>
#include <string>

namespace Nyx::Domain {
  struct ObservationSession {
    std::string id;
    std::string user_id;
    std::string target_id;
    std::optional<std::string> notes;
    std::string created_at;
    std::string updated_at;
  };
} // namespace Nyx::Domain
