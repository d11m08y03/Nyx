#pragma once

#include <string>

namespace Nyx::Application::Equipment {
  struct CreateTelescopeRequest {
    std::string name;
    int aperture_mm;
    int focal_length_mm;
    std::string optical_design;
    std::string brand;
    std::string model;
  };

  struct UpdateTelescopeRequest {
    std::string name;
    int aperture_mm;
    int focal_length_mm;
    std::string optical_design;
    std::string brand;
    std::string model;
  };

  struct TelescopeResponse {
    std::string id;
    std::string user_id;
    std::string name;
    int aperture_mm;
    int focal_length_mm;
    std::string optical_design;
    std::string brand;
    std::string model;
  };

  struct CreateCameraRequest {
    std::string name;
    std::string sensor_type;
    std::string brand;
    std::string model;
    double pixel_size_um;
    std::string resolution;
  };

  struct UpdateCameraRequest {
    std::string name;
    std::string sensor_type;
    std::string brand;
    std::string model;
    double pixel_size_um;
    std::string resolution;
  };

  struct CameraResponse {
    std::string id;
    std::string user_id;
    std::string name;
    std::string sensor_type;
    std::string brand;
    std::string model;
    double pixel_size_um;
    std::string resolution;
  };

  struct CreateMountRequest {
    std::string name;
    std::string mount_type;
    bool is_tracked;
    bool has_goto;
    std::string brand;
    std::string model;
  };

  struct UpdateMountRequest {
    std::string name;
    std::string mount_type;
    bool is_tracked;
    bool has_goto;
    std::string brand;
    std::string model;
  };

  struct MountResponse {
    std::string id;
    std::string user_id;
    std::string name;
    std::string mount_type;
    bool is_tracked;
    bool has_goto;
    std::string brand;
    std::string model;
  };

  struct CreateFilterRequest {
    std::string name;
    std::string band;
    double bandwidth_nm;
    std::string brand;
    std::string model;
  };

  struct UpdateFilterRequest {
    std::string name;
    std::string band;
    double bandwidth_nm;
    std::string brand;
    std::string model;
  };

  struct FilterResponse {
    std::string id;
    std::string user_id;
    std::string name;
    std::string band;
    double bandwidth_nm;
    std::string brand;
    std::string model;
  };
} // namespace Nyx::Application::Equipment
