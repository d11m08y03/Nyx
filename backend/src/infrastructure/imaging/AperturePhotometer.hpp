#pragma once

#include "application/observation/IPhotometryProcessor.hpp"

namespace Nyx::Infrastructure::Imaging {
  class AperturePhotometer
    : public Nyx::Application::Observation::
        IPhotometryProcessor {
    public:
      auto detect_sources(
        const Nyx::Application::Observation::
          DecodedImage& image,
        double threshold_sigma
      ) -> std::vector<
        Nyx::Application::Observation::DetectedSource
      > override;

      auto measure_aperture(
        const Nyx::Application::Observation::
          DecodedImage& image,
        int center_x,
        int center_y,
        double aperture_radius,
        double annulus_inner_radius,
        double annulus_outer_radius
      ) -> Nyx::Core::Result<
        Nyx::Application::Observation::PhotometryResult
      > override;
  };
} // namespace Nyx::Infrastructure::Imaging
