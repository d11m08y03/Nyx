#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Nyx::Domain {
  struct LightCurvePoint {
    int64_t id;
    std::string tess_observation_id;
    double time;
    std::optional<float> pdcsap_flux;
    std::optional<float> pdcsap_flux_err;
    std::optional<float> sap_flux;
    int quality;
  };
} // namespace Nyx::Domain
