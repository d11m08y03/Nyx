#include "core/validation/RequestValidator.hpp"

#include <gtest/gtest.h>

namespace Nyx::Core::Tests {
  static const auto test_schema = nlohmann::json::parse(R"({
    "type": "object",
    "required": ["email", "password"],
    "additionalProperties": false,
    "properties": {
      "email": { "type": "string", "format": "email" },
      "password": { "type": "string", "minLength": 8 }
    }
  })");

  TEST(RequestValidatorTest, ValidObjectPasses) {
    auto body = nlohmann::json{{"email", "a@b.com"}, {"password", "12345678"}};
    auto result = RequestValidator::validate(body, test_schema);
    ASSERT_TRUE(result.has_value());
  }

  TEST(RequestValidatorTest, MissingRequiredFieldFails) {
    auto body = nlohmann::json{{"email", "a@b.com"}};
    auto result = RequestValidator::validate(body, test_schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ValidationError);
    ASSERT_FALSE(result.error().details.empty());
    EXPECT_EQ(result.error().details[0].field, "password");
  }

  TEST(RequestValidatorTest, InvalidEmailFormatFails) {
    auto body = nlohmann::json{{"email", "not-an-email"}, {"password", "12345678"}};
    auto result = RequestValidator::validate(body, test_schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ValidationError);
    ASSERT_FALSE(result.error().details.empty());
    EXPECT_EQ(result.error().details[0].field, "email");
  }

  TEST(RequestValidatorTest, PasswordTooShortFails) {
    auto body = nlohmann::json{{"email", "a@b.com"}, {"password", "short"}};
    auto result = RequestValidator::validate(body, test_schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ValidationError);
    ASSERT_FALSE(result.error().details.empty());
    EXPECT_EQ(result.error().details[0].field, "password");
  }

  TEST(RequestValidatorTest, ExtraFieldsRejected) {
    auto body = nlohmann::json{
      {"email", "a@b.com"}, {"password", "12345678"}, {"extra", "field"}
    };
    auto result = RequestValidator::validate(body, test_schema);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ValidationError);
  }

  TEST(RequestValidatorTest, NonObjectBodyReturnsInvalidJson) {
    auto body = nlohmann::json("just a string");
    auto result = RequestValidator::validate(body, test_schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidJson);
  }

  TEST(RequestValidatorTest, NullBodyReturnsInvalidJson) {
    auto body = nlohmann::json(nullptr);
    auto result = RequestValidator::validate(body, test_schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidJson);
  }
} // namespace Nyx::Core::Tests
