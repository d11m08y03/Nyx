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

    private:
      std::shared_ptr<Nyx::Application::Auth::AuthService> auth_service;
  };
} // namespace Nyx::Presentation::Http::Auth
