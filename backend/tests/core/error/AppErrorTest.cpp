#include "core/error/AppError.hpp"

#include <gtest/gtest.h>

namespace Nyx::Core::Tests {
  TEST(AppErrorTest, ValidationErrorReturnsHttp400) {
    auto error = AppError::validation("Invalid input");
    EXPECT_EQ(error.http_status_code(), 400);
    EXPECT_EQ(error.error_code_string(), "VALIDATION_ERROR");
  }

  TEST(AppErrorTest, NotFoundReturnsHttp404) {
    auto error = AppError::not_found("User not found");
    EXPECT_EQ(error.http_status_code(), 404);
    EXPECT_EQ(
      error.error_code_string(), "RESOURCE_NOT_FOUND"
    );
  }

  TEST(AppErrorTest, UnauthorizedReturnsHttp401) {
    auto error = AppError::unauthorized("Invalid credentials");
    EXPECT_EQ(error.http_status_code(), 401);
    EXPECT_EQ(
      error.error_code_string(), "AUTHENTICATION_REQUIRED"
    );
  }

  TEST(AppErrorTest, ConflictReturnsHttp409) {
    auto error = AppError::conflict("Email already exists");
    EXPECT_EQ(error.http_status_code(), 409);
    EXPECT_EQ(error.error_code_string(), "CONFLICT");
  }

  TEST(AppErrorTest, ValidationErrorWithDetails) {
    auto error = AppError::validation(
      "Missing fields",
      {
        {"email", "required"},
        {"password", "must be at least 8 characters"}
      }
    );

    EXPECT_EQ(error.details.size(), 2);
    EXPECT_EQ(error.details[0].field, "email");
    EXPECT_EQ(error.details[1].field, "password");
  }

  TEST(AppErrorTest, ResultTypeWithSuccess) {
    Result<int> result = 42;
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
  }

  TEST(AppErrorTest, ResultTypeWithError) {
    Result<int> result = std::unexpected(AppError::not_found("Not found"));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().http_status_code(), 404);
  }
} // namespace Nyx::Core::Tests
