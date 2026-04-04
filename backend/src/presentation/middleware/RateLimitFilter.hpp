#pragma once

#include <chrono>
#include <deque>
#include <drogon/HttpFilter.h>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Nyx::Presentation::Middleware {
  class RateLimitFilter
    : public drogon::HttpFilter<RateLimitFilter> {
    public:
      auto doFilter(
        const drogon::HttpRequestPtr& request,
        drogon::FilterCallback&& failure_callback,
        drogon::FilterChainCallback&& chain_callback
      ) -> void override;

    private:
      static constexpr auto max_requests = 10;
      static constexpr auto window = std::chrono::minutes(1);

      using time_point = std::chrono::steady_clock::time_point;

      std::mutex mutex;
      std::unordered_map<std::string, std::deque<time_point>> requests;
  };
} // namespace Nyx::Presentation::Middleware
