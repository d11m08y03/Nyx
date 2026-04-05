#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Target {
  inline const auto resolve_target_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["target_name"],
      "additionalProperties": false,
      "properties": {
        "target_name": { "type": "string", "minLength": 1, "maxLength": 255 }
      }
    })");
} // namespace Nyx::Presentation::Http::Target
