#pragma once

#include <optional>
#include <string>

namespace Nyx::Domain {
  struct Target {
    std::string id;
    std::string canonical_name;
    std::optional<std::string> target_type;
    std::optional<double> right_ascension;
    std::optional<double> declination;
  };
} // namespace Nyx::Domain
