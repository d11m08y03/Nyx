#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Nyx::Application::Observation {
  struct CreateSessionRequest {
    std::string target_id;
    std::string telescope_id;
    std::string camera_id;
    std::string mount_id;
    std::string location_id;
    std::optional<std::string> filter_id;
    std::optional<std::string> notes;
  };

  struct UpdateSessionRequest {
    std::optional<std::string> filter_id;
    std::optional<std::string> notes;
  };

  struct UploadedFileData {
    std::string original_filename;
    const char* data;
    std::size_t length;
    std::string mime_type;
  };

  struct RunPhotometryRequest {
    int target_x;
    int target_y;
  };

  struct ImageResponse {
    std::string id;
    std::string session_id;
    std::string original_filename;
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

  struct SessionResponse {
    std::string id;
    std::string user_id;
    std::string target_id;
    std::string telescope_id;
    std::string camera_id;
    std::string mount_id;
    std::string location_id;
    std::optional<std::string> filter_id;
    std::optional<std::string> notes;
    std::string created_at;
    std::string updated_at;
    std::vector<ImageResponse> images;
  };

  struct PhotometryStatusResponse {
    std::string session_id;
    std::string status;
    std::string message;
  };
} // namespace Nyx::Application::Observation
