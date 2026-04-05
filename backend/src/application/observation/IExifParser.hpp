#pragma once

#include "core/error/AppError.hpp"

#include <optional>
#include <string>

namespace Nyx::Application::Observation {
  struct ExifData {
    std::optional<std::string> captured_at;
    std::optional<std::string> camera_model;
    std::optional<double> exposure_time_seconds;
    std::optional<int> iso_speed;
    std::optional<double> gps_latitude;
    std::optional<double> gps_longitude;
    std::optional<int> image_width;
    std::optional<int> image_height;
  };

  class IExifParser {
    public:
      virtual ~IExifParser() = default;

      virtual auto parse(
        const std::string& file_path
      ) -> Nyx::Core::Result<ExifData> = 0;
  };
} // namespace Nyx::Application::Observation
