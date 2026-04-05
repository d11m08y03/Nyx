#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Nyx::Domain {
  struct ObservationImage {
    std::string id;
    std::string session_id;
    std::string filename;
    std::string original_filename;
    std::string file_path;
    int64_t file_size_bytes;
    std::string mime_type;
    std::optional<std::string> captured_at;
    std::optional<std::string> camera_model;
    std::optional<double> exposure_time_seconds;
    std::optional<int> iso_speed;
    std::optional<double> gps_latitude;
    std::optional<double> gps_longitude;
    std::optional<int> image_width;
    std::optional<int> image_height;
    std::optional<int> target_x;
    std::optional<int> target_y;
    std::optional<double> raw_flux;
    std::optional<double> raw_flux_error;
    std::optional<double> relative_flux;
    std::optional<double> relative_flux_error;
    std::optional<std::string> photometry_status;
    std::optional<std::string> photometry_error_message;
    std::string created_at;
  };
} // namespace Nyx::Domain
