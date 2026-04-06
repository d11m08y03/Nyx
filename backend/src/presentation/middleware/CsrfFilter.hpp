#pragma once

#include <drogon/HttpFilter.h>

namespace Nyx::Presentation::Middleware {
  class CsrfFilter
    : public drogon::HttpFilter<CsrfFilter> {
    public:
      auto doFilter(
        const drogon::HttpRequestPtr& request,
        drogon::FilterCallback&& failure_callback,
        drogon::FilterChainCallback&& chain_callback
      ) -> void override;
  };
} // namespace Nyx::Presentation::Middleware
