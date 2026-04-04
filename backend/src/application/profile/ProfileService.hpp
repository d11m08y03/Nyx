#pragma once

#include "application/profile/Dtos.hpp"
#include "core/error/AppError.hpp"
#include "domain/repositories/IUserRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>

namespace Nyx::Application::Profile {
  class ProfileService {
    public:
      ProfileService(
        std::shared_ptr<Nyx::Domain::IUserRepository> user_repository
      );

      auto complete_onboarding(
        const std::string& user_id,
        const OnboardingRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

    private:
      std::shared_ptr<Nyx::Domain::IUserRepository> user_repository;
  };
} // namespace Nyx::Application::Profile
