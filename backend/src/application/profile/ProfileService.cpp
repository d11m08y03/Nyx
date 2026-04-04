#include "application/profile/ProfileService.hpp"

namespace Nyx::Application::Profile {
  ProfileService::ProfileService(
    std::shared_ptr<Nyx::Domain::IUserRepository> user_repository
  )
    : user_repository(std::move(user_repository)) {
    spdlog::debug("ProfileService initialized");
  }

  auto ProfileService::complete_onboarding(
    const std::string& user_id,
    const OnboardingRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Completing onboarding for user_id={}", user_id);

    auto result = this->user_repository->update_display_name(
      user_id, request.display_name
    );

    if (!result.has_value()) {
      logger->error(
        "Failed to update display_name for user_id={}", user_id
      );
      return std::unexpected(result.error());
    }

    logger->info("Onboarding completed for user_id={}", user_id);
    return {};
  }
} // namespace Nyx::Application::Profile
