#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Observation {
  inline const auto create_session_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["target_id"],
      "additionalProperties": false,
      "properties": {
        "target_id": {
          "type": "string",
          "minLength": 36,
          "maxLength": 36
        },
        "notes": {
          "type": "string",
          "maxLength": 5000
        }
      }
    })");

  inline const auto update_session_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "additionalProperties": false,
      "properties": {
        "notes": {
          "type": "string",
          "maxLength": 5000
        }
      }
    })");
} // namespace Nyx::Presentation::Http::Observation
