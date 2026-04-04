#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Equipment {
  // Telescope schemas

  inline const auto create_telescope_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["name", "aperture_mm", "focal_length_mm", "optical_design"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string", "minLength": 1, "maxLength": 255 },
        "aperture_mm": { "type": "integer", "minimum": 1 },
        "focal_length_mm": { "type": "integer", "minimum": 1 },
        "optical_design": { "type": "string", "minLength": 1, "maxLength": 50 },
        "brand": { "type": "string", "maxLength": 255 },
        "model": { "type": "string", "maxLength": 255 }
      }
    })");

  inline const auto update_telescope_schema = create_telescope_schema;

  // Camera schemas

  inline const auto create_camera_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["name", "sensor_type"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string", "minLength": 1, "maxLength": 255 },
        "sensor_type": { "type": "string", "minLength": 1, "maxLength": 50 },
        "brand": { "type": "string", "maxLength": 255 },
        "model": { "type": "string", "maxLength": 255 },
        "pixel_size_um": { "type": "number", "minimum": 0 },
        "resolution": { "type": "string", "maxLength": 50 }
      }
    })");

  inline const auto update_camera_schema = create_camera_schema;

  // Mount schemas

  inline const auto create_mount_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["name", "mount_type"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string", "minLength": 1, "maxLength": 255 },
        "mount_type": { "type": "string", "minLength": 1, "maxLength": 50 },
        "is_tracked": { "type": "boolean" },
        "has_goto": { "type": "boolean" },
        "brand": { "type": "string", "maxLength": 255 },
        "model": { "type": "string", "maxLength": 255 }
      }
    })");

  inline const auto update_mount_schema = create_mount_schema;

  // Filter schemas

  inline const auto create_filter_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["name", "band"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string", "minLength": 1, "maxLength": 255 },
        "band": { "type": "string", "minLength": 1, "maxLength": 50 },
        "bandwidth_nm": { "type": "number", "minimum": 0 },
        "brand": { "type": "string", "maxLength": 255 },
        "model": { "type": "string", "maxLength": 255 }
      }
    })");

  inline const auto update_filter_schema = create_filter_schema;
} // namespace Nyx::Presentation::Http::Equipment
