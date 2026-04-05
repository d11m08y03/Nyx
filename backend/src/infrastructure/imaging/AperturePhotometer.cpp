#include "infrastructure/imaging/AperturePhotometer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Imaging {
  static auto to_luminance(
    const Nyx::Application::Observation::DecodedImage&
      image,
    int x,
    int y
  ) -> double {
    auto idx = static_cast<std::size_t>(
      (y * image.width + x) * image.channels
    );
    if (image.channels >= 3) {
      return 0.2126 * image.pixels[idx]
        + 0.7152 * image.pixels[idx + 1]
        + 0.0722 * image.pixels[idx + 2];
    }
    return static_cast<double>(image.pixels[idx]);
  }

  auto AperturePhotometer::detect_sources(
    const Nyx::Application::Observation::DecodedImage&
      image,
    double threshold_sigma
  ) -> std::vector<
    Nyx::Application::Observation::DetectedSource
  > {
    auto total_pixels = static_cast<std::size_t>(
      image.width * image.height
    );

    auto luminance = std::vector<double>(total_pixels);
    for (auto y = 0; y < image.height; ++y) {
      for (auto x = 0; x < image.width; ++x) {
        auto idx = static_cast<std::size_t>(
          y * image.width + x
        );
        luminance[idx] = to_luminance(image, x, y);
      }
    }

    auto mean = std::accumulate(
      luminance.begin(), luminance.end(), 0.0
    ) / static_cast<double>(total_pixels);

    auto variance = 0.0;
    for (auto val : luminance) {
      auto diff = val - mean;
      variance += diff * diff;
    }
    variance /= static_cast<double>(total_pixels);
    auto stddev = std::sqrt(variance);

    auto threshold = mean + threshold_sigma * stddev;

    constexpr auto window_half = 5;
    constexpr auto edge_margin = 20;
    auto sources = std::vector<
      Nyx::Application::Observation::DetectedSource
    >{};

    for (auto y = edge_margin;
         y < image.height - edge_margin; ++y) {
      for (auto x = edge_margin;
           x < image.width - edge_margin; ++x) {
        auto idx = static_cast<std::size_t>(
          y * image.width + x
        );
        auto value = luminance[idx];

        if (value < threshold) continue;

        auto is_local_max = true;
        for (auto dy = -window_half;
             dy <= window_half && is_local_max; ++dy) {
          for (auto dx = -window_half;
               dx <= window_half && is_local_max;
               ++dx) {
            if (dx == 0 && dy == 0) continue;
            auto neighbor_idx = static_cast<std::size_t>(
              (y + dy) * image.width + (x + dx)
            );
            if (luminance[neighbor_idx] > value) {
              is_local_max = false;
            }
          }
        }

        if (is_local_max) {
          sources.push_back(
            Nyx::Application::Observation::
              DetectedSource{
                .x = x, .y = y, .peak_value = value,
              }
          );
        }
      }
    }

    std::sort(
      sources.begin(), sources.end(),
      [](const auto& a, const auto& b) {
        return a.peak_value > b.peak_value;
      }
    );

    spdlog::debug(
      "Detected {} sources above {:.1f} sigma",
      sources.size(), threshold_sigma
    );

    return sources;
  }

  auto AperturePhotometer::measure_aperture(
    const Nyx::Application::Observation::DecodedImage&
      image,
    int center_x,
    int center_y,
    double aperture_radius,
    double annulus_inner_radius,
    double annulus_outer_radius
  ) -> Nyx::Core::Result<
    Nyx::Application::Observation::PhotometryResult
  > {
    auto outer_int = static_cast<int>(
      std::ceil(annulus_outer_radius)
    );

    if (center_x - outer_int < 0
        || center_x + outer_int >= image.width
        || center_y - outer_int < 0
        || center_y + outer_int >= image.height) {
      return std::unexpected(
        Nyx::Core::AppError::validation(
          "Aperture extends beyond image bounds",
          {{"position",
            "Star too close to image edge"}}
        )
      );
    }

    auto aperture_sum = 0.0;
    auto aperture_count = 0;
    auto annulus_sum = 0.0;
    auto annulus_count = 0;

    auto r_ap_sq = aperture_radius * aperture_radius;
    auto r_ann_in_sq =
      annulus_inner_radius * annulus_inner_radius;
    auto r_ann_out_sq =
      annulus_outer_radius * annulus_outer_radius;

    for (auto dy = -outer_int; dy <= outer_int; ++dy) {
      for (auto dx = -outer_int; dx <= outer_int;
           ++dx) {
        auto dist_sq = static_cast<double>(
          dx * dx + dy * dy
        );
        auto px = center_x + dx;
        auto py = center_y + dy;
        auto value = to_luminance(image, px, py);

        if (dist_sq <= r_ap_sq) {
          aperture_sum += value;
          ++aperture_count;
        } else if (dist_sq >= r_ann_in_sq
                   && dist_sq <= r_ann_out_sq) {
          annulus_sum += value;
          ++annulus_count;
        }
      }
    }

    if (annulus_count == 0) {
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "No pixels in sky annulus"
        )
      );
    }

    auto sky_per_pixel =
      annulus_sum / static_cast<double>(annulus_count);
    auto raw_flux = aperture_sum
      - sky_per_pixel
        * static_cast<double>(aperture_count);
    auto raw_flux_error = std::sqrt(
      aperture_sum
      + static_cast<double>(aperture_count)
        * sky_per_pixel
    );

    return Nyx::Application::Observation::
      PhotometryResult{
        .raw_flux = raw_flux,
        .raw_flux_error = raw_flux_error,
      };
  }
} // namespace Nyx::Infrastructure::Imaging
