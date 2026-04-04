#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Location {
  inline const auto create_location_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["name", "latitude", "longitude"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string", "minLength": 1, "maxLength": 255 },
        "latitude": { "type": "number", "minimum": -90, "maximum": 90 },
        "longitude": { "type": "number", "minimum": -180, "maximum": 180 },
        "bortle_class": { "type": "integer", "minimum": 1, "maximum": 9 }
      }
    })");

  inline const auto update_location_schema = create_location_schema;
} // namespace Nyx::Presentation::Http::Location
