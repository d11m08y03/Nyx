#pragma once

#include <optional>
#include <string>

namespace Nyx::Domain {
  struct TessObservation {
    std::string id;
    std::string target_id;
    std::string obsid;
    int cadence_seconds;
    double start_time;
    double end_time;
    std::optional<std::string> data_uri;
  };
} // namespace Nyx::Domain
