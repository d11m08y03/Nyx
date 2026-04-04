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
  // UUID validation

  TEST(RequestValidatorTest, ValidUuidPasses) {
    EXPECT_TRUE(RequestValidator::is_valid_uuid(
      "d7311a21-8a87-4aa4-af87-4757d081509a"
    ));
  }

  TEST(RequestValidatorTest, ValidUuidUppercasePasses) {
    EXPECT_TRUE(RequestValidator::is_valid_uuid(
      "D7311A21-8A87-4AA4-AF87-4757D081509A"
    ));
  }

  TEST(RequestValidatorTest, EmptyStringFailsUuid) {
    EXPECT_FALSE(RequestValidator::is_valid_uuid(""));
  }

  TEST(RequestValidatorTest, PlainStringFailsUuid) {
    EXPECT_FALSE(RequestValidator::is_valid_uuid("not-a-uuid"));
  }

  TEST(RequestValidatorTest, UuidWithoutHyphensFailsUuid) {
    EXPECT_FALSE(RequestValidator::is_valid_uuid(
      "d7311a218a874aa4af874757d081509a"
    ));
  }

  TEST(RequestValidatorTest, UuidTooShortFailsUuid) {
    EXPECT_FALSE(RequestValidator::is_valid_uuid(
      "d7311a21-8a87-4aa4-af87"
    ));
  }

  TEST(RequestValidatorTest, UuidWithExtraCharsFailsUuid) {
    EXPECT_FALSE(RequestValidator::is_valid_uuid(
      "d7311a21-8a87-4aa4-af87-4757d081509a-extra"
    ));
  }
} // namespace Nyx::Core::Tests
