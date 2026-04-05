#pragma once

#include "application/observation/IDngDecoder.hpp"
#include "core/error/AppError.hpp"

#include <vector>

namespace Nyx::Application::Observation {
  struct PhotometryResult {
    double raw_flux;
    double raw_flux_error;
  };

  struct DetectedSource {
    int x;
    int y;
    double peak_value;
  };

  class IPhotometryProcessor {
    public:
      virtual ~IPhotometryProcessor() = default;

      virtual auto detect_sources(
        const DecodedImage& image,
        double threshold_sigma
      ) -> std::vector<DetectedSource> = 0;

      virtual auto measure_aperture(
        const DecodedImage& image,
        int center_x,
        int center_y,
        double aperture_radius,
        double annulus_inner_radius,
        double annulus_outer_radius
      ) -> Nyx::Core::Result<PhotometryResult> = 0;
  };
} // namespace Nyx::Application::Observation
