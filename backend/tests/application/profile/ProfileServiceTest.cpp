#include "application/profile/ProfileService.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Profile::Tests {
  class MockUserRepository : public Nyx::Domain::IUserRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::User>, create,
        (const Nyx::Domain::User& user), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::User>>), find_by_email,
        (const std::string& email), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, update_display_name,
        (const std::string& user_id, const std::string& display_name),
        (override)
      );
  };

  class ProfileServiceTest : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->user_repo = std::make_shared<MockUserRepository>();
        this->logger = spdlog::default_logger();
        this->service = std::make_unique<ProfileService>(this->user_repo);
      }

      std::shared_ptr<MockUserRepository> user_repo;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<ProfileService> service;
  };

  TEST_F(ProfileServiceTest, CompleteOnboardingSuccess) {
    auto request = OnboardingRequest{.display_name = "StarGazer"};

    EXPECT_CALL(*this->user_repo, update_display_name("user-1", "StarGazer"))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->complete_onboarding(
      "user-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(ProfileServiceTest, CompleteOnboardingUserNotFound) {
    auto request = OnboardingRequest{.display_name = "StarGazer"};

    EXPECT_CALL(*this->user_repo, update_display_name("bad-id", "StarGazer"))
      .WillOnce(::testing::Return(
        std::unexpected(Nyx::Core::AppError::not_found("User not found"))
      ));

    auto result = this->service->complete_onboarding(
      "bad-id", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::ResourceNotFound);
  }

  TEST_F(ProfileServiceTest, CompleteOnboardingDatabaseError) {
    auto request = OnboardingRequest{.display_name = "StarGazer"};

    EXPECT_CALL(*this->user_repo, update_display_name("user-1", "StarGazer"))
      .WillOnce(::testing::Return(
        std::unexpected(Nyx::Core::AppError::internal("Database error"))
      ));

    auto result = this->service->complete_onboarding(
      "user-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::InternalError);
  }
} // namespace Nyx::Application::Profile::Tests
