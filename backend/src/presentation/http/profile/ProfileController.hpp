#pragma once

#include "application/profile/ProfileService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Profile {
  class ProfileController
    : public drogon::HttpController<ProfileController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        ProfileController::complete_onboarding,
        "/api/v1/users/me/profile",
        drogon::Put,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter",
        "Nyx::Presentation::Middleware::CsrfFilter"
      );

      METHOD_LIST_END

      ProfileController();

      auto complete_onboarding(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

    private:
      std::shared_ptr<Nyx::Application::Profile::ProfileService> profile_service;
  };
} // namespace Nyx::Presentation::Http::Profile
