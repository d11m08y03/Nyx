#pragma once

#include "application/auth/ITokenService.hpp"

#include <drogon/HttpFilter.h>
#include <memory>

namespace Nyx::Presentation::Middleware {
  class JwtAuthFilter
    : public drogon::HttpFilter<JwtAuthFilter> {
    public:
      JwtAuthFilter();

      auto doFilter(
        const drogon::HttpRequestPtr& request,
        drogon::FilterCallback&& failure_callback,
        drogon::FilterChainCallback&& chain_callback
      ) -> void override;

    private:
      std::shared_ptr<
        Nyx::Application::Auth::ITokenService
      > token_service;
  };
} // namespace Nyx::Presentation::Middleware
