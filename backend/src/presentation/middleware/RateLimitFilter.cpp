#include "presentation/middleware/RateLimitFilter.hpp"
#include "presentation/http/ResponseHelper.hpp"

#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Middleware {
  auto RateLimitFilter::doFilter(
    const drogon::HttpRequestPtr& request,
    drogon::FilterCallback&& failure_callback,
    drogon::FilterChainCallback&& chain_callback
  ) -> void {
    auto client_ip = request->peerAddr().toIp();
    auto now = std::chrono::steady_clock::now();

    auto lock = std::lock_guard<std::mutex>{this->mutex};

    auto& timestamps = this->requests[client_ip];

    while (!timestamps.empty() && (now - timestamps.front()) > window) {
      timestamps.pop_front();
    }

    if (static_cast<int>(timestamps.size()) >= max_requests) {
      auto correlation_id = request->getAttributes()->get<std::string>(
        "correlation_id"
      );
      auto logger = request->getAttributes()->get<std::shared_ptr<spdlog::logger>>(
        "logger"
      );

      if (!logger) {
        logger = spdlog::default_logger();
      }

      logger->warn("Rate limit exceeded for ip={}", client_ip);

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::RateLimitExceeded,
        "Too many requests, please try again later",
        {}
      };

      failure_callback(Nyx::Presentation::Http::ResponseHelper::error(
        error, correlation_id
      ));
      return;
    }

    timestamps.push_back(now);
    chain_callback();
  }
} // namespace Nyx::Presentation::Middleware
