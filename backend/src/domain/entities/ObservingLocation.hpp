#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Nyx::Domain {
  struct ObservingLocation {
    std::string id;
    std::string user_id;
    std::string name;
    double latitude;
    double longitude;
    std::optional<int16_t> bortle_class;
  };
} // namespace Nyx::Domain
