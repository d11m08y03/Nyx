#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Profile {
  inline const auto onboarding_request_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["display_name"],
      "additionalProperties": false,
      "properties": {
        "display_name": {
          "type": "string",
          "minLength": 2,
          "maxLength": 50
        }
      }
    })");
} // namespace Nyx::Presentation::Http::Profile
