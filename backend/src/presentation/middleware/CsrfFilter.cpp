#include "presentation/middleware/CsrfFilter.hpp"
#include "presentation/http/ResponseHelper.hpp"

#include <sodium.h>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Middleware {
  auto CsrfFilter::doFilter(
    const drogon::HttpRequestPtr& request,
    drogon::FilterCallback&& failure_callback,
    drogon::FilterChainCallback&& chain_callback
  ) -> void {
    auto method = request->method();
    if (method == drogon::Get
      || method == drogon::Head
      || method == drogon::Options) {
      chain_callback();
      return;
    }

    auto correlation_id =
      request->getAttributes()
        ->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>(
          "logger"
        );

    if (!logger) {
      logger = spdlog::default_logger();
    }

    auto cookie_token = request->getCookie("csrf_token");
    auto header_token = std::string{
      request->getHeader("X-CSRF-Token")
    };

    if (cookie_token.empty() || header_token.empty()) {
      logger->warn("CSRF validation failed: missing token");

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::PermissionDenied,
        "CSRF token is missing",
        {}
      };

      failure_callback(
        Nyx::Presentation::Http::ResponseHelper::error(
          error, correlation_id
        )
      );
      return;
    }

    if (cookie_token.size() != header_token.size()
      || sodium_memcmp(
          cookie_token.data(),
          header_token.data(),
          cookie_token.size()
        ) != 0) {
      logger->warn("CSRF validation failed: token mismatch");

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::PermissionDenied,
        "CSRF token is invalid",
        {}
      };

      failure_callback(
        Nyx::Presentation::Http::ResponseHelper::error(
          error, correlation_id
        )
      );
      return;
    }

    chain_callback();
  }
} // namespace Nyx::Presentation::Middleware
