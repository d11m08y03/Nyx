#pragma once

#include "core/error/AppError.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <regex>
#include <spdlog/spdlog.h>

namespace Nyx::Core {
  class ValidationErrorHandler : public nlohmann::json_schema::error_handler {
    public:
      auto error(
        const nlohmann::json::json_pointer& pointer,
        const nlohmann::json& /*instance*/,
        const std::string& message
      ) -> void override {
        auto field = pointer.to_string();
        if (!field.empty() && field[0] == '/') {
          field = field.substr(1);
        }

        if (field.empty()) {
          static const auto required_pattern = std::regex(
            R"(required property '([^']+)')"
          );
          auto match = std::smatch{};
          if (std::regex_search(
            message, match, required_pattern
          )) {
            field = match[1].str();
          }
        }

        this->errors.push_back(FieldError{
          .field = field,
          .message = message,
        });
      }

      [[nodiscard]] auto has_errors() const -> bool {
        return !this->errors.empty();
      }

      auto take_errors() -> std::vector<FieldError> {
        return std::move(this->errors);
      }

    private:
      std::vector<FieldError> errors;
  };

  class RequestValidator {
    public:
      static auto is_valid_uuid(const std::string& value) -> bool {
        static const auto uuid_pattern = std::regex(
          "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
          "[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
        );
        return std::regex_match(value, uuid_pattern);
      }

      static auto validate(
        const nlohmann::json& body,
        const nlohmann::json& schema
      ) -> Result<nlohmann::json> {
        if (!body.is_object()) {
          return std::unexpected(AppError{
            ErrorCode::InvalidJson,
            "Request body must be a JSON object",
            {}
          });
        }

        auto validator =
          nlohmann::json_schema::json_validator{
            nullptr,
            nlohmann::json_schema::default_string_format_check
          };

        try {
          validator.set_root_schema(schema);
        } catch (const std::exception& exception) {
          spdlog::error(
            "JSON schema error: {}", exception.what()
          );
          return std::unexpected(
            AppError::internal("Invalid JSON schema")
          );
        }

        auto handler = ValidationErrorHandler{};
        validator.validate(body, handler);

        if (handler.has_errors()) {
          return std::unexpected(AppError::validation(
            "Request validation failed",
            handler.take_errors()
          ));
        }

        return body;
      }
  };
} // namespace Nyx::Core
