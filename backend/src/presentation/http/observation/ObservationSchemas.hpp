#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Observation {
  inline const auto create_session_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": [
        "target_id", "telescope_id", "camera_id",
        "mount_id", "location_id"
      ],
      "additionalProperties": false,
      "properties": {
        "target_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "telescope_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "camera_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "mount_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "location_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "filter_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "notes": {
          "type": "string", "maxLength": 5000
        }
      }
    })");

  inline const auto update_session_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "additionalProperties": false,
      "properties": {
        "filter_id": {
          "type": "string",
          "minLength": 36, "maxLength": 36
        },
        "notes": {
          "type": "string", "maxLength": 5000
        }
      }
    })");

  inline const auto run_photometry_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["target_x", "target_y"],
      "additionalProperties": false,
      "properties": {
        "target_x": {
          "type": "integer", "minimum": 0
        },
        "target_y": {
          "type": "integer", "minimum": 0
        }
      }
    })");
} // namespace Nyx::Presentation::Http::Observation
