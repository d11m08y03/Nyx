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

  inline const auto verify_email_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["token"],
      "additionalProperties": false,
      "properties": {
        "token": {
          "type": "string",
          "minLength": 1
        }
      }
    })");

  inline const auto resend_verification_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["email"],
      "additionalProperties": false,
      "properties": {
        "email": {
          "type": "string",
          "format": "email"
        }
      }
    })");

  inline const auto google_login_schema =
    nlohmann::json::parse(R"({
      "type": "object",
      "required": ["code", "redirect_uri"],
      "additionalProperties": false,
      "properties": {
        "code": {
          "type": "string",
          "minLength": 1
        },
        "redirect_uri": {
          "type": "string",
          "minLength": 1
        }
      }
    })");
} // namespace Nyx::Presentation::Http::Auth
