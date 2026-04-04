#pragma once

#include <nlohmann/json.hpp>

namespace Nyx::Presentation::Http::Auth {
  inline const auto register_request_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["email", "password"],
      "additionalProperties": false,
      "properties": {
        "email": {
          "type": "string",
          "format": "email"
        },
        "password": {
          "type": "string",
          "minLength": 8
        }
      }
    })");

  inline const auto login_request_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["email", "password"],
      "additionalProperties": false,
      "properties": {
        "email": {
          "type": "string"
        },
        "password": {
          "type": "string"
        }
      }
    })");

  inline const auto refresh_request_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["refresh_token"],
      "additionalProperties": false,
      "properties": {
        "refresh_token": {
          "type": "string"
        }
      }
    })");
} // namespace Nyx::Presentation::Http::Auth
