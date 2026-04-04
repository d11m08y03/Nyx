#pragma once

#include <drogon/HttpFilter.h>

namespace Nyx::Presentation::Middleware {
  class CorrelationIdFilter
    : public drogon::HttpFilter<CorrelationIdFilter> {
    public:
      auto doFilter(
        const drogon::HttpRequestPtr& request,
        drogon::FilterCallback&& failure_callback,
        drogon::FilterChainCallback&& chain_callback
      ) -> void override;
  };
} // namespace Nyx::Presentation::Middleware
