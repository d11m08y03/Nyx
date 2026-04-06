#pragma once

#include "application/auth/AuthService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Auth {
  class AuthController : public drogon::HttpController<AuthController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        AuthController::register_user,
        "/api/v1/auth/register",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );
      ADD_METHOD_TO(
        AuthController::login,
        "/api/v1/auth/login",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );
      ADD_METHOD_TO(
        AuthController::refresh,
        "/api/v1/auth/refresh",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );
      ADD_METHOD_TO(
        AuthController::logout,
        "/api/v1/auth/logout",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );
      ADD_METHOD_TO(
        AuthController::verify_email,
        "/api/v1/auth/verify-email",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );
      ADD_METHOD_TO(
        AuthController::resend_verification,
        "/api/v1/auth/resend-verification",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );
      ADD_METHOD_TO(
        AuthController::google_login,
        "/api/v1/auth/google",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::RateLimitFilter"
      );

      METHOD_LIST_END

      AuthController();

      auto register_user(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto login(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto refresh(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto logout(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto verify_email(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto resend_verification(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto google_login(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

    private:
      auto generate_csrf_token() -> std::string;

      std::shared_ptr<Nyx::Application::Auth::AuthService> auth_service;
      bool cookie_secure = true;
      uint32_t refresh_token_max_age_seconds = 604800;
  };
} // namespace Nyx::Presentation::Http::Auth
